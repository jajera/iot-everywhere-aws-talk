#include "status_led.h"

#include "logger.h"

#ifdef STATUS_LED_HAS_RGB

#include "soc/soc_caps.h"

#if !SOC_RMT_SUPPORTED
#include "esp_err.h"
#include "led_strip.h"
#endif

namespace {

constexpr uint32_t kFlashMs = 800;
constexpr uint8_t kBlueLevel = 120;
constexpr uint8_t kBootLevel = 255;
constexpr uint32_t kBootHoldMs = 2000;

int rgbGpio() {
#ifdef PIN_RGB_LED
  return PIN_RGB_LED;
#elif defined(RGB_BUILTIN)
  return RGB_BUILTIN - SOC_GPIO_PIN_COUNT;
#else
  return -1;
#endif
}

#if SOC_RMT_SUPPORTED

void writeRgb(uint8_t red, uint8_t green, uint8_t blue) {
#ifdef RGB_BUILTIN
  rgbLedWrite(RGB_BUILTIN, red, green, blue);
#elif defined(PIN_RGB_LED)
  rgbLedWrite(PIN_RGB_LED, red, green, blue);
#endif
}

bool initRgbBackend() {
  writeRgb(0, 0, 0);
  Logger::logf("led", "rgb ready (rgbLedWrite gpio=%d)", rgbGpio());
  return true;
}

#else  // !SOC_RMT_SUPPORTED — SPI led_strip (ESP-IDF component)

led_strip_handle_t g_strip = nullptr;

void writeRgb(uint8_t red, uint8_t green, uint8_t blue) {
  if (g_strip == nullptr) {
    return;
  }
  if (red == 0 && green == 0 && blue == 0) {
    esp_err_t err = led_strip_clear(g_strip);
    if (err != ESP_OK) {
      Logger::logf("led", "clear failed 0x%x", static_cast<unsigned>(err));
    }
    return;
  }
  esp_err_t err = led_strip_set_pixel(g_strip, 0, red, green, blue);
  if (err != ESP_OK) {
    Logger::logf("led", "set_pixel failed 0x%x", static_cast<unsigned>(err));
    return;
  }
  err = led_strip_refresh(g_strip);
  if (err != ESP_OK) {
    Logger::logf("led", "refresh failed 0x%x", static_cast<unsigned>(err));
  }
}

bool initRgbBackend() {
  const int gpio = rgbGpio();
  if (gpio < 0) {
    Logger::log("led", "no rgb gpio defined");
    return false;
  }

  led_strip_config_t strip_config = {
      .strip_gpio_num = gpio,
      .max_leds = 1,
      .led_model = LED_MODEL_WS2812,
      .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
      .flags = {.invert_out = false},
  };
  // C61 has no RMT; SPI must use DMA so USB/Wi-Fi IRQs cannot insert >50µs gaps
  // mid-frame (those look like WS2812 reset and drop early G/R bytes → "blue only").
  led_strip_spi_config_t spi_config = {
      .clk_src = SPI_CLK_SRC_DEFAULT,
      .spi_bus = SPI2_HOST,
      .flags = {.with_dma = true},
  };

  esp_err_t err = led_strip_new_spi_device(&strip_config, &spi_config, &g_strip);
  if (err != ESP_OK || g_strip == nullptr) {
    Logger::logf("led", "led_strip SPI+DMA failed err=0x%x gpio=%d — trying no-dma",
                 static_cast<unsigned>(err), gpio);
    spi_config.flags.with_dma = false;
    err = led_strip_new_spi_device(&strip_config, &spi_config, &g_strip);
  }
  if (err != ESP_OK || g_strip == nullptr) {
    Logger::logf("led", "led_strip SPI init failed err=0x%x gpio=%d", static_cast<unsigned>(err),
                 gpio);
    g_strip = nullptr;
    return false;
  }

  // WS2812B-V5 needs ~280µs idle-low reset before the first frame
  delayMicroseconds(300);
  led_strip_clear(g_strip);
  delayMicroseconds(300);
  Logger::logf("led", "rgb ready (led_strip SPI v3 gpio=%d, dma=%d)", gpio,
               static_cast<int>(spi_config.flags.with_dma));

  // White first proves all three dies; then R/G/B (not the hardwired USB red LED)
  Logger::log("led", "boot WHITE 2s — addressable LED near GPIO8");
  writeRgb(kBootLevel, kBootLevel, kBootLevel);
  delay(kBootHoldMs);
  Logger::log("led", "boot RED 2s");
  writeRgb(kBootLevel, 0, 0);
  delay(kBootHoldMs);
  Logger::log("led", "boot GREEN 2s");
  writeRgb(0, kBootLevel, 0);
  delay(kBootHoldMs);
  Logger::log("led", "boot BLUE 2s");
  writeRgb(0, 0, kBootLevel);
  delay(kBootHoldMs);
  writeRgb(0, 0, 0);
  return true;
}

#endif  // SOC_RMT_SUPPORTED

}  // namespace

#endif  // STATUS_LED_HAS_RGB

void StatusLed::begin() {
#ifdef STATUS_LED_HAS_RGB
  initialized_ = initRgbBackend();
#else
  Logger::log("led", "no onboard rgb on this board");
#endif
}

void StatusLed::loop() {
#ifdef STATUS_LED_HAS_RGB
  if (!initialized_ || !active_) {
    return;
  }
  if (millis() >= off_at_ms_) {
    writeRgb(0, 0, 0);
    active_ = false;
  }
#endif
}

void StatusLed::flashTelemetrySent() {
#ifdef STATUS_LED_HAS_RGB
  if (!initialized_) {
    return;
  }
  writeRgb(0, 0, kBlueLevel);
  active_ = true;
  off_at_ms_ = millis() + kFlashMs;
#endif
}
