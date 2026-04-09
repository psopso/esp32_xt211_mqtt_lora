#pragma once
#include "esphome.h"

namespace esp32xt211mqttlora {

class MyComponent : public esphome::Component {
 public:
  void setup() override;
  void loop() override;

 private:
  unsigned long last_log_time = 0;
};

};
