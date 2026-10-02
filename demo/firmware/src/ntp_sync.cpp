#include "ntp_sync.h"

#include <sys/time.h>
#include <time.h>

#include <esp_sntp.h>

#include "logger.h"

namespace {
constexpr uint32_t kSyncTimeoutMs = 30000;
constexpr uint32_t kPollIntervalMs = 5000;

NTPSync* g_active_sync = nullptr;

int32_t computeOffsetMs(const struct timeval* server_time) {
  struct timeval local_time {};
  gettimeofday(&local_time, nullptr);
  const int64_t server_us =
      static_cast<int64_t>(server_time->tv_sec) * 1000000LL + server_time->tv_usec;
  const int64_t local_us =
      static_cast<int64_t>(local_time.tv_sec) * 1000000LL + local_time.tv_usec;
  return static_cast<int32_t>((server_us - local_us) / 1000LL);
}
}  // namespace

extern "C" void sntp_sync_time(struct timeval* tv) {
  const int32_t offset_ms = computeOffsetMs(tv);
  settimeofday(tv, nullptr);
  sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);

  if (g_active_sync != nullptr) {
    g_active_sync->markSynced(offset_ms);
  }
}

void NTPSync::begin(const char* server) {
  server_ = server;
  g_active_sync = this;
  resetSync();
}

void NTPSync::resetSync() {
  started_ = false;
  synchronized_ = false;
  timeout_logged_ = false;
  clock_offset_ms_ = 0;
  start_ms_ = 0;
  last_poll_ms_ = 0;
}

void NTPSync::markSynced(int32_t offset_ms) {
  synchronized_ = true;
  clock_offset_ms_ = offset_ms;

  struct tm timeinfo {};
  if (getLocalTime(&timeinfo, 10)) {
    char iso8601[32];
    strftime(iso8601, sizeof(iso8601), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    Logger::logf("ntp", "synced utc=%s offset_ms=%ld", iso8601, static_cast<long>(offset_ms));
  }
}

bool NTPSync::loop() {
  const uint32_t now = millis();
  if (synchronized_) {
    return true;
  }

  if (!started_) {
    configTime(0, 0, server_);
    start_ms_ = now;
    last_poll_ms_ = now - kPollIntervalMs;
    started_ = true;
    Logger::logf("ntp", "sync started server=%s", server_);
  }

  if ((now - last_poll_ms_) >= kPollIntervalMs) {
    last_poll_ms_ = now;
    struct tm timeinfo {};
    if (getLocalTime(&timeinfo, 10)) {
      if (!synchronized_) {
        markSynced(clock_offset_ms_);
      }
      return true;
    }
  }

  if (!timeout_logged_ && (now - start_ms_) >= kSyncTimeoutMs) {
    timeout_logged_ = true;
    Logger::log("ntp", "sync timeout after 30s, ts will fallback to 0");
  }
  return false;
}

bool NTPSync::isSynced() const { return synchronized_; }

uint32_t NTPSync::getEpoch() const {
  if (!synchronized_) {
    return 0;
  }
  return static_cast<uint32_t>(time(nullptr));
}

int32_t NTPSync::getClockOffsetMs() const { return synchronized_ ? clock_offset_ms_ : 0; }
