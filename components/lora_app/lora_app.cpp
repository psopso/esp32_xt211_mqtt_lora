#include "lora_app.h"
#include "esphome/core/log.h"
#include <vector>

namespace esphome {
namespace lora_app {

// TAG přesunut sem. Zabrání to chybám při kompilaci (multiple definition), 
// pokud byste lora_app.h includoval na více místech.
static const char *const TAG = "lora_app";

void LoraApp::set_lora_driver(ILoraDriver *driver) {
  this->driver_ = driver;
}

void LoraApp::loop() {
  // Aplikaci je úplně jedno, jaký hardware pod tím běží. 
  // Věří, že dodaný objekt splňuje "smlouvu" ILoraDriver.
  while (this->driver_ != nullptr && this->driver_->available()) {
    auto pkt = this->driver_->read_packet();
    ESP_LOGI(TAG, "Přijato %d bajtů, RSSI: %d", pkt.data.size(), pkt.rssi);
    
    // Naplánování neblokujícího čekání
    // Identifikátor "test_reply" zajistí, že se časovač při dalším paketu přepíše.
    this->set_timeout("test_reply", 2000, [this]() {
      ESP_LOGI(TAG, "Odesílám testovací odpověď (Ping-Pong)");
      
      // Pošleme jednoduchý payload (např. text "PONG")
      std::vector<uint8_t> response = {0x50, 0x4F, 0x4E, 0x47};
      this->driver_->send_packet(response);
    });
  }
}

} // namespace lora_app
} // namespace esphome