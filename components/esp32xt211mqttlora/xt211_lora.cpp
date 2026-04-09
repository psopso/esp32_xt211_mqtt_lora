#include "xt211_lora.h"

namespace esp32xt211mqttlora {

static const char *TAG = "esp32xt211mqttlora";

// registry SX1278
static const uint8_t REG_VERSION = 0x42;
static const uint8_t REG_OP_MODE = 0x01;
static const uint8_t REG_FRF_MSB = 0x06;
static const uint8_t REG_FRF_MID = 0x07;
static const uint8_t REG_FRF_LSB = 0x08;

void MyComponent::setup() {
  ESP_LOGCONFIG(TAG, "SX1278 init");

  ESP_LOGI(TAG, "SX1278 init OK");
}

void MyComponent::loop() {
  auto now = esphome::millis();

  if (now - last_log_time > 10000) {
    ESP_LOGI(TAG, "Loop běží");
    last_log_time = now;
  }
}

}  // namespace esp32xt211mqttlora
