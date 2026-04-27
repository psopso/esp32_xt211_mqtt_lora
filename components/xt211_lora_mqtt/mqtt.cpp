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

  void send_data_item_to_mqtt(const lora_queue_item_t *item) {
    ESP_LOGI(TAG, "send_data_item_to_mqtt");
    std::unique_ptr<cJSON, decltype(verbose_cjson_delete)> root(cJSON_CreateObject(), verbose_cjson_delete);
    if (!root) {
      ESP_LOGE("LORA", "Malo pameti pro JSON");
      return; // Pokud dojde paměť, prostě vyskočíme. Žádný únik nehrozí.
    }
//elektromertest/data {"datetime":"Sun Apr 26 16:49:55 2026","data":{"reading_datetime":"Sun 2026-04-26 16:40:22 //GMT","first_boot":false,"values":{"1.8.0":443.451,"1.8.1":83.769,"1.8.2":359.682,"1.8.3":0,"1.8.4":0,"96.1.1":"5100025085"}}}    

    //cJSON_AddStringToObject(root.get(), "state", "OK");
    //cJSON_AddNumberToObject(root.get(), "batt_v", 3.2);

    //cJSON *podObjekt = cJSON_CreateObject();  //vytvorim data

//    std::unique_ptr<cJSON, decltype(verbose_cjson_delete)> data(cJSON_CreateObject(), verbose_cjson_delete);
    
//    std::string dt = get_timestamp_string(item->timestamp);
//    cJSON_AddStringToObject(data.get(), "datetime", dt.c_str());


//    cJSON_AddItemToObject(root.get(), "data", data.get());
    

    //mqtt::global_mqtt_client->publish("muj/topic", json_string.get());
    //sesphome::mqtt::global_mqtt_client->publish("elektromertest1/data", "Testovaci zprava do mqtt");
//    std::unique_ptr<char, decltype(verbose_free)> json_string(cJSON_PrintUnformatted(root.get()), verbose_free);

//    if (!json_string) {
//      ESP_LOGE("LORA", "Chyba generovani textu");
//      return; // Vyskočíme. C++ se samo postará o zavolání cJSON_Delete(root)!
//    }
    //esphome::mqtt::global_mqtt_client->publish("elektromertest1/data", json_string.get());

  }
