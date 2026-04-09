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
