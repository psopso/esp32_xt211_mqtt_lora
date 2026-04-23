#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "ilora_driver.h" // Includujeme POUZE rozhraní

static const char *const TAG = "lora_app";

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
      ESP_LOGI(TAG, "Přijato %d bajtů, RSSI: %d", pkt.data.size(), pkt.rssi);
      
      // 2. Naplánování neblokujícího čekání
      // Identifikátor "test_reply" zajistí, že se časovač při dalším paketu přepíše.
      // Pokud chcete odpovědět na každý paket zvlášť, stačí použít unikátní název nebo prázdný string.
      this->set_timeout("test_reply", 2000, [this]() {
        ESP_LOGI("lora_app", "Odesílám testovací odpověď (Ping-Pong)");
        
        // Pošleme jednoduchý payload (např. text "PONG")
        std::vector<uint8_t> response = {0x50, 0x4F, 0x4E, 0x47};
//        this->driver_->send_packet(response);
      });
    }
  }

 protected:
  ILoraDriver *driver_{nullptr}; // Ukazatel na obecné rozhraní
};

}
}