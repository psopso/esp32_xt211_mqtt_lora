#include "lora_app.h"
#include "esphome/core/log.h"
#include <vector>

void process_incoming_packet(const std::vector<uint8_t>& data);

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
    process_incoming_packet(pkt.data);
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

// Pomocná funkce pro zpětný překlad kódu na text
    std::string get_state_string(uint8_t code) {
        switch(code) {
            case 0: return "OK";
            case 1: return "SLEEP";
            case 99: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    void process_incoming_packet(const std::vector<uint8_t>& data) {

        // 1. Ochrana proti podtečení paměti (velikost hlavičky)
        if (data.size() < 5) {
            ESP_LOGW("LORA_RX", "Paket je prilis kratky!");
            return;
        }

        // 2. Přetypování přijatých bytů na naši strukturu
        const lora_universal_packet_t *packet = reinterpret_cast<const lora_universal_packet_t *>(data.data());

        // 3. Bezpečnostní kontrola sítě
        if (packet->network_id != 0xA1B2) {
            ESP_LOGW("LORA_RX", "Cizi paket ignorovan (NetID: 0x%04X)", packet->network_id);
            return;
        }

        // 4. Rozvětvení podle typu zprávy
        switch (packet->packet_type) {
            
            case MSG_TYPE_METER_DATA: {
                ESP_LOGI("LORA_RX", "Prijata DATA (polozek: %d)", packet->item_count);
                
                for (int i = 0; i < packet->item_count; i++) {
                    const lora_queue_item_t &item = packet->payload.items[i];
                    //item.timestamp

		    time_t ts = item.timestamp;
		    struct tm *utc = gmtime(&ts);
		    char buffer[20];
		    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", utc);

                    // Dekomprese dat (zpet na desetinná čísla, pokud to HA vyžaduje)
                    float total_kwh = item.obis_1_8_0_Wh / 1000.0f;
                    
                    ESP_LOGD("LORA_RX", "Zaznam %d: %s %f kWh %f kWh %f kWh", i, buffer, item.obis_1_8_0_Wh / 1000.0f, item.obis_1_8_1_Wh / 1000.0f, item.obis_1_8_2_Wh / 1000.0f);
                    //Test
		    esphome::lora_app::pokus(item);       
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

                ESP_LOGI("LORA_RX", "Stav: %s, Boot count: %d, Baterie: %.2f V Wakeupcount: %d AdaptiveOffset: %d", 
                         state_text.c_str(), status.boot_count, batt_v, status.wakeup_cycle_count, status.adaptive_offset);

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
