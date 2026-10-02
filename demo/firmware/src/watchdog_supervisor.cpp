#include "watchdog_supervisor.h"

#include <esp_system.h>
#include <esp_task_wdt.h>

#include "connectivity_timeout_utils.h"
#include "logger.h"

namespace {
constexpr uint32_t kConnectivityTimeoutMs = 5UL * 60UL * 1000UL;
}  // namespace

void WatchdogSupervisor::begin(uint32_t timeout_sec) {
  timeout_ms_ = timeout_sec * 1000UL;

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = timeout_sec * 1000U;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  esp_task_wdt_reconfigure(&cfg);
#else
  esp_task_wdt_init(timeout_sec, true);
#endif

  esp_task_wdt_add(nullptr);
  last_dual_connected_ms_ = millis();
  initialized_ = true;
}

void WatchdogSupervisor::feed() {
  if (!initialized_) {
    return;
  }
  esp_task_wdt_reset();
}

void WatchdogSupervisor::noteDualConnected() { last_dual_connected_ms_ = millis(); }

void WatchdogSupervisor::checkConnectivityTimeout(bool wifi_ok, bool mqtt_ok) {
  if (wifi_ok && mqtt_ok) {
    noteDualConnected();
    return;
  }

  if (ConnectivityTimeoutUtils::shouldReset(millis(), last_dual_connected_ms_, kConnectivityTimeoutMs)) {
    Logger::log("wdt", "5-minute connectivity timeout reached, restarting");
    delay(50);
    ESP.restart();
  }
}

void WatchdogSupervisor::logResetReason() const {
  const esp_reset_reason_t reason = esp_reset_reason();
  Logger::logf("wdt", "reset_reason=%d", static_cast<int>(reason));
}
