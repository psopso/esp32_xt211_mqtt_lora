#include "lora_app.h"
#include "esphome/core/log.h"
#include <vector>

void process_incoming_packet(LoraPacket pkt);

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

    //Dekodovani paketu
    process_incoming_packet(pkt);
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

void process_incoming_packet(LoraPacket pkt) {
    // 1. Základní kontrola velikosti (aby nedošlo k přístupu mimo paměť)
    // Minimální velikost je hlavička (5 bytů)
    if (pkt.data.size() < 5) {
        ESP_LOGW("DECODE", "Paket je příliš krátký!");
        return;
    }

    // 2. Přetypování bufferu na ukazatel na naši strukturu
    const lora_data_payload_t *payload = reinterpret_cast<const lora_data_payload_t *>(pkt.data.data());

    // 3. Validace sítě a obsahu
    if (payload->network_id != MY_SECRET_NETWORK_ID /*0xA1B2*/) { // Vaše ID sítě
        ESP_LOGW("DECODE", "Neznámé Network ID: 0x%04X", payload->network_id);
        return;
    }

    // 4. Bezpečnostní kontrola počtu položek
    if (payload->item_count > LORA_MAX_ITEMS_PER_PACKET) {
        ESP_LOGE("DECODE", "Neplatný počet položek: %d", payload->item_count);
        return;
    }

    // 5. Čtení dat
    ESP_LOGI("DECODE", "Přijat paket od ID: %d, typ: %d, položek: %d", 
             payload->sender_id, payload->packet_type, payload->item_count);

    for (int i = 0; i < payload->item_count; i++) {
        const lora_queue_item_t &item = payload->items[i];
        
        ESP_LOGI("DECODE", "  [%d] Čas: %u, Celkem: %u Wh, T1: %u Wh, T2: %u Wh, Restart: %s",
                 i, 
                 item.timestamp, 
                 item.obis_1_8_0_Wh, 
                 item.obis_1_8_1_Wh, 
                 item.obis_1_8_2_Wh,
                 item.first_after_restart ? "ANO" : "NE");
        
        // Zde můžete data poslat do sensorů v ESPHome:
        // id(my_sensor).publish_state(item.obis_1_8_0_Wh);
    }
}
