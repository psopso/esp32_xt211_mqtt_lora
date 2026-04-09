#include "xt211_lora.h"

namespace esp32xt211mqttlora {

static const char *TAG = "esp32xt211mqttlora";

bool setuprun = false;

void MyComponent::setup() {
  esphome::delay(10);
  ESP_LOGI(TAG, "Setup OK");
}

void MyComponent::loop() {
  auto now = esphome::millis();

  if (now - last_log_time > 10000) {
    ESP_LOGI(TAG, "Loop běží %d", setuprun);
    last_log_time = now;
  }
}

}  // namespace esp32xt211mqttlora