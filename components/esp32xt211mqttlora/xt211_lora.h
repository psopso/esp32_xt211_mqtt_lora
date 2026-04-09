#pragma once

#include "esphome.h"
#include "driver/spi_master.h"

namespace esp32xt211mqttlora {

class MyComponent : public esphome::Component {
 public:
  void setup() override;
  void loop() override;

  float get_setup_priority() const override {
    return esphome::setup_priority::LATE;
  }

  // settery
  void set_mosi(int pin) { mosi_ = pin; }
  void set_miso(int pin) { miso_ = pin; }
  void set_clk(int pin)  { clk_ = pin; }
  void set_nss(int pin)  { nss_ = pin; }
  void set_rst(int pin)  { rst_ = pin; }
  void set_dio0(int pin) { dio0_ = pin; }

 protected:
  int mosi_, miso_, clk_;
  int nss_, rst_, dio0_;

  spi_device_handle_t spi_;

  uint8_t read_reg(uint8_t reg);
  void write_reg(uint8_t reg, uint8_t val);

  unsigned long last_log_time = 0;
};

}  // namespace