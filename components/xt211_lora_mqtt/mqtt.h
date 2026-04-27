#pragma once
#include "lora_app.h"

void send_data_item_to_mqtt(const lora_queue_item_t *item);

std::string get_timestamp_string(std::time_t ts) {
    // Převod time_t na sys_time (bod v čase)
    auto tp = std::chrono::system_clock::from_time_t(ts);
    // Přímé formátování do stringu
    return std::format("{:%a %Y-%m-%d %H:%M:%S GMT}", tp);
}
