#include "esphome/core/log.h"
#include <memory> // Nutné pro std::unique_ptr
#include <cstdlib> // Nutné pro free()
#include "esphome/components/mqtt/mqtt_client.h"
#include "cJSON.h"
#include "mqtt.h"

static const char *const TAG = "mqtt";

std::string get_timestamp_string(std::time_t ts) {
    // Převod time_t na sys_time (bod v čase)
    auto tp = std::chrono::system_clock::from_time_t(ts);
    // Přímé formátování do stringu
    return std::format("{:%a %Y-%m-%d %H:%M:%S GMT}", tp);
}

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

  void send_data_item_to_mqtt(const lora_queue_item_t *item, std::string *topic) {
    ESP_LOGI(TAG, "send_data_item_to_mqtt");
    std::unique_ptr<cJSON, decltype(verbose_cjson_delete)> root(cJSON_CreateObject(), verbose_cjson_delete);
    if (!root) {
      ESP_LOGE("LORA", "Malo pameti pro JSON");
      return; // Pokud dojde paměť, prostě vyskočíme. Žádný únik nehrozí.
    }

    std::string dt = get_timestamp_string(item->timestamp);
    std::string dtnow = get_timestamp_string(std::time(nullptr));

    cJSON_AddStringToObject(root.get(), "datetime", dtnow.c_str());

    cJSON *data = cJSON_CreateObject();
    cJSON_AddItemToObject(root.get(), "data", data);
    cJSON_AddStringToObject(data, "reading_datetime", dt.c_str());

    cJSON *values = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "values", values);
    
    cJSON_AddNumberToObject(values, "1.8.0", (double)item->obis_1_8_0_Wh/1000.000);
    cJSON_AddNumberToObject(values, "1.8.1", (double)item->obis_1_8_1_Wh/1000.000);
    cJSON_AddNumberToObject(values, "1.8.2", (double)item->obis_1_8_2_Wh/1000.000);
    

    std::unique_ptr<char, decltype(verbose_free)> json_string(cJSON_PrintUnformatted(root.get()), verbose_free);

    if (!json_string) {
      ESP_LOGE("LORA", "Chyba generovani textu");
      return; // Vyskočíme. C++ se samo postará o zavolání cJSON_Delete(root)!
    }

    esphome::mqtt::global_mqtt_client->publish(topic->c_str(), json_string.get());

  }

  void send_status_to_mqtt(const lora_queue_item_t *item, std::string *topic) {
    ESP_LOGI(TAG, "send_status_to_mqtt");

  }
