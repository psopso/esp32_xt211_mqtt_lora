#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

void Ra02Lora::setup() {
    this->spi_setup(); // Inicializuje SPI komunikaci v ESP-IDF
    this->reset_pin_->setup();
    this->dio0_pin_->setup();
    
    // Reset a konfigurace SX1278 (stejná logika jako v předchozích zprávách)
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);

    // Check communication
    uint8_t version = this->read_reg(0x42);
    if (version != 0x12) {
        ESP_LOGE("ra02_lora", "SX1278 not found! Expected 0x12, got: 0x%02X", version);
        this->mark_failed(); // Stops the component in ESPHome
        return;
    }

    ESP_LOGI("ra02_lora", "SX1278 initialized successfully, version: 0x%02X", version);
}

void Ra02Lora::loop() {
  // Musí zde být, i když je prázdná
}

void Ra02Lora::dump_config() {
  // ESPHome vyžaduje tuto metodu pro výpis informací do logu při startu
  ESP_LOGCONFIG("ra02_lora", "Ra02 LoRa Component");
  ESP_LOGCONFIF("ra02_lora", "reset pin", reset_pin_);
  ESP_LOGCONFIF("ra02_lora", "dio0 pin", dio0_pin_);
}

void Ra02Lora::write_reg(uint8_t reg, uint8_t val) {
    this->enable(); // Nastaví CS na LOW
    this->transfer_byte(reg | 0x80);
    this->transfer_byte(val);
    this->disable(); // Nastaví CS na HIGH
}

// ... zbytek metod (read_reg, send_packet atd.)
}
}
