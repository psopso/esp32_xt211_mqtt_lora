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

 protected:
  unsigned long last_log_time = 0;
};

}  // namespace esp32xt211mqttlora