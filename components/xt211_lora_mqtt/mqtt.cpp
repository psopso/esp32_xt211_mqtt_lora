#include "esphome/core/log.h"
#include <memory> // Nutné pro std::unique_ptr
#include <cstdlib> // Nutné pro free()
#include "esphome/components/mqtt/mqtt_client.h"
#include "cJSON.h"
#include "mqtt.h"

static const char *const TAG = "mqtt";


  // Vytvoříme si vlastní mazací funkci pro textový řetězec
  auto verbose_free = [](char* ptr) {
      free(ptr);
      ESP_LOGI("LORA", "--> Pamet pro JSON text byla uspesne smazana!");
  };

  // Vytvoříme si vlastní mazací funkci pro cJSON objekt
  auto verbose_cjson_delete = [](cJSON* ptr) {
      cJSON_Delete(ptr);
      ESP_LOGI("LORA", "--> Pamet pro cJSON objekt byla uspesne smazana!");
  };

  void pokus(struct lora_status_item_t *item) {
    ESP_LOGI(TAG, "Pokus v mqtt");
    std::unique_ptr<cJSON, decltype(verbose_cjson_delete)> root(cJSON_CreateObject(), verbose_cjson_delete);
    if (!root) {
      ESP_LOGE("LORA", "Malo pameti pro JSON");
      return; // Pokud dojde paměť, prostě vyskočíme. Žádný únik nehrozí.
    }

    //std::unique_ptr<char, decltype(verbose_free)> json_string(cJSON_PrintUnformatted(root.get()), verbose_free);
  }

