#include "esphome/core/log.h"
#include <memory> // Nutné pro std::unique_ptr
#include <cstdlib> // Nutné pro free()

static const char *const TAG = "mqtt";

namespace esphome {
namespace lora_app {

  void pokus() {
    ESP_LOGI(TAG, "Pokus v mqtt");
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);
    
  }

 }
}