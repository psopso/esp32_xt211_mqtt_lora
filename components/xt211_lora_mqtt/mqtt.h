#pragma once
#include "lora_app.h"

#include <chrono>
#include <format>
#include <string>

void send_data_item_to_mqtt(const lora_queue_item_t *item, std::string *topic);
void send_status_to_mqtt(const lora_status_item_t *statusitem, std::string *topic, int16_t rssi);