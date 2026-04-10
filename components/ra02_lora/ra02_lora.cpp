#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora";

void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    this->dio0_pin_->setup();
    
    // Hardwarový reset
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10); // Krátká pauza po resetu je jistota

    // Kontrola komunikace
    uint8_t version = this->read_reg(0x42);
    if (version != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen! Registr 0x42 vratil: 0x%02X", version);
        this->mark_failed();
        return;
    }

    ESP_LOGI(TAG, "SX1278 uspesne inicializovan, verze: 0x%02X", version);
}

void Ra02Lora::loop() {}

void Ra02Lora::dump_config() {
    ESP_LOGCONFIG(TAG, "Ra02 LoRa Component");
    LOG_PIN("  Reset Pin: ", this->reset_pin_);
    LOG_PIN("  DIO0 Pin: ", this->dio0_pin_);
}

void Ra02Lora::write_reg(uint8_t reg, uint8_t val) {
    this->enable();
    this->transfer_byte(reg | 0x80); // MSB = 1 pro zápis
    this->transfer_byte(val);
    this->disable();
}

uint8_t Ra02Lora::read_reg(uint8_t reg) {
    this->enable();
    this->transfer_byte(reg & 0x7F); // MSB = 0 pro čtení
    uint8_t val = this->transfer_byte(0x00);
    this->disable();
    return val;
}

} // namespace ra02_lora
} // namespace esphome