#pragma once
#include "esphome/core/component.h"
#include "esphome/components/lora_common/ilora_driver.h"

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

// 2. Struktura samotného odesílaného LoRa paketu
#define LORA_MAX_ITEMS_PER_PACKET 8

#pragma pack(push, 1)
typedef struct {
	uint16_t network_id;      // 2 bajty: Zabezpečení sítě (např. 0xA1B2)
    uint8_t sender_id;        // 1 bajt: Kdo to posílá
    uint8_t packet_type;      // Např. MSG_TYPE_METER_DATA (1 byt)
    uint8_t item_count;       // Kolik záznamů paket reálně obsahuje (1 byt)
    lora_queue_item_t items[LORA_MAX_ITEMS_PER_PACKET]; // 4 * 17 = 68 bajtů  8*17+5=141
} lora_data_payload_t;        // CELÝ PAKET: 70?? bajtů (Ideální pro LoRa!)
#pragma pack(pop)
