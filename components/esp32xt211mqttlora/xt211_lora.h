#pragma once

#include "esphome.h"

namespace esp32xt211mqttlora {

class MyComponent : public esphome::Component {
 public:
  void setup() override;
  void loop() override;

  float get_setup_priority() const override {
    return esphome::setup_priority::LATE;
  }

  // 🔽 settery
  void set_mosi(int pin) { mosi_ = pin; }
  void set_miso(int pin) { miso_ = pin; }
  void set_sck(int pin)  { sck_ = pin; }
  void set_nss(int pin)  { nss_ = pin; }
  void set_rst(int pin)  { rst_ = pin; }
  void set_dio0(int pin) { dio0_ = pin; }

 protected:
  unsigned long last_log_time = 0;

  // 🔽 uložené piny
  int mosi_;
  int miso_;
  int sck_;
  int nss_;
  int rst_;
  int dio0_;
};

}  // namespace esp32xt211mqttlora