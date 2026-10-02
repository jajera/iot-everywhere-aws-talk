#!/usr/bin/env bash
# Minimal cloud path: DynamoDB + ingest Lambda + IoT rules + query Lambda Function URL.
# No Terraform. Idempotent-ish for a single talk prefix.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
REGION="${AWS_REGION:-ap-southeast-2}"
PREFIX="${PREFIX:-iot-talk}"
PYTHON_RUNTIME="${PYTHON_RUNTIME:-python3.14}"
ACCOUNT_ID="$(aws sts get-caller-identity --query Account --output text)"

TELEMETRY_TABLE="${PREFIX}-telemetry"
EVENTS_TABLE="${PREFIX}-events"
METRICS_TABLE="${PREFIX}-metrics"
CAMERA_BUCKET="${PREFIX}-camera-${ACCOUNT_ID}"
INGEST_FN="${PREFIX}-ingest"
QUERY_FN="${PREFIX}-query"
ROLE_NAME="${PREFIX}-lambda-role"
WARMER_RULE="${PREFIX}-analytics-warmer"
RULE_TELEMETRY="${PREFIX//-/_}_telemetry"
RULE_EVENTS="${PREFIX//-/_}_events"
RULE_FLEET_TELEMETRY="${PREFIX//-/_}_fleet_telemetry"
RULE_FLEET_EVENTS="${PREFIX//-/_}_fleet_events"
RULE_FLEET_CAMERA="${PREFIX//-/_}_fleet_camera"

echo "Region=$REGION Prefix=$PREFIX Runtime=$PYTHON_RUNTIME"

create_table() {
  local name="$1"
  if aws dynamodb describe-table --table-name "$name" --region "$REGION" >/dev/null 2>&1; then
    echo "Table $name exists"
    return
  fi
  aws dynamodb create-table \
    --table-name "$name" \
    --attribute-definitions \
      AttributeName=device_id,AttributeType=S \
      AttributeName=record_id,AttributeType=S \
      AttributeName=effective_ts,AttributeType=N \
    --key-schema \
      AttributeName=device_id,KeyType=HASH \
      AttributeName=record_id,KeyType=RANGE \
    --billing-mode PAY_PER_REQUEST \
    --global-secondary-indexes "[
      {
        \"IndexName\": \"device_ts_idx\",
        \"KeySchema\": [
          {\"AttributeName\": \"device_id\", \"KeyType\": \"HASH\"},
          {\"AttributeName\": \"effective_ts\", \"KeyType\": \"RANGE\"}
        ],
        \"Projection\": {\"ProjectionType\": \"ALL\"}
      }
    ]" \
    --region "$REGION" >/dev/null
  echo "Creating table $name…"
  aws dynamodb wait table-exists --table-name "$name" --region "$REGION"
}

create_table "$TELEMETRY_TABLE"
create_table "$EVENTS_TABLE"

create_metrics_table() {
  local name="$1"
  if aws dynamodb describe-table --table-name "$name" --region "$REGION" >/dev/null 2>&1; then
    echo "Table $name exists"
    return
  fi
  aws dynamodb create-table \
    --table-name "$name" \
    --attribute-definitions \
      AttributeName=device_id,AttributeType=S \
      AttributeName=bucket_ts,AttributeType=N \
    --key-schema \
      AttributeName=device_id,KeyType=HASH \
      AttributeName=bucket_ts,KeyType=RANGE \
    --billing-mode PAY_PER_REQUEST \
    --region "$REGION" >/dev/null
  echo "Creating table $name…"
  aws dynamodb wait table-exists --table-name "$name" --region "$REGION"
}

create_metrics_table "$METRICS_TABLE"

# Camera latest JPEG bucket (public read via presigned URLs only)
if aws s3api head-bucket --bucket "$CAMERA_BUCKET" --region "$REGION" 2>/dev/null; then
  echo "Bucket $CAMERA_BUCKET exists"
else
  aws s3api create-bucket \
    --bucket "$CAMERA_BUCKET" \
    --region "$REGION" \
    --create-bucket-configuration LocationConstraint="$REGION" >/dev/null
  echo "Created bucket $CAMERA_BUCKET"
fi
aws s3api put-public-access-block \
  --bucket "$CAMERA_BUCKET" \
  --public-access-block-configuration \
  BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true \
  --region "$REGION" >/dev/null || true
aws s3api put-bucket-cors --bucket "$CAMERA_BUCKET" --cors-configuration '{
  "CORSRules": [{
    "AllowedOrigins": ["*"],
    "AllowedMethods": ["GET", "HEAD"],
    "AllowedHeaders": ["*"],
    "MaxAgeSeconds": 3000
  }]
}' --region "$REGION" >/dev/null
mkdir -p "$ROOT/out"
echo "$CAMERA_BUCKET" >"$ROOT/out/camera-bucket.txt"

# IAM role for both Lambdas
TRUST='{"Version":"2012-10-17","Statement":[{"Effect":"Allow","Principal":{"Service":"lambda.amazonaws.com"},"Action":"sts:AssumeRole"}]}'
ROLE_ARN="arn:aws:iam::${ACCOUNT_ID}:role/${ROLE_NAME}"
if ! aws iam get-role --role-name "$ROLE_NAME" >/dev/null 2>&1; then
  aws iam create-role --role-name "$ROLE_NAME" --assume-role-policy-document "$TRUST" >/dev/null
  echo "Created role $ROLE_NAME"
  sleep 8
else
  echo "Role $ROLE_NAME exists"
fi

aws iam attach-role-policy \
  --role-name "$ROLE_NAME" \
  --policy-arn arn:aws:iam::aws:policy/service-role/AWSLambdaBasicExecutionRole >/dev/null || true

INLINE=$(cat <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "dynamodb:PutItem",
        "dynamodb:UpdateItem",
        "dynamodb:Query",
        "dynamodb:GetItem",
        "dynamodb:Scan",
        "dynamodb:BatchWriteItem"
      ],
      "Resource": [
        "arn:aws:dynamodb:${REGION}:${ACCOUNT_ID}:table/${TELEMETRY_TABLE}",
        "arn:aws:dynamodb:${REGION}:${ACCOUNT_ID}:table/${TELEMETRY_TABLE}/index/*",
        "arn:aws:dynamodb:${REGION}:${ACCOUNT_ID}:table/${EVENTS_TABLE}",
        "arn:aws:dynamodb:${REGION}:${ACCOUNT_ID}:table/${EVENTS_TABLE}/index/*",
        "arn:aws:dynamodb:${REGION}:${ACCOUNT_ID}:table/${METRICS_TABLE}"
      ]
    },
    {
      "Effect": "Allow",
      "Action": ["iot:ListThings"],
      "Resource": "*"
    },
    {
      "Effect": "Allow",
      "Action": ["s3:PutObject", "s3:GetObject", "s3:HeadObject"],
      "Resource": "arn:aws:s3:::${CAMERA_BUCKET}/cameras/*"
    }
  ]
}
EOF
)
aws iam put-role-policy --role-name "$ROLE_NAME" --policy-name "${PREFIX}-ddb" --policy-document "$INLINE" >/dev/null

pack_lambda() {
  local src_dir="$1"
  local zip_path="$2"
  rm -f "$zip_path"
  (cd "$src_dir" && zip -q -r "$zip_path" handler.py)
}

INGEST_ZIP="$ROOT/out/ingest.zip"
QUERY_ZIP="$ROOT/out/query.zip"
mkdir -p "$ROOT/out"
pack_lambda "$ROOT/ingest" "$INGEST_ZIP"
pack_lambda "$ROOT/query" "$QUERY_ZIP"

ENV_VARS="Variables={TELEMETRY_TABLE_NAME=${TELEMETRY_TABLE},EVENTS_TABLE_NAME=${EVENTS_TABLE},METRICS_TABLE_NAME=${METRICS_TABLE},CAMERA_BUCKET=${CAMERA_BUCKET},DEVICE_IDS=esp32-c61-01;ideaspark-oled-01;ideaspark-oled-02;esp32-s3-01;esp32-c3-01;esp32-cam-01}"
TAGS_CSV="Project=iot-everywhere-aws-talk,Component=demo,ManagedBy=demo-aws-deploy-stack,Prefix=${PREFIX}"

deploy_fn() {
  local name="$1"
  local zip="$2"
  local description="$3"
  local memory="${4:-256}"
  local timeout="${5:-30}"
  aws lambda wait function-updated --function-name "$name" --region "$REGION" 2>/dev/null || true
  if aws lambda get-function --function-name "$name" --region "$REGION" >/dev/null 2>&1; then
    aws lambda update-function-code --function-name "$name" --zip-file "fileb://$zip" --region "$REGION" >/dev/null
    aws lambda wait function-updated --function-name "$name" --region "$REGION"
    aws lambda update-function-configuration \
      --function-name "$name" \
      --description "$description" \
      --runtime "$PYTHON_RUNTIME" \
      --handler handler.lambda_handler \
      --timeout "$timeout" \
      --memory-size "$memory" \
      --environment "$ENV_VARS" \
      --region "$REGION" >/dev/null
    aws lambda wait function-updated --function-name "$name" --region "$REGION"
    local fn_arn
    fn_arn="$(aws lambda get-function --function-name "$name" --region "$REGION" --query Configuration.FunctionArn --output text)"
    aws lambda tag-resource --resource "$fn_arn" --tags "$TAGS_CSV" --region "$REGION" >/dev/null
    echo "Updated Lambda $name"
  else
    aws lambda create-function \
      --function-name "$name" \
      --description "$description" \
      --runtime "$PYTHON_RUNTIME" \
      --role "$ROLE_ARN" \
      --handler handler.lambda_handler \
      --zip-file "fileb://$zip" \
      --timeout "$timeout" \
      --memory-size "$memory" \
      --environment "$ENV_VARS" \
      --tags "$TAGS_CSV" \
      --region "$REGION" >/dev/null
    aws lambda wait function-active --function-name "$name" --region "$REGION"
    echo "Created Lambda $name"
  fi
}

deploy_fn "$INGEST_FN" "$INGEST_ZIP" \
  "Talk demo: ingest IoT telemetry/events into DynamoDB (IoT rule target)." 256 30
deploy_fn "$QUERY_FN" "$QUERY_ZIP" \
  "Talk demo: HTTP query API for latest telemetry and recent events (Amplify dashboard)." 1024 45

INGEST_ARN="$(aws lambda get-function --function-name "$INGEST_FN" --region "$REGION" --query Configuration.FunctionArn --output text)"

# Allow IoT to invoke ingest (devices/ + fleet/)
for stmt_id_arn in \
  "${PREFIX}-iot-invoke|${RULE_TELEMETRY}" \
  "${PREFIX}-iot-invoke-events|${RULE_EVENTS}" \
  "${PREFIX}-iot-invoke-fleet-telemetry|${RULE_FLEET_TELEMETRY}" \
  "${PREFIX}-iot-invoke-fleet-events|${RULE_FLEET_EVENTS}" \
  "${PREFIX}-iot-invoke-fleet-camera|${RULE_FLEET_CAMERA}"
do
  stmt_id="${stmt_id_arn%%|*}"
  rule_name="${stmt_id_arn##*|}"
  aws lambda add-permission \
    --function-name "$INGEST_FN" \
    --statement-id "$stmt_id" \
    --action lambda:InvokeFunction \
    --principal iot.amazonaws.com \
    --source-arn "arn:aws:iot:${REGION}:${ACCOUNT_ID}:rule/${rule_name}" \
    --region "$REGION" >/dev/null 2>&1 || true
done

upsert_rule() {
  local name="$1"
  local sql="$2"
  local payload
  payload=$(cat <<EOF
{
  "sql": "${sql}",
  "actions": [{"lambda": {"functionArn": "${INGEST_ARN}"}}],
  "ruleDisabled": false,
  "awsIotSqlVersion": "2016-03-23"
}
EOF
)
  if aws iot get-topic-rule --rule-name "$name" --region "$REGION" >/dev/null 2>&1; then
    aws iot replace-topic-rule --rule-name "$name" --topic-rule-payload "$payload" --region "$REGION"
    echo "Updated rule $name"
  else
    aws iot create-topic-rule --rule-name "$name" --topic-rule-payload "$payload" --region "$REGION"
    echo "Created rule $name"
  fi
}

upsert_rule "$RULE_TELEMETRY" "SELECT * FROM 'devices/+/telemetry'"
upsert_rule "$RULE_EVENTS" "SELECT * FROM 'devices/+/events'"
upsert_rule "$RULE_FLEET_TELEMETRY" "SELECT * FROM 'fleet/+/telemetry'"
upsert_rule "$RULE_FLEET_EVENTS" "SELECT * FROM 'fleet/+/events'"
upsert_rule "$RULE_FLEET_CAMERA" "SELECT * FROM 'fleet/+/camera'"

# Query Function URL (CORS open for Amplify)
if ! aws lambda get-function-url-config --function-name "$QUERY_FN" --region "$REGION" >/dev/null 2>&1; then
  aws lambda create-function-url-config \
    --function-name "$QUERY_FN" \
    --auth-type NONE \
    --cors '{"AllowOrigins":["*"],"AllowMethods":["*"],"AllowHeaders":["*"]}' \
    --region "$REGION" >/dev/null
fi

aws lambda add-permission \
  --function-name "$QUERY_FN" \
  --statement-id "${PREFIX}-url-public" \
  --action lambda:InvokeFunctionUrl \
  --principal "*" \
  --function-url-auth-type NONE \
  --region "$REGION" >/dev/null 2>&1 || true

# AuthType NONE also needs InvokeFunction for public Function URL access
aws lambda add-permission \
  --function-name "$QUERY_FN" \
  --statement-id "${PREFIX}-url-invoke" \
  --action lambda:InvokeFunction \
  --principal "*" \
  --region "$REGION" >/dev/null 2>&1 || true

QUERY_URL="$(aws lambda get-function-url-config --function-name "$QUERY_FN" --region "$REGION" --query FunctionUrl --output text | sed 's:/*$::')"
echo "$QUERY_URL" >"$ROOT/out/query-url.txt"
QUERY_ARN="$(aws lambda get-function --function-name "$QUERY_FN" --region "$REGION" --query Configuration.FunctionArn --output text)"

# Keep analytics caches warm so 1h/6h/24h switches stay snappy.
if aws events describe-rule --name "$WARMER_RULE" --region "$REGION" >/dev/null 2>&1; then
  aws events put-rule \
    --name "$WARMER_RULE" \
    --schedule-expression "rate(1 minute)" \
    --state ENABLED \
    --description "Warm iot-talk fleet analytics caches (1h/6h/24h)" \
    --region "$REGION" >/dev/null
  echo "Updated EventBridge rule $WARMER_RULE"
else
  aws events put-rule \
    --name "$WARMER_RULE" \
    --schedule-expression "rate(1 minute)" \
    --state ENABLED \
    --description "Warm iot-talk fleet analytics caches (1h/6h/24h)" \
    --region "$REGION" >/dev/null
  echo "Created EventBridge rule $WARMER_RULE"
fi
aws lambda add-permission \
  --function-name "$QUERY_FN" \
  --statement-id "${PREFIX}-analytics-warmer" \
  --action lambda:InvokeFunction \
  --principal events.amazonaws.com \
  --source-arn "arn:aws:events:${REGION}:${ACCOUNT_ID}:rule/${WARMER_RULE}" \
  --region "$REGION" >/dev/null 2>&1 || true
aws events put-targets \
  --rule "$WARMER_RULE" \
  --cli-input-json "$(python3 -c "import json; print(json.dumps({'Rule': '''$WARMER_RULE''', 'Targets': [{'Id': '1', 'Arn': '''$QUERY_ARN''', 'Input': '{\"warm\":true}'}]}))")" \
  --region "$REGION" >/dev/null

echo "Backfilling metrics rollups (last 24h)…"
export AWS_REGION="$REGION" PREFIX="$PREFIX"
python3 - <<'PY' || true
import os, time
from decimal import Decimal
import boto3
from boto3.dynamodb.conditions import Key

region = os.environ.get("AWS_REGION", "ap-southeast-2")
prefix = os.environ.get("PREFIX", "iot-talk")
telemetry = f"{prefix}-telemetry"
metrics = f"{prefix}-metrics"
device_ids = "esp32-c61-01;ideaspark-oled-01;ideaspark-oled-02;esp32-s3-01;esp32-c3-01;esp32-cam-01".split(";")
bucket_s = 60
since = int(time.time()) - 86400
ddb = boto3.resource("dynamodb", region_name=region)
src = ddb.Table(telemetry)
dst = ddb.Table(metrics)

def as_float(v):
    if v is None: return None
    try: return float(v)
    except Exception: return None

def query_range(device_id):
    kwargs = {
        "IndexName": "device_ts_idx",
        "KeyConditionExpression": Key("device_id").eq(device_id) & Key("effective_ts").gte(since),
        "ScanIndexForward": True,
        "ProjectionExpression": "effective_ts, payload",
    }
    items = []
    while True:
        page = src.query(**kwargs)
        items.extend(page.get("Items", []))
        lek = page.get("LastEvaluatedKey")
        if not lek: return items
        kwargs["ExclusiveStartKey"] = lek

total = 0
for device_id in device_ids:
    buckets = {}
    for item in query_range(device_id):
        ts = int(item.get("effective_ts") or 0)
        if ts <= 0: continue
        p = item.get("payload") or {}
        key = (ts // bucket_s) * bucket_s
        acc = buckets.setdefault(key, {"n": 0, "rssi_sum": 0.0, "rssi_n": 0, "temp_sum": 0.0, "temp_n": 0,
                                       "heap_min": None, "heap_last": None, "uptime_last": None, "reset_last": None})
        acc["n"] += 1
        r = as_float(p.get("rssi"))
        if r is not None:
            acc["rssi_sum"] += r; acc["rssi_n"] += 1
        t = as_float(p.get("chip_temp_c"))
        if t is not None:
            acc["temp_sum"] += t; acc["temp_n"] += 1
        h = p.get("heap_free")
        if h is not None:
            try:
                hi = int(h)
                acc["heap_last"] = hi
                acc["heap_min"] = hi if acc["heap_min"] is None else min(acc["heap_min"], hi)
            except Exception:
                pass
        u = p.get("uptime_s")
        if u is not None:
            try: acc["uptime_last"] = int(u)
            except Exception: pass
        rr = p.get("reset_reason")
        if rr is not None:
            try: acc["reset_last"] = int(rr)
            except Exception: pass
    with dst.batch_writer() as batch:
        for bts, acc in buckets.items():
            item = {
                "device_id": device_id,
                "bucket_ts": bts,
                "n": acc["n"],
                "updated_at": int(time.time()),
            }
            if acc["rssi_n"]:
                item["rssi_sum"] = Decimal(str(round(acc["rssi_sum"], 3)))
                item["rssi_n"] = acc["rssi_n"]
            if acc["temp_n"]:
                item["temp_sum"] = Decimal(str(round(acc["temp_sum"], 3)))
                item["temp_n"] = acc["temp_n"]
            if acc["heap_last"] is not None:
                item["heap_last"] = acc["heap_last"]
            if acc["heap_min"] is not None:
                item["heap_min"] = acc["heap_min"]
            if acc["uptime_last"] is not None:
                item["uptime_last"] = acc["uptime_last"]
            if acc["reset_last"] is not None:
                item["reset_last"] = acc["reset_last"]
            batch.put_item(Item=item)
            total += 1
    print(f"  {device_id}: {len(buckets)} buckets")
print(f"Backfill wrote {total} metric buckets")
PY

# Prime warm cache immediately
aws lambda invoke \
  --function-name "$QUERY_FN" \
  --cli-binary-format raw-in-base64-out \
  --payload '{"warm":true}' \
  --region "$REGION" \
  "$ROOT/out/warm-analytics.json" >/dev/null || true


echo
echo "=== Validation ==="
FAIL=0

ok() { echo "OK  $1"; }
bad() { echo "FAIL $1"; FAIL=1; }

status="$(aws dynamodb describe-table --table-name "$TELEMETRY_TABLE" --region "$REGION" --query 'Table.TableStatus' --output text)"
[[ "$status" == "ACTIVE" ]] && ok "table $TELEMETRY_TABLE ACTIVE" || bad "table $TELEMETRY_TABLE ACTIVE (got $status)"

status="$(aws dynamodb describe-table --table-name "$EVENTS_TABLE" --region "$REGION" --query 'Table.TableStatus' --output text)"
[[ "$status" == "ACTIVE" ]] && ok "table $EVENTS_TABLE ACTIVE" || bad "table $EVENTS_TABLE ACTIVE (got $status)"

status="$(aws dynamodb describe-table --table-name "$METRICS_TABLE" --region "$REGION" --query 'Table.TableStatus' --output text)"
[[ "$status" == "ACTIVE" ]] && ok "table $METRICS_TABLE ACTIVE" || bad "table $METRICS_TABLE ACTIVE (got $status)"

gsi="$(aws dynamodb describe-table --table-name "$TELEMETRY_TABLE" --region "$REGION" --query "Table.GlobalSecondaryIndexes[?IndexName=='device_ts_idx'].IndexStatus | [0]" --output text)"
[[ "$gsi" == "ACTIVE" ]] && ok "GSI device_ts_idx on $TELEMETRY_TABLE" || bad "GSI device_ts_idx on $TELEMETRY_TABLE (got $gsi)"

gsi="$(aws dynamodb describe-table --table-name "$EVENTS_TABLE" --region "$REGION" --query "Table.GlobalSecondaryIndexes[?IndexName=='device_ts_idx'].IndexStatus | [0]" --output text)"
[[ "$gsi" == "ACTIVE" ]] && ok "GSI device_ts_idx on $EVENTS_TABLE" || bad "GSI device_ts_idx on $EVENTS_TABLE (got $gsi)"

rt="$(aws lambda get-function-configuration --function-name "$INGEST_FN" --region "$REGION" --query Runtime --output text)"
[[ "$rt" == "$PYTHON_RUNTIME" ]] && ok "Lambda $INGEST_FN runtime $PYTHON_RUNTIME" || bad "Lambda $INGEST_FN runtime (got $rt)"

rt="$(aws lambda get-function-configuration --function-name "$QUERY_FN" --region "$REGION" --query Runtime --output text)"
[[ "$rt" == "$PYTHON_RUNTIME" ]] && ok "Lambda $QUERY_FN runtime $PYTHON_RUNTIME" || bad "Lambda $QUERY_FN runtime (got $rt)"

for fn in "$INGEST_FN" "$QUERY_FN"; do
  desc="$(aws lambda get-function-configuration --function-name "$fn" --region "$REGION" --query Description --output text)"
  [[ -n "$desc" && "$desc" != "None" ]] && ok "Lambda $fn description set" || bad "Lambda $fn description missing"
  fn_arn="$(aws lambda get-function --function-name "$fn" --region "$REGION" --query Configuration.FunctionArn --output text)"
  tags="$(aws lambda list-tags --resource "$fn_arn" --region "$REGION" --query 'Tags.Project' --output text)"
  [[ "$tags" == "iot-everywhere-aws-talk" ]] && ok "Lambda $fn tags (Project)" || bad "Lambda $fn tags (Project got $tags)"
done

if aws iot get-topic-rule --rule-name "$RULE_TELEMETRY" --region "$REGION" >/dev/null 2>&1; then
  ok "IoT rule $RULE_TELEMETRY"
else
  bad "IoT rule $RULE_TELEMETRY"
fi

if aws iot get-topic-rule --rule-name "$RULE_EVENTS" --region "$REGION" >/dev/null 2>&1; then
  ok "IoT rule $RULE_EVENTS"
else
  bad "IoT rule $RULE_EVENTS"
fi

if aws iot get-topic-rule --rule-name "$RULE_FLEET_TELEMETRY" --region "$REGION" >/dev/null 2>&1; then
  ok "IoT rule $RULE_FLEET_TELEMETRY"
else
  bad "IoT rule $RULE_FLEET_TELEMETRY"
fi

if aws iot get-topic-rule --rule-name "$RULE_FLEET_EVENTS" --region "$REGION" >/dev/null 2>&1; then
  ok "IoT rule $RULE_FLEET_EVENTS"
else
  bad "IoT rule $RULE_FLEET_EVENTS"
fi

code="$(curl -sS -o /dev/null -w '%{http_code}' "$QUERY_URL/devices" || true)"
[[ "$code" == "200" ]] && ok "query devices list ($code)" || bad "query devices list (got $code)"

code="$(curl -sS -o /dev/null -w '%{http_code}' "$QUERY_URL/fleet/analytics?window=3600" || true)"
[[ "$code" == "200" ]] && ok "query fleet analytics ($code)" || bad "query fleet analytics (got $code)"

code="$(curl -sS -o /dev/null -w '%{http_code}' -X OPTIONS "$QUERY_URL/devices/esp32-c61-01/telemetry/latest" || true)"
[[ "$code" == "204" || "$code" == "200" ]] && ok "query Function URL OPTIONS ($code)" || bad "query Function URL OPTIONS (got $code)"

# Function URL CORS + handler ACAO together → duplicate header → browser CORS fail
acao_count="$(
  curl -sS -D - -o /dev/null -H "Origin: https://example.com" \
    "$QUERY_URL/devices/esp32-c61-01/telemetry/latest" \
    | grep -ci '^access-control-allow-origin:' || true
)"
[[ "$acao_count" == "1" ]] && ok "query CORS single ACAO header" || bad "query CORS ACAO count (got $acao_count, want 1)"

code="$(curl -sS -o /dev/null -w '%{http_code}' "$QUERY_URL/devices/esp32-c61-01/telemetry/latest" || true)"
[[ "$code" == "404" || "$code" == "200" ]] && ok "query telemetry route ($code)" || bad "query telemetry route (got $code)"

# Smoke: invoke ingest with a sample payload, then query should return 200
SMOKE_DEVICE="${THING_NAME:-esp32-c61-01}"
SMOKE_PAYLOAD=$(cat <<EOF
{"device_id":"${SMOKE_DEVICE}","ts":0,"type":"connectivity","rssi":-50,"uptime_s":1,"heap_free":100000,"chip_temp_c":40.0,"chip_model":"smoke","wifi_ssid":"smoke"}
EOF
)
aws lambda invoke \
  --function-name "$INGEST_FN" \
  --cli-binary-format raw-in-base64-out \
  --payload "$SMOKE_PAYLOAD" \
  --region "$REGION" \
  "$ROOT/out/ingest-smoke.json" >/dev/null
smoke_code="$(python3 -c "import json; print(json.load(open('$ROOT/out/ingest-smoke.json')).get('statusCode',0))")"
[[ "$smoke_code" == "200" ]] && ok "ingest smoke invoke" || bad "ingest smoke invoke (got $smoke_code)"

sleep 2
code="$(curl -sS -o /dev/null -w '%{http_code}' "$QUERY_URL/devices/${SMOKE_DEVICE}/telemetry/latest" || true)"
[[ "$code" == "200" ]] && ok "query returns smoke telemetry for $SMOKE_DEVICE" || bad "query returns smoke telemetry (got $code)"

echo
if [[ "$FAIL" -ne 0 ]]; then
  echo "Validation FAILED — fix issues before stage 3."
  exit 1
fi

echo "Validation PASSED"
echo
echo "Query API base URL:"
echo "  $QUERY_URL"
echo
echo "Dashboard routes:"
echo "  GET $QUERY_URL/fleet/analytics?window=3600   (3600 | 21600 | 86400)"
echo "  GET $QUERY_URL/devices"
echo "  GET $QUERY_URL/devices/{Thing}/telemetry/latest"
echo "  GET $QUERY_URL/devices/{Thing}/telemetry?limit=60"
echo "  GET $QUERY_URL/devices/{Thing}/events?limit=10"
echo
echo "Next: stage 3 firmware (see demo/aws/README.md)"
