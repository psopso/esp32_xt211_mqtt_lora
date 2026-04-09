#pragma once

#include "esphome.h"
#include "esphome/components/spi/spi.h"

namespace esp32xt211mqttlora {

class MyComponent : public esphome::Component, public esphome::spi::SPIDevice {
 public:
  void setup() override;
  void loop() override;

  float get_setup_priority() const override {
    return esphome::setup_priority::LATE;
  }

  // role (zatím nepoužíváme, ale necháme)
  enum Role {
    ROLE_TX,
    ROLE_RX
  };

  void set_role(const std::string &role) {
    if (role == "tx") role_ = ROLE_TX;
    else role_ = ROLE_RX;
  }

  // GPIO settery
  void set_nss(int pin) { nss_ = pin; }
  void set_rst(int pin) { rst_ = pin; }
  void set_dio0(int pin) { dio0_ = pin; }

 protected:
  unsigned long last_log_time = 0;

  Role role_ = ROLE_RX;

  int nss_;
  int rst_;
  int dio0_;

  // SPI helpery
  uint8_t read_reg(uint8_t reg);
  void write_reg(uint8_t reg, uint8_t value);
};

}  // namespace esp32xt211mqttlora