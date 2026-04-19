#pragma once
#include "esphome/core/component.h"
#include "ra02_lora.h" // Známe hlavičku driveru

namespace esphome {
namespace moje_aplikace {

class MojeBrana : public Component {
 public:
  // Předáme driver přes konstruktor (Dependency Injection)
  MojeBrana(ra02_lora::Ra02Lora *lora_driver) : lora_(lora_driver) {}

  void setup() override {
      ESP_LOGI("app", "Aplikační vrstva inicializována.");
  }

  void loop() override {
      uint32_t now = millis();

      // Zde řešíme aplikační odesílání majáku
      if (now - this->last_tx_ > 10000) {
          ESP_LOGI("app", "Odesílám aplikační data...");
          this->lora_->send_packet({0xDE, 0xAD, 0xBE, 0xEF});
          this->last_tx_ = now;
      }

      // Zde vybíráme přijatá data z driveru
      while (this->lora_->available()) {
          ra02_lora::LoraPacket pkt = this->lora_->read_packet();
          
          // Zpracování dat (např. odeslání do Home Assistantu)
          ESP_LOGI("app", "Aplikace obdržela data, délka: %d, RSSI: %d", 
                   pkt.data.size(), pkt.rssi);
                   
          if (pkt.data.size() > 0 && pkt.data[0] == 0xAA) {
              ESP_LOGI("app", "Detekován příkaz pro zapnutí relé!");
          }
      }
  }

 private:
  ra02_lora::Ra02Lora *lora_;
  uint32_t last_tx_{0};
};

}
}