#include "xt211_lora.h"

void set_role(const std::string &role) {
  if (role == "tx") role_ = ROLE_TX;
  else role_ = ROLE_RX;
}

namespace esp32xt211mqttlora {

static const char *TAG = "esp32xt211mqttlora";

bool setuprun = false;
bool firstrun = true;
int counter = 0;

void MyComponent::dump_config() {
  ESP_LOGW(TAG, "*** esp32xt211mqttlora ***");
  ESP_LOGI(TAG, "GPIO Setup");
  ESP_LOGI(TAG, "  MOSI: %d", mosi_);
  ESP_LOGI(TAG, "  MISO: %d", miso_);
  ESP_LOGI(TAG, "  SCK:  %d", sck_);
  ESP_LOGI(TAG, "  NSS:  %d", nss_);
  ESP_LOGI(TAG, "  RST:  %d", rst_);
  ESP_LOGW(TAG, "  DIO0: %d", dio0_);  
      //delay(10);  // NOLINT
}

void MyComponent::setup() {
//  esphome::delay(2000);
  ESP_LOGCONFIG(TAG, "Setup OK");
  setuprun = true;
}

void MyComponent::loop() {
  if (firstrun) {
    firstrun = false;
    ESP_LOGI(TAG, "First Loop %d", setuprun);
  }

  auto now = esphome::millis();

  if (role_ == ROLE_TX) {
    if (now - last_log_time > 5000) {
      ESP_LOGI(TAG, "[TX] Posílám paket: HELLO");
      last_log_time = now;
    }
  }

  if (role_ == ROLE_RX) {
    if (now - last_log_time > 7000) {
      ESP_LOGI(TAG, "[RX] Přijat paket: HELLO");
      last_log_time = now;
    }
  }
  if (now - last_log_time > 10000) {
    counter = counter + 1;
    ESP_LOGI(TAG, "Loop běží %d  %d", setuprun, counter);

    last_log_time = now;
  }
}

}  // namespace esp32xt211mqttlora