#pragma once
#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include <vector>

namespace esphome {
namespace ra02_lora {

class Ra02Lora : public Component, public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_reset_pin(InternalGPIOPin *pin) { reset_pin_ = pin; }
  void set_dio0_pin(InternalGPIOPin *pin) { dio0_pin_ = pin; }
  void set_dio1_pin(InternalGPIOPin *pin) { dio1_pin_ = pin; }

  void send_packet(std::vector<uint8_t> data);
  void start_cad();

// DEKLARACE STATICKÉ FUNKCE PRO PŘERUŠENÍ
static void IRAM_ATTR gpio_intr_handler(Ra02Lora *arg);

 protected:
  InternalGPIOPin *reset_pin_;
  InternalGPIOPin *dio0_pin_;
  InternalGPIOPin *dio1_pin_;

  uint32_t last_transmission_ = 0;
  uint32_t interval_ = 10000;
  bool waiting_for_cad_ = false;

  void write_reg(uint8_t reg, uint8_t val);
  uint8_t read_reg(uint8_t reg);

};



} // namespace ra02_lora
} // namespace esphome