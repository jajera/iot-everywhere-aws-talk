#include "wifi_manager.h"

#include "backoff_utils.h"
#include "logger.h"

namespace {
constexpr uint32_t kInitialBackoffMs = 1000;
constexpr uint32_t kMaxBackoffMs = 60000;
constexpr uint32_t kBootTimeoutMs = 30000;
}  // namespace

void WiFiManager::begin(const char* ssid, const char* password) {
  ssid_ = ssid;
  password_ = password;
  boot_start_ms_ = millis();
  last_attempt_ms_ = 0;
  backoff_ms_ = kInitialBackoffMs;
  boot_timeout_logged_ = false;
  state_ = State::kDisconnected;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  // Schedule first connect attempt immediately; backoff applies from second attempt onward.
  last_attempt_ms_ = millis() - kInitialBackoffMs;
}

bool WiFiManager::loop() {
  const wl_status_t status = WiFi.status();
  const uint32_t now = millis();

  if (status == WL_CONNECTED) {
    if (state_ != State::kConnected) {
      state_ = State::kConnected;
      backoff_ms_ = kInitialBackoffMs;
      Logger::logf("wifi", "connected ip=%s", WiFi.localIP().toString().c_str());
    }
    return true;
  }

  if (!boot_timeout_logged_ && (now - boot_start_ms_) >= kBootTimeoutMs) {
    Logger::log("wifi", "failed to connect within 30s; continuing retries");
    boot_timeout_logged_ = true;
  }

  if (state_ == State::kConnected) {
    Logger::log("wifi", "disconnected");
    state_ = State::kDisconnected;
  }

  if ((now - last_attempt_ms_) >= backoff_ms_) {
    Logger::logf("wifi", "attempting connect (backoff=%lums)", backoff_ms_);
    WiFi.begin(ssid_, password_);
    last_attempt_ms_ = now;
    state_ = State::kConnecting;
    backoff_ms_ = BackoffUtils::next(backoff_ms_, kInitialBackoffMs, kMaxBackoffMs);
  }

  return false;
}

bool WiFiManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }

int8_t WiFiManager::getRSSI() const { return isConnected() ? static_cast<int8_t>(WiFi.RSSI()) : -127; }

String WiFiManager::getIP() const { return isConnected() ? WiFi.localIP().toString() : String("0.0.0.0"); }
