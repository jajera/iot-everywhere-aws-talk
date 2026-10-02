import hashlib
import base64
import json
import os
import time
from decimal import Decimal

try:
    import boto3
except ModuleNotFoundError:
    boto3 = None

TELEMETRY_TABLE_NAME = os.getenv("TELEMETRY_TABLE_NAME", "iot-talk-telemetry")
EVENTS_TABLE_NAME = os.getenv("EVENTS_TABLE_NAME", "iot-talk-events")
METRICS_TABLE_NAME = os.getenv("METRICS_TABLE_NAME", "iot-talk-metrics")
CAMERA_BUCKET = os.getenv("CAMERA_BUCKET", "")
ROLLUP_BUCKET_S = int(os.getenv("ROLLUP_BUCKET_S", "60"))

dynamodb = boto3.resource("dynamodb") if boto3 else None
s3 = boto3.client("s3") if boto3 else None


def _now_epoch() -> int:
    return int(time.time())


def _normalize_event(event):
    if isinstance(event, str):
        return json.loads(event)
    if isinstance(event, dict) and "body" in event and isinstance(event["body"], str):
        return json.loads(event["body"])
    if isinstance(event, dict):
        return event
    raise ValueError("Unsupported event payload")


def _extract_record_type(payload: dict) -> str:
    kind = payload.get("type")
    if kind == "button":
        return "event"
    if kind == "camera":
        return "camera"
    return "telemetry"


def _effective_ts(payload: dict):
    try:
        ts = int(payload.get("ts", 0))
    except (TypeError, ValueError):
        ts = 0
    if ts <= 0:
        return _now_epoch(), True
    return ts, False


def _record_id(payload: dict, record_type: str, effective_ts: int) -> str:
    # Strip bulky fields from identity hash
    slim = {k: v for k, v in payload.items() if k not in ("jpeg_b64", "data_b64")}
    canonical = json.dumps(slim, sort_keys=True, separators=(",", ":"))
    digest_input = f"{record_type}:{payload['device_id']}:{effective_ts}:{canonical}"
    return hashlib.sha256(digest_input.encode("utf-8")).hexdigest()


def _to_dynamo_value(value):
    if isinstance(value, float):
        return Decimal(str(value))
    if isinstance(value, list):
        return [_to_dynamo_value(item) for item in value]
    if isinstance(value, dict):
        return {k: _to_dynamo_value(v) for k, v in value.items()}
    return value


def _validate_payload(payload: dict):
    for field in ("device_id", "ts", "type"):
        if field not in payload:
            raise ValueError(f"Missing required field: {field}")
    if not str(payload["device_id"]).strip():
        raise ValueError("device_id must be non-empty")


def _store_camera(payload: dict, effective_ts: int):
    if not CAMERA_BUCKET:
        raise RuntimeError("CAMERA_BUCKET is not configured")
    if s3 is None:
        raise RuntimeError("S3 client is not initialized")

    b64 = payload.get("jpeg_b64") or payload.get("data_b64")
    if not b64:
        raise ValueError("camera payload missing jpeg_b64")

    raw = base64.b64decode(b64)
    if len(raw) < 100 or raw[:2] != b"\xff\xd8":
        raise ValueError("camera payload is not a JPEG")

    device_id = str(payload["device_id"])
    key = f"cameras/{device_id}/latest.jpg"
    s3.put_object(
        Bucket=CAMERA_BUCKET,
        Key=key,
        Body=raw,
        ContentType="image/jpeg",
        Metadata={
            "device_id": device_id,
            "ts": str(effective_ts),
            "bytes": str(len(raw)),
        },
        CacheControl="no-cache",
    )

    # Lightweight event row (no base64 in DynamoDB)
    if dynamodb is None:
        raise RuntimeError("DynamoDB client is not initialized")
    meta = {
        "device_id": device_id,
        "ts": effective_ts,
        "type": "camera",
        "camera_bytes": len(raw),
        "camera_s3_key": key,
        "camera_ok": True,
    }
    table = dynamodb.Table(EVENTS_TABLE_NAME)
    record_id = _record_id(meta, "camera", effective_ts)
    table.put_item(
        Item={
            "device_id": device_id,
            "record_id": record_id,
            "record_type": "camera",
            "effective_ts": effective_ts,
            "ts_original": int(payload.get("ts", 0) or 0),
            "ts_fallback_used": False,
            "ingest_ts": _now_epoch(),
            "payload": _to_dynamo_value(meta),
        }
    )
    return key, len(raw)


def lambda_handler(event, _context):
    payload = _normalize_event(event)
    _validate_payload(payload)

    record_type = _extract_record_type(payload)
    effective_ts, ts_fallback_used = _effective_ts(payload)

    if record_type == "camera":
        key, nbytes = _store_camera(payload, effective_ts)
        return {
            "statusCode": 200,
            "record_type": "camera",
            "s3_key": key,
            "bytes": nbytes,
            "effective_ts": effective_ts,
        }

    record_id = _record_id(payload, record_type, effective_ts)

    if dynamodb is None:
        raise RuntimeError("DynamoDB client is not initialized")

    table_name = EVENTS_TABLE_NAME if record_type == "event" else TELEMETRY_TABLE_NAME
    table = dynamodb.Table(table_name)

    item = {
        "device_id": str(payload["device_id"]),
        "record_id": record_id,
        "record_type": record_type,
        "effective_ts": effective_ts,
        "ts_original": int(payload.get("ts", 0) or 0),
        "ts_fallback_used": ts_fallback_used,
        "ingest_ts": _now_epoch(),
        "payload": _to_dynamo_value(payload),
    }

    table.put_item(Item=item)

    if record_type == "telemetry":
        _upsert_rollup(str(payload["device_id"]), effective_ts, payload)

    return {
        "statusCode": 200,
        "record_type": record_type,
        "table_name": table_name,
        "record_id": record_id,
        "effective_ts": effective_ts,
    }


def _as_decimal(value):
    if value is None:
        return None
    try:
        return Decimal(str(float(value)))
    except (TypeError, ValueError):
        return None


def _upsert_rollup(device_id: str, effective_ts: int, payload: dict):
    """1-minute aggregates so fleet analytics can skip raw telemetry scans."""
    if not METRICS_TABLE_NAME or dynamodb is None:
        return
    bucket_ts = (effective_ts // ROLLUP_BUCKET_S) * ROLLUP_BUCKET_S
    table = dynamodb.Table(METRICS_TABLE_NAME)

    expr = ["#n :one"]
    names = {"#n": "n"}
    values = {":one": 1}

    rssi = _as_decimal(payload.get("rssi"))
    if rssi is not None:
        expr.append("rssi_sum :rssi")
        expr.append("rssi_n :one")
        values[":rssi"] = rssi

    temp = _as_decimal(payload.get("chip_temp_c"))
    if temp is not None:
        expr.append("temp_sum :temp")
        expr.append("temp_n :one")
        values[":temp"] = temp

    update = f"ADD {', '.join(expr)}"
    set_parts = ["updated_at = :now"]
    values[":now"] = _now_epoch()

    heap = payload.get("heap_free")
    if heap is not None:
        try:
            heap_i = int(heap)
            set_parts.append("heap_last = :heap")
            values[":heap"] = heap_i
            # Keep running min via condition-less SET if_not_exists + separate path:
            # use if_not_exists then client can't easily min; store heap_min with
            # if_not_exists and accept approximate (first sample) — good enough for demo.
            set_parts.append("heap_min = if_not_exists(heap_min, :heap)")
        except (TypeError, ValueError):
            pass

    uptime = payload.get("uptime_s")
    if uptime is not None:
        try:
            set_parts.append("uptime_last = :up")
            values[":up"] = int(uptime)
        except (TypeError, ValueError):
            pass

    reset = payload.get("reset_reason")
    if reset is not None:
        try:
            set_parts.append("reset_last = :rr")
            values[":rr"] = int(reset)
        except (TypeError, ValueError):
            pass

    update = f"{update} SET {', '.join(set_parts)}"
    try:
        table.update_item(
            Key={"device_id": device_id, "bucket_ts": bucket_ts},
            UpdateExpression=update,
            ExpressionAttributeNames=names,
            ExpressionAttributeValues=values,
        )
    except Exception:
        # Rollup is best-effort; raw telemetry remains source of truth.
        pass
