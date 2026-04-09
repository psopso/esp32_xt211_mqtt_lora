#include "my_component.h"

static const char *TAG = "my_component";

void MyComponent::setup() {
  ESP_LOGI(TAG, "Setup proběhl");
}

void MyComponent::loop() {
  unsigned long now = millis();

  if (now - last_log_time >= 10000) {  // 10 sekund
    ESP_LOGI(TAG, "Loop běží...");
    last_log_time = now;
  }
}