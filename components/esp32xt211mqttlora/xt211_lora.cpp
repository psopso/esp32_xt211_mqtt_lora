#include "xt211_lora.h"

namespace esp32xt211mqttlora {

static const char *TAG = "esp32xt211mqttlora";

// registry
static const uint8_t REG_VERSION = 0x42;

// SPI read
uint8_t MyComponent::read_reg(uint8_t reg) {
  uint8_t tx[2] = {uint8_t(reg & 0x7F), 0x00};
  uint8_t rx[2] = {0};

  spi_transaction_t t = {};
  t.length = 16;
  t.tx_buffer = tx;
  t.rx_buffer = rx;

  spi_device_transmit(spi_, &t);

  return rx[1];
}

// SPI write
void MyComponent::write_reg(uint8_t reg, uint8_t val) {
  uint8_t tx[2] = {uint8_t(reg | 0x80), val};

  spi_transaction_t t = {};
  t.length = 16;
  t.tx_buffer = tx;

  spi_device_transmit(spi_, &t);
}

void MyComponent::setup() {
  ESP_LOGCONFIG(TAG, "Init SPI SX1278");

  // 🔌 BUS CONFIG
  spi_bus_config_t buscfg = {};
  buscfg.miso_io_num = miso_;
  buscfg.mosi_io_num = mosi_;
  buscfg.sclk_io_num = clk_;
  buscfg.quadwp_io_num = -1;
  buscfg.quadhd_io_num = -1;

  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

  // 🔌 DEVICE CONFIG
  spi_device_interface_config_t devcfg = {};
  devcfg.clock_speed_hz = 1 * 1000 * 1000; // radši pomalejší na start
  devcfg.mode = 0;
  devcfg.spics_io_num = nss_;
  devcfg.queue_size = 1;

  ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_));

  // 🔄 RESET
  gpio_set_direction((gpio_num_t)rst_, GPIO_MODE_OUTPUT);

  gpio_set_level((gpio_num_t)rst_, 0);
  vTaskDelay(pdMS_TO_TICKS(10));

  gpio_set_level((gpio_num_t)rst_, 1);
  vTaskDelay(pdMS_TO_TICKS(10));

  // 📡 TEST
  uint8_t version = read_reg(REG_VERSION);

  ESP_LOGI(TAG, "SX1278 version: 0x%02X", version);

  if (version == 0x12) {
    ESP_LOGI(TAG, "SX1278 OK");
  } else {
    ESP_LOGE(TAG, "SX1278 ERROR");
  }
}

void MyComponent::loop() {
  auto now = esphome::millis();

  if (now - last_log_time > 10000) {
    ESP_LOGI(TAG, "Loop běží");
    last_log_time = now;
  }
}

}  // namespace