#pragma once

#include "esphome.h"

class MyComponent : public esphome::Component {
 public:
  void setup() override;
  void loop() override;

 private:
  unsigned long last_log_time = 0;
};