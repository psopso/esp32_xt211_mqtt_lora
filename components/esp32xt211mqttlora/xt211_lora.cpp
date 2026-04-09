#include "xt211_lora.h"

namespace esp32xt211mqttlora {

static const char *TAG = "esp32xt211mqttlora";

// registry SX1278
static const uint8_t REG_VERSION = 0x42;
static const uint8_t REG_OP_MODE = 0x01;
static const uint8_t REG_FRF_MSB = 0x06;
static const uint8_t REG_FRF_MID = 0x07;
static const uint8_t REG_FRF_LSB = 0x08;

// SPI read
uint8_t MyComponent::read_reg(uint8_t reg) {
  this->enable();
  this->transfer(reg & 0x7F);
  uint8_t val = this->transfer(0x00);
  this->disable();
  return val;
}

// SPI write
void MyComponent::write_reg(uint8_t reg, uint8_t value) {
  this->enable();
  this->transfer(reg | 0x80);
  this->transfer(value);
  this->disable();
}

void MyComponent::setup() {
  ESP_LOGCONFIG(TAG, "SX1278 init");

  // reset pin
  pinMode(rst_, OUTPUT);
  digitalWrite(rst_, LOW);
  delay(10);
  digitalWrite(rst_, HIGH);
  delay(10);

  // přečtení verze
  uint8_t version = read_reg(REG_VERSION);
  ESP_LOGI(TAG, "SX1278 version: 0x%02X", version);

  if (version != 0x12) {
    ESP_LOGE(TAG, "SX1278 nenalezen!");
    return;
  }

  // LoRa mode + sleep
  write_reg(REG_OP_MODE, 0x80);

  // Standby
  write_reg(REG_OP_MODE, 0x81);

  // frekvence 433 MHz
  uint64_t frf = (433000000ULL << 19) / 32000000;

  write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
  write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
  write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));

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