#pragma once
#include "esphome/core/component.h"
#include "ilora_driver.h"

namespace esphome {
namespace lora_app {

class LoraApp : public Component {
 public:
  // Metoda pro předání (bind) driveru zvenčí
  void set_lora_driver(ILoraDriver *driver);

  // Hlavní smyčka komponenty
  void loop() override;

 protected:
  ILoraDriver *driver_{nullptr}; // Ukazatel na obecné rozhraní
};

} // namespace lora_app
} // namespace esphome