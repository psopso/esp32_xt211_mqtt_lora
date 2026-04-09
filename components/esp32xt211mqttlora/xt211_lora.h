#pragma once
namespace esp32xt211mqttlora {

#include "esphome.h"

class MyComponent : public esphome::Component {
 public:
  void setup() override;
  void loop() override;

 private:
  unsigned long last_log_time = 0;
};

};
