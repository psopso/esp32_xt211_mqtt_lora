#pragma once

#include "esphome/core/component.h"
//#include "esphome/components/lora_common/ilora_driver.h"
#include "esphome/components/ra02_lora_lib/ilora_driver.h"
#include "esphome/components/mqtt/mqtt_client.h"
#include "cJSON.h"

#define MY_SECRET_NETWORK_ID 0xA1B2 // Vymyslete si jakékoliv 16bitové číslo
#define SENDER_ID_METER      0x01   // ID vašeho elektroměru
#define SENDER_ID_GATEWAY    0x02   // ID brány

typedef enum {
    MSG_TYPE_METER_DATA, 
    MSG_TYPE_STATUS,     
    MSG_TYPE_BATTERY     
} comm_msg_type_t;

// 1. Zkomprimovaný JEDEN záznam pro LoRa
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp;       // 4 byty (UNIX čas)
    uint32_t obis_1_8_0_Wh;   // 4 byty (Total Wh)
    uint32_t obis_1_8_1_Wh;   // 4 byty (T1 Wh)
    uint32_t obis_1_8_2_Wh;   // 4 byty (T2 Wh)
    uint8_t first_after_restart; // 1 byt (bool)
} lora_queue_item_t;          // CELKEM: Jen 17 bajtů na záznam!
#pragma pack(pop)

// 2. Zkomprimovaný záznam pro STATUS (Nově přidáno!)
#pragma pack(push, 1)
typedef struct {
    uint8_t state_code;        // 1B (Např. 0 = OK, 1 = INIT, 2 = ERROR) místo textu
    uint8_t reason_code;       // 1B (Např. 0 = WAKEUP, 1 = REBOOT) místo textu
    uint32_t boot_count;       // 4B
    uint32_t wakeup_cycle_count;// 4B
    uint32_t first_boot_time;  // 4B
    uint16_t wait_time;        // 2B
    uint8_t wait_time_min;     // 1B
    uint8_t wait_time_max;     // 1B
    int32_t adaptive_offset;   // 4B
    int16_t ntp_drift_ms;      // 2B (ntp_drift * 1000)
    int8_t rssi;               // 1B
    uint16_t batt_voltage_mv;  // 2B (battery_voltage * 1000)
    uint8_t batt_soc;          // 1B (0-100 %)
} lora_status_item_t;          // CELKEM: 28 bajtů (místo původních stovek!)
#pragma pack(pop)

// 3. Struktura samotného odesílaného LoRa paketu
#define LORA_MAX_ITEMS_PER_PACKET 8

#pragma pack(push, 1)
/*typedef struct {
	uint16_t network_id;      // 2 bajty: Zabezpečení sítě (např. 0xA1B2)
    uint8_t sender_id;        // 1 bajt: Kdo to posílá
    uint8_t packet_type;      // Např. MSG_TYPE_METER_DATA (1 byt)
    uint8_t item_count;       // Kolik záznamů paket reálně obsahuje (1 byt)
    lora_queue_item_t items[LORA_MAX_ITEMS_PER_PACKET]; // 4 * 17 = 68 bajtů  8*17+5=141
} lora_data_payload_t;        // CELÝ PAKET: 70?? bajtů (Ideální pro LoRa!)
*/
typedef struct {
    uint16_t network_id;      // 2 bajty
    uint8_t sender_id;        // 1 bajt
    uint8_t packet_type;      // 1 bajt (MSG_TYPE_...)
    uint8_t item_count;       // 1 bajt (Pro data 1-8, pro status vždy 1)
    
    // Union znamená, že v paměti bude buď 'items' NEBO 'status'. 
    // Šetří to paměť a sjednocuje odesílání!
    union {
        lora_queue_item_t items[LORA_MAX_ITEMS_PER_PACKET]; 
        lora_status_item_t status;                          
    } payload;
} lora_universal_packet_t;
#pragma pack(pop)

#include "mqtt.h"

namespace esphome {
namespace lora_app {

class LoRaMqttGateway : public Component {
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


//    cJSON *resp = cJSON_CreateObject();
//    cJSON_AddStringToObject(resp, "status", "ok");
//    char *rendered = cJSON_PrintUnformatted(resp);
    
//    ESP_LOGD("custom", "JSON výstup: %s", rendered);

    // DŮLEŽITÉ: Uvolnění paměti pro objekt i vygenerovaný string
//    cJSON_free(rendered);
//    cJSON_Delete(resp);
