#include "xt211_lora.h"

namespace esp32xt211mqttlora {

static const char *TAG = "esp32xt211mqttlora";

bool setuprun = false;

void MyComponent::setup() {
  esphome::delay(1000);
  ESP_LOGCONFIG(TAG, "Setup OK");
  ESP_LOGI(TAG, "MOSI: %d", mosi_);
  ESP_LOGI(TAG, "MISO: %d", miso_);
  ESP_LOGI(TAG, "SCK:  %d", sck_);
  ESP_LOGI(TAG, "NSS:  %d", nss_);
  ESP_LOGI(TAG, "RST:  %d", rst_);
  ESP_LOGI(TAG, "DIO0: %d", dio0_);  
  setuprun = true;
}

void MyComponent::loop() {
  auto now = esphome::millis();

  if (now - last_log_time > 10000) {
    ESP_LOGI(TAG, "Loop běží %d", setuprun);
    ESP_LOGI(TAG, "MOSI: %d", mosi_);
    ESP_LOGI(TAG, "MISO: %d", miso_);
    ESP_LOGI(TAG, "SCK:  %d", sck_);
    ESP_LOGI(TAG, "NSS:  %d", nss_);
    ESP_LOGI(TAG, "RST:  %d", rst_);
    ESP_LOGI(TAG, "DIO0: %d", dio0_);  

    last_log_time = now;
  }
}

}  // namespace esp32xt211mqttlora