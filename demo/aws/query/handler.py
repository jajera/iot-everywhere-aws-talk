import json
import os
import re
import time
from concurrent.futures import ThreadPoolExecutor
from decimal import Decimal

try:
    import boto3
    from boto3.dynamodb.conditions import Key
except ModuleNotFoundError:
    boto3 = None
    Key = None

TELEMETRY_TABLE_NAME = os.getenv("TELEMETRY_TABLE_NAME", "iot-talk-telemetry")
EVENTS_TABLE_NAME = os.getenv("EVENTS_TABLE_NAME", "iot-talk-events")
METRICS_TABLE_NAME = os.getenv("METRICS_TABLE_NAME", "iot-talk-metrics")
CAMERA_BUCKET = os.getenv("CAMERA_BUCKET", "")
PRESIGN_TTL_S = int(os.getenv("CAMERA_PRESIGN_TTL_S", "900"))
DEFAULT_EVENTS_LIMIT = int(os.getenv("DEFAULT_EVENTS_LIMIT", "10"))
MAX_EVENTS_LIMIT = int(os.getenv("MAX_EVENTS_LIMIT", "50"))
DEFAULT_HISTORY_LIMIT = int(os.getenv("DEFAULT_HISTORY_LIMIT", "60"))
MAX_HISTORY_LIMIT = int(os.getenv("MAX_HISTORY_LIMIT", "200"))
STALE_AFTER_S = int(os.getenv("STALE_AFTER_S", "120"))
EXPECTED_INTERVAL_S = int(os.getenv("EXPECTED_INTERVAL_S", "15"))
WEAK_RSSI_DBM = int(os.getenv("WEAK_RSSI_DBM", "-75"))
HOT_TEMP_C = float(os.getenv("HOT_TEMP_C", "70"))
ANALYTICS_CACHE_TTL_S = int(os.getenv("ANALYTICS_CACHE_TTL_S", "30"))
# Longer windows change slowly — keep warm Lambda responses longer.
ANALYTICS_CACHE_TTL_BY_WINDOW = {
    3600: int(os.getenv("ANALYTICS_CACHE_TTL_1H", "25")),
    21600: int(os.getenv("ANALYTICS_CACHE_TTL_6H", "60")),
    86400: int(os.getenv("ANALYTICS_CACHE_TTL_24H", "120")),
}
CAMERA_CACHE_TTL_S = int(os.getenv("CAMERA_CACHE_TTL_S", "120"))
MAX_FLEET_EVENTS = int(os.getenv("MAX_FLEET_EVENTS", "100"))
ROLLUP_BUCKET_S = int(os.getenv("ROLLUP_BUCKET_S", "60"))
# Prefer rollups when window is large enough that raw scans hurt.
ROLLUP_MIN_WINDOW_S = int(os.getenv("ROLLUP_MIN_WINDOW_S", "3600"))
# window seconds -> bucket seconds. Longer windows need a pre-aggregated store.
WINDOW_BUCKETS = {3600: 60, 21600: 300, 86400: 900}
DEFAULT_WINDOW_S = 3600

ANALYTICS_FIELDS = (
    "ts",
    "rssi",
    "chip_temp_c",
    "heap_free",
    "heap_min_free",
    "uptime_s",
    "reset_reason",
    "clock_offset_ms",
)

_analytics_cache = {}
_camera_cache = {}

dynamodb = boto3.resource("dynamodb") if boto3 else None
s3 = boto3.client("s3") if boto3 else None

JSON_HEADERS = {"content-type": "application/json"}


def _json_default(value):
    if isinstance(value, Decimal):
        if value % 1 == 0:
            return int(value)
        return float(value)
    raise TypeError(f"Unsupported JSON type: {type(value)}")


def _response(status_code: int, body: dict):
    return {
        "statusCode": status_code,
        "headers": JSON_HEADERS,
        "body": json.dumps(body, default=_json_default),
    }


def _table(name: str):
    if dynamodb is None:
        raise RuntimeError("DynamoDB client is not initialized")
    return dynamodb.Table(name)


def _as_int(value, default=0):
    if value is None:
        return default
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def _parse_limit(event, default, maximum):
    raw = (event.get("queryStringParameters") or {}).get("limit")
    if raw is None or raw == "":
        return default
    value = int(raw)
    if value <= 0:
        raise ValueError("limit must be > 0")
    return min(value, maximum)


def _query_device_ts(table_name: str, device_id: str, limit: int):
    result = _table(table_name).query(
        IndexName="device_ts_idx",
        KeyConditionExpression=Key("device_id").eq(device_id),
        ScanIndexForward=False,
        Limit=limit,
    )
    return result.get("Items", [])


def _payload(item: dict) -> dict:
    return item.get("payload") or {}


def _seen_ts(item: dict) -> int:
    payload = _payload(item)
    ts = _as_int(payload.get("ts"), 0)
    if ts > 0:
        return ts
    return _as_int(item.get("effective_ts") or item.get("ingest_ts"), 0)


def _as_float(value):
    if value is None:
        return None
    if isinstance(value, Decimal):
        return float(value)
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def _device_ids():
    """Known device ids: DEVICE_IDS env (comma/semicolon), else IoT Things."""
    raw = os.getenv("DEVICE_IDS", "").strip()
    if raw:
        parts = raw.replace(";", ",").split(",")
        return [part.strip() for part in parts if part.strip()]
    if boto3 is None:
        return []
    try:
        client = boto3.client("iot")
        names = []
        token = None
        while True:
            kwargs = {"maxResults": 100}
            if token:
                kwargs["nextToken"] = token
            page = client.list_things(**kwargs)
            names.extend(thing["thingName"] for thing in page.get("things", []))
            token = page.get("nextToken")
            if not token:
                break
        return sorted(set(names))
    except Exception:
        # Missing iot:ListThings or empty registry — prefer DEVICE_IDS env.
        return []


def _latest_item(device_id: str):
    items = _query_device_ts(TELEMETRY_TABLE_NAME, device_id, 1)
    return items[0] if items else None


def _device_from_item(device_id: str, item: dict, now: int):
    payload = _payload(item)
    seen = _seen_ts(item)
    age = now - seen if seen else None
    return {
        "device_id": device_id,
        "model": payload.get("model") or payload.get("chip_model"),
        "chip_model": payload.get("chip_model"),
        "rssi": _as_float(payload.get("rssi")),
        "chip_temp_c": _as_float(payload.get("chip_temp_c")),
        "heap_free": _as_int(payload.get("heap_free"), None)
        if payload.get("heap_free") is not None
        else None,
        "uptime_s": _as_int(payload.get("uptime_s"), None)
        if payload.get("uptime_s") is not None
        else None,
        "wifi_ssid": payload.get("wifi_ssid"),
        "wifi_ip": payload.get("wifi_ip"),
        "last_seen_ts": seen,
        "age_s": age,
        "live": age is not None and age <= STALE_AFTER_S,
        "telemetry": payload,
    }


def _camera_latest(device_id: str):
    """Presigned latest JPEG if present in S3."""
    if not CAMERA_BUCKET or s3 is None or not device_id:
        return None
    if not _looks_like_camera(device_id):
        return None
    now = int(time.time())
    cached = _camera_cache.get(device_id)
    if cached and now - cached[0] < CAMERA_CACHE_TTL_S:
        return cached[1]

    key = f"cameras/{device_id}/latest.jpg"
    try:
        head = s3.head_object(Bucket=CAMERA_BUCKET, Key=key)
    except Exception:
        _camera_cache[device_id] = (now, None)
        return None
    meta = head.get("Metadata") or {}
    url = s3.generate_presigned_url(
        "get_object",
        Params={"Bucket": CAMERA_BUCKET, "Key": key},
        ExpiresIn=PRESIGN_TTL_S,
    )
    body = {
        "url": url,
        "key": key,
        "bytes": head.get("ContentLength"),
        "ts": _as_int(meta.get("ts"), 0) or None,
        "expires_in_s": PRESIGN_TTL_S,
    }
    _camera_cache[device_id] = (now, body)
    return body


def _looks_like_camera(device_id: str, model: str = None) -> bool:
    blob = f"{device_id or ''} {model or ''}".lower()
    return "cam" in blob


def _query_metrics_range(device_id: str, since: int):
    if not METRICS_TABLE_NAME:
        return []
    table = _table(METRICS_TABLE_NAME)
    kwargs = {
        "KeyConditionExpression": Key("device_id").eq(device_id)
        & Key("bucket_ts").gte(since),
        "ScanIndexForward": True,
    }
    items = []
    while True:
        page = table.query(**kwargs)
        items.extend(page.get("Items", []))
        last = page.get("LastEvaluatedKey")
        if not last:
            return items
        kwargs["ExclusiveStartKey"] = last


def _points_from_rollups(items):
    """Expand 1-minute rollup rows into analytics points (avg per bucket)."""
    points = []
    sample_count = 0
    for item in items:
        n = _as_int(item.get("n"), 0)
        sample_count += n
        rssi_n = _as_int(item.get("rssi_n"), 0)
        temp_n = _as_int(item.get("temp_n"), 0)
        rssi_sum = _as_float(item.get("rssi_sum"))
        temp_sum = _as_float(item.get("temp_sum"))
        points.append(
            {
                "ts": _as_int(item.get("bucket_ts"), 0),
                "rssi": (rssi_sum / rssi_n) if rssi_n and rssi_sum is not None else None,
                "chip_temp_c": (temp_sum / temp_n) if temp_n and temp_sum is not None else None,
                "heap_free": _as_float(item.get("heap_last")),
                "heap_min_free": _as_float(item.get("heap_min")),
                "uptime_s": _as_float(item.get("uptime_last")),
                "reset_reason": _as_float(item.get("reset_last")),
                "clock_offset_ms": None,
                "_n": max(1, n),
            }
        )
    points.sort(key=lambda p: p["ts"])
    return points, sample_count


def _bucket_series_from_rollups(points, bucket_s, sample_count_hint=None):
    """Re-bucket rollup points; sample counts use per-point _n when present."""
    buckets = {}
    for p in points:
        key = (p["ts"] // bucket_s) * bucket_s
        acc = buckets.setdefault(key, {"rssi": [], "temp": [], "heap": [], "n": 0})
        acc["n"] += int(p.get("_n") or 1)
        if p["rssi"] is not None:
            acc["rssi"].append(p["rssi"])
        if p["chip_temp_c"] is not None:
            acc["temp"].append(p["chip_temp_c"])
        if p["heap_free"] is not None:
            acc["heap"].append(p["heap_free"])
    series = []
    for key in sorted(buckets):
        acc = buckets[key]
        series.append(
            [
                key,
                _round(_mean(acc["rssi"])),
                _round(_mean(acc["temp"])),
                int(min(acc["heap"])) if acc["heap"] else None,
                acc["n"],
            ]
        )
    return series


def _list_devices():
    """Latest telemetry per known device via GSI (Limit=1) — no full-table scan."""
    device_ids = _device_ids()
    now = int(time.time())
    devices = []

    def load(device_id):
        item = _latest_item(device_id)
        if not item:
            return None
        return _device_from_item(device_id, item, now)

    if device_ids:
        with ThreadPoolExecutor(max_workers=min(8, len(device_ids))) as pool:
            for device in pool.map(load, device_ids):
                if device:
                    devices.append(device)

    devices.sort(key=lambda d: (not d["live"], -(d["last_seen_ts"] or 0), d["device_id"]))

    live = sum(1 for d in devices if d["live"])
    stale = len(devices) - live
    rssi_vals = [d["rssi"] for d in devices if d.get("rssi") is not None]
    temp_vals = [d["chip_temp_c"] for d in devices if d.get("chip_temp_c") is not None]
    heap_vals = [d["heap_free"] for d in devices if d.get("heap_free") is not None]

    summary = {
        "device_count": len(devices),
        "live_count": live,
        "stale_count": stale,
        "avg_rssi": round(sum(rssi_vals) / len(rssi_vals), 1) if rssi_vals else None,
        "avg_chip_temp_c": round(sum(temp_vals) / len(temp_vals), 1) if temp_vals else None,
        "avg_heap_free": int(sum(heap_vals) / len(heap_vals)) if heap_vals else None,
        "stale_after_s": STALE_AFTER_S,
    }
    return devices, summary


def _history_points(items):
    points = []
    for item in reversed(items):  # chronological
        payload = _payload(item)
        points.append(
            {
                "ts": _seen_ts(item),
                "rssi": payload.get("rssi"),
                "chip_temp_c": payload.get("chip_temp_c"),
                "heap_free": payload.get("heap_free"),
                "uptime_s": payload.get("uptime_s"),
            }
        )
    return points


def _query_device_range(table_name: str, device_id: str, since: int, fields=None):
    kwargs = {
        "IndexName": "device_ts_idx",
        "KeyConditionExpression": Key("device_id").eq(device_id)
        & Key("effective_ts").gte(since),
        "ScanIndexForward": True,
    }
    if fields:
        names = {"#p": "payload"}
        paths = ["effective_ts"]
        for i, field in enumerate(fields):
            names[f"#f{i}"] = field
            paths.append(f"#p.#f{i}")
        kwargs["ProjectionExpression"] = ", ".join(paths)
        kwargs["ExpressionAttributeNames"] = names
    table = _table(table_name)
    items = []
    while True:
        page = table.query(**kwargs)
        items.extend(page.get("Items", []))
        last = page.get("LastEvaluatedKey")
        if not last:
            return items
        kwargs["ExclusiveStartKey"] = last


def _percentile(sorted_vals, pct):
    if not sorted_vals:
        return None
    k = (len(sorted_vals) - 1) * pct / 100.0
    lo = int(k)
    hi = min(lo + 1, len(sorted_vals) - 1)
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (k - lo)


def _round(value, digits=1):
    return None if value is None else round(value, digits)


def _mean(vals):
    return sum(vals) / len(vals) if vals else None


def _slope_per_hour(pairs):
    """Least-squares slope of (ts, value) pairs, in value units per hour."""
    if len(pairs) < 2 or pairs[-1][0] - pairs[0][0] < 300:
        return None
    n = len(pairs)
    mx = sum(p[0] for p in pairs) / n
    my = sum(p[1] for p in pairs) / n
    den = sum((p[0] - mx) ** 2 for p in pairs)
    if den == 0:
        return None
    num = sum((p[0] - mx) * (p[1] - my) for p in pairs)
    return num / den * 3600


def _analytics_points(items):
    points = []
    for item in items:
        payload = _payload(item)
        point = {"ts": _as_int(item.get("effective_ts"), 0)}
        for field in ANALYTICS_FIELDS[1:]:
            point[field] = _as_float(payload.get(field))
        points.append(point)
    points.sort(key=lambda p: p["ts"])
    return points


def _device_stats(points, sample_count=None):
    samples = sample_count if sample_count is not None else sum(int(p.get("_n") or 1) for p in points)
    stats = {"samples": samples}
    if not points:
        return stats

    tss = [p["ts"] for p in points]
    gaps = [b - a for a, b in zip(tss, tss[1:])]
    span = max(1, tss[-1] - tss[0])
    expected = span // EXPECTED_INTERVAL_S + 1
    gap_threshold = EXPECTED_INTERVAL_S * 3
    stats.update(
        {
            "first_ts": tss[0],
            "last_ts": tss[-1],
            "delivery_pct": _round(min(100.0, 100.0 * samples / expected)),
            "max_gap_s": max(gaps) if gaps else 0,
            "gap_count": sum(1 for g in gaps if g > gap_threshold),
        }
    )

    reboots = []
    for prev, cur in zip(points, points[1:]):
        if prev["uptime_s"] is not None and cur["uptime_s"] is not None:
            if cur["uptime_s"] < prev["uptime_s"]:
                reason = cur["reset_reason"]
                reboots.append({"ts": cur["ts"], "reset_reason": _as_int(reason, None)})
    stats["reboots"] = reboots

    rssi = sorted(p["rssi"] for p in points if p["rssi"] is not None)
    if rssi:
        rssi_mean = _mean(rssi)
        stats["rssi"] = {
            "min": rssi[0],
            "max": rssi[-1],
            "avg": _round(rssi_mean),
            "p10": _round(_percentile(rssi, 10)),
            "p50": _round(_percentile(rssi, 50)),
            "stddev": _round((sum((v - rssi_mean) ** 2 for v in rssi) / len(rssi)) ** 0.5),
            "weak_pct": _round(100.0 * sum(1 for v in rssi if v < WEAK_RSSI_DBM) / len(rssi)),
        }

    temps = [p["chip_temp_c"] for p in points if p["chip_temp_c"] is not None]
    if temps:
        stats["temp"] = {
            "min": _round(min(temps)),
            "avg": _round(_mean(temps)),
            "max": _round(max(temps)),
            "slope_per_h": _round(
                _slope_per_hour([(p["ts"], p["chip_temp_c"]) for p in points if p["chip_temp_c"] is not None]),
                2,
            ),
        }

    heap_pairs = [(p["ts"], p["heap_free"]) for p in points if p["heap_free"] is not None]
    heap_min = [p["heap_min_free"] for p in points if p["heap_min_free"] is not None]
    if heap_pairs:
        slope = _slope_per_hour(heap_pairs)
        stats["heap"] = {
            "last": int(heap_pairs[-1][1]),
            "min": int(min(v for _, v in heap_pairs)),
            "low_water": int(min(heap_min)) if heap_min else None,
            "slope_bytes_per_h": int(slope) if slope is not None else None,
        }

    offsets = [abs(p["clock_offset_ms"]) for p in points if p["clock_offset_ms"] is not None]
    if offsets:
        stats["clock_offset_ms_max"] = int(max(offsets))
    return stats


def _health_score(stats, live):
    score = 100.0
    if not live:
        score -= 40
    delivery = stats.get("delivery_pct")
    if delivery is not None:
        score -= min(25.0, max(0.0, 100.0 - delivery) * 0.5)
    rssi_avg = (stats.get("rssi") or {}).get("avg")
    if rssi_avg is not None and rssi_avg < -70:
        score -= min(20.0, (-70 - rssi_avg) * 2)
    score -= min(30.0, 10.0 * len(stats.get("reboots") or []))
    heap_slope = (stats.get("heap") or {}).get("slope_bytes_per_h")
    if heap_slope is not None and heap_slope < -2048:
        score -= 15
    temp_max = (stats.get("temp") or {}).get("max")
    if temp_max is not None and temp_max >= HOT_TEMP_C:
        score -= 10
    return max(0, int(round(score)))


def _bucket_series(points, bucket_s):
    buckets = {}
    for p in points:
        key = (p["ts"] // bucket_s) * bucket_s
        acc = buckets.setdefault(key, {"rssi": [], "temp": [], "heap": [], "n": 0})
        acc["n"] += 1
        if p["rssi"] is not None:
            acc["rssi"].append(p["rssi"])
        if p["chip_temp_c"] is not None:
            acc["temp"].append(p["chip_temp_c"])
        if p["heap_free"] is not None:
            acc["heap"].append(p["heap_free"])
    series = []
    for key in sorted(buckets):
        acc = buckets[key]
        series.append(
            [
                key,
                _round(_mean(acc["rssi"])),
                _round(_mean(acc["temp"])),
                int(min(acc["heap"])) if acc["heap"] else None,
                acc["n"],
            ]
        )
    return series


def _parse_window(event):
    raw = (event.get("queryStringParameters") or {}).get("window")
    if raw is None or raw == "":
        return DEFAULT_WINDOW_S
    value = int(raw)
    if value not in WINDOW_BUCKETS:
        raise ValueError(f"window must be one of {sorted(WINDOW_BUCKETS)}")
    return value


def _cache_ttl(window_s: int) -> int:
    return ANALYTICS_CACHE_TTL_BY_WINDOW.get(window_s, ANALYTICS_CACHE_TTL_S)


def _load_device_analytics(device, since: int, bucket_s: int, window_s: int):
    device_id = device["device_id"]
    use_rollup = window_s >= ROLLUP_MIN_WINDOW_S and bool(METRICS_TABLE_NAME)
    points = []
    sample_count = None
    series_fn = _bucket_series

    if use_rollup:
        try:
            rollups = _query_metrics_range(device_id, since)
        except Exception:
            rollups = []
        if rollups:
            points, sample_count = _points_from_rollups(rollups)
            series_fn = lambda pts, bs: _bucket_series_from_rollups(pts, bs)

    if not points:
        telemetry = _query_device_range(
            TELEMETRY_TABLE_NAME, device_id, since, ANALYTICS_FIELDS
        )
        points = _analytics_points(telemetry)
        sample_count = len(points)
        series_fn = _bucket_series

    events = _query_device_range(EVENTS_TABLE_NAME, device_id, since)
    return device, points, sample_count, series_fn, [_payload(e) for e in events]


def _fleet_analytics(window_s: int):
    cached = _analytics_cache.get(window_s)
    now = int(time.time())
    if cached and now - cached[0] < _cache_ttl(window_s):
        return cached[1]

    bucket_s = WINDOW_BUCKETS[window_s]
    since = now - window_s
    devices, summary = _list_devices()

    with ThreadPoolExecutor(max_workers=min(12, max(1, len(devices)))) as pool:
        loaded = list(
            pool.map(
                lambda d: _load_device_analytics(d, since, bucket_s, window_s),
                devices,
            )
        )

    out_devices = []
    all_events = []
    for device, points, sample_count, series_fn, events in loaded:
        stats = _device_stats(points, sample_count=sample_count)
        stats["event_count"] = len(events)
        out_devices.append(
            {
                "device_id": device["device_id"],
                "model": device["model"],
                "live": device["live"],
                "last_seen_ts": device["last_seen_ts"],
                "age_s": device["age_s"],
                "telemetry": device["telemetry"],
                "stats": stats,
                "health": _health_score(stats, device["live"]),
                "series": series_fn(points, bucket_s),
                "camera": _camera_latest(device["device_id"])
                if _looks_like_camera(device["device_id"], device.get("model"))
                else None,
            }
        )
        for e in events:
            all_events.append({**e, "device_id": e.get("device_id") or device["device_id"]})

    all_events.sort(key=lambda e: _as_int(e.get("ts"), 0), reverse=True)

    samples = sum(d["stats"]["samples"] for d in out_devices)
    deliveries = [d["stats"]["delivery_pct"] for d in out_devices if d["stats"].get("delivery_pct") is not None]
    fleet = {
        **summary,
        "samples": samples,
        "reboots": sum(len(d["stats"].get("reboots") or []) for d in out_devices),
        "events": len(all_events),
        "avg_delivery_pct": _round(_mean(deliveries)),
        "min_health": min((d["health"] for d in out_devices), default=None),
    }

    body = {
        "window_s": window_s,
        "bucket_s": bucket_s,
        "since": since,
        "now": now,
        "expected_interval_s": EXPECTED_INTERVAL_S,
        "weak_rssi_dbm": WEAK_RSSI_DBM,
        "hot_temp_c": HOT_TEMP_C,
        "fleet": fleet,
        "devices": out_devices,
        "events": all_events[:MAX_FLEET_EVENTS],
    }
    _analytics_cache[window_s] = (now, body)
    return body


def _warm_analytics():
    """Precompute all windows so UI switches hit warm cache."""
    results = {}
    for window_s in WINDOW_BUCKETS:
        body = _fleet_analytics(window_s)
        results[str(window_s)] = {
            "devices": len(body.get("devices") or []),
            "samples": (body.get("fleet") or {}).get("samples"),
            "cached_until": int(time.time()) + _cache_ttl(window_s),
        }
    return {"warmed": results}


def _parse_path(event):
    raw_path = event.get("rawPath") or event.get("path") or ""
    qs_device = (event.get("pathParameters") or {}).get("deviceId")

    if raw_path.rstrip("/").endswith("/fleet/analytics"):
        return None, "fleet_analytics"

    if raw_path.rstrip("/") == "/devices" or raw_path.endswith("/devices"):
        return None, "devices_list"

    match = re.search(
        r"/devices/([^/]+)/(telemetry/latest|telemetry|events|camera/latest)$",
        raw_path.rstrip("/"),
    )
    if match:
        device_id = match.group(1)
        kind = match.group(2)
        if kind == "telemetry/latest":
            return device_id, "telemetry_latest"
        if kind == "telemetry":
            return device_id, "telemetry_history"
        if kind == "camera/latest":
            return device_id, "camera_latest"
        return device_id, "events_recent"

    if qs_device and "telemetry/latest" in raw_path:
        return qs_device, "telemetry_latest"
    if qs_device and raw_path.rstrip("/").endswith("/telemetry"):
        return qs_device, "telemetry_history"
    if qs_device and "/events" in raw_path:
        return qs_device, "events_recent"
    return qs_device, None


def lambda_handler(event, _context):
    # EventBridge warmer (or manual invoke) — keep all windows cached.
    if isinstance(event, dict) and event.get("warm"):
        return _warm_analytics()

    if event.get("requestContext", {}).get("http", {}).get("method") == "OPTIONS":
        return {"statusCode": 204, "headers": JSON_HEADERS, "body": ""}

    device_id, route = _parse_path(event)
    if route is None:
        return _response(400, {"error": "unsupported route"})

    if route == "devices_list":
        devices, summary = _list_devices()
        return _response(200, {"devices": devices, "summary": summary, "count": len(devices)})

    if route == "fleet_analytics":
        try:
            window_s = _parse_window(event)
        except (TypeError, ValueError) as exc:
            return _response(400, {"error": str(exc)})
        return _response(200, _fleet_analytics(window_s))

    if not device_id:
        return _response(400, {"error": "deviceId path parameter is required"})

    if route == "camera_latest":
        cam = _camera_latest(device_id)
        if not cam:
            return _response(404, {"error": f"No camera frame for device_id={device_id}"})
        return _response(200, {"device_id": device_id, "camera": cam})

    if route == "telemetry_latest":
        items = _query_device_ts(TELEMETRY_TABLE_NAME, device_id, 1)
        if not items:
            return _response(404, {"error": f"No telemetry for device_id={device_id}"})
        item = items[0]
        return _response(
            200,
            {
                "device_id": device_id,
                "telemetry": _payload(item),
                "last_seen_ts": _seen_ts(item),
                "record": item,
            },
        )

    if route == "telemetry_history":
        try:
            limit = _parse_limit(event, DEFAULT_HISTORY_LIMIT, MAX_HISTORY_LIMIT)
        except (TypeError, ValueError) as exc:
            return _response(400, {"error": str(exc)})
        items = _query_device_ts(TELEMETRY_TABLE_NAME, device_id, limit)
        points = _history_points(items)
        return _response(
            200,
            {
                "device_id": device_id,
                "points": points,
                "count": len(points),
                "limit": limit,
            },
        )

    try:
        limit = _parse_limit(event, DEFAULT_EVENTS_LIMIT, MAX_EVENTS_LIMIT)
    except (TypeError, ValueError) as exc:
        return _response(400, {"error": str(exc)})

    items = _query_device_ts(EVENTS_TABLE_NAME, device_id, limit)
    return _response(
        200,
        {
            "device_id": device_id,
            "events": [_payload(item) for item in items],
            "count": len(items),
            "limit": limit,
        },
    )
