#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "ilora_driver.h" // Includujeme POUZE rozhraní

namespace esphome {
namespace lora_app {

class LoraApp : public Component {
 public:
  // Metoda pro předání (bind) driveru zvenčí
  void set_lora_driver(ILoraDriver *driver) { this->driver_ = driver; }

  void loop() override {
    // Aplikaci je úplně jedno, jaký hardware pod tím běží. 
    // Věří, že dodaný objekt splňuje "smlouvu" ILoraDriver.
    while (this->driver_ != nullptr && this->driver_->available()) {
      auto pkt = this->driver_->read_packet();
      ESP_LOGI("app", "Přijato %d bajtů, RSSI: %d", pkt.data.size(), pkt.rssi);
    }
  }

 protected:
  ILoraDriver *driver_{nullptr}; // Ukazatel na obecné rozhraní
};

}
}