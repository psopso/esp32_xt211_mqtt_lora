#include "lora_app.h"
#include "esphome/core/log.h"
#include <vector>

void process_incoming_packet(LoraPacket pkt);

namespace esphome {
namespace lora_app {

// TAG přesunut sem. Zabrání to chybám při kompilaci (multiple definition), 
// pokud byste lora_app.h includoval na více místech.
static const char *const TAG = "lora_app";

void LoRaMqttGateway::set_lora_driver(ILoraDriver *driver) {
  this->driver_ = driver;
}

void LoRaMqttGateway::loop() {
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

void process_incoming_packet(LoraPacket packet) {
    // 1. Základní kontrola velikosti (aby nedošlo k přístupu mimo paměť)
    // Minimální velikost je hlavička (5 bytů)
    if (packet.data.size() < 5) {
        ESP_LOGW("DECODE", "Paket je příliš krátký!");
        return;
    }

    // 2. Přetypování bufferu na ukazatel na naši strukturu
    const lora_data_payload_t *payload = reinterpret_cast<const lora_data_payload_t *>(packet.data.data());

    // 3. Validace sítě a obsahu
    if (payload->network_id != MY_SECRET_NETWORK_ID /*0xA1B2*/) { // Vaše ID sítě
        ESP_LOGW("DECODE", "Neznámé Network ID: 0x%04X", payload->network_id);
        return;
    }

        // 4. Rozvětvení podle typu zprávy
        switch (packet->packet_type) {
            
            case MSG_TYPE_METER_DATA: {
                ESP_LOGI("LORA_RX", "Prijata DATA (polozek: %d)", packet->item_count);
                
                for (int i = 0; i < packet->item_count; i++) {
                    const lora_queue_item_t &item = packet->payload.items[i];
                    
                    // Dekomprese dat (zpet na desetinná čísla, pokud to HA vyžaduje)
                    float total_kwh = item.obis_1_8_0_Wh / 1000.0f;
                    
                    ESP_LOGD("LORA_RX", "Zaznam %d: %f kWh", i, total_kwh);
                    
                    // id(sensor_total_kwh).publish_state(total_kwh);
                }
                break;
            }

            case MSG_TYPE_STATUS: {
                ESP_LOGI("LORA_RX", "Prijat STATUS");
                
                // Přistupujeme do paměti přes větvičku .status v našem unionu!
                const lora_status_item_t &status = packet->payload.status;

                // Překlad stavu zpět na text
                std::string state_text = get_state_string(status.state_code);
                
                // Přepočet driftu a napětí zpět z celých čísel na desetinná
                float drift_sec = status.ntp_drift_ms / 1000.0f;
                float batt_v = status.batt_voltage_mv / 1000.0f;

                ESP_LOGI("LORA_RX", "Stav: %s, Boot count: %d, Baterie: %.2f V", 
                         state_text.c_str(), status.boot_count, batt_v);

                // Zde publikujeme do text_sensor a sensor komponent v ESPHome
                // id(status_text_sensor).publish_state(state_text);
                // id(boot_count_sensor).publish_state(status.boot_count);
                // id(battery_v_sensor).publish_state(batt_v);
                break;
            }

            default:
                ESP_LOGW("LORA_RX", "Neznamy typ paketu: %d", packet->packet_type);
                break;
        }


}
