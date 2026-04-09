#include "xt211_lora.h"

static const char *TAG = "my_component";

void MyComponent::setup() {
  ESP_LOGI(TAG, "Setup proběhl");
}

void MyComponent::loop() {
  unsigned long now = esphome::millis();

  if (now - last_log_time >= 1000) {  // 10 sekund
    ESP_LOGI(TAG, "Loop běží...");
    last_log_time = now;
  }
}