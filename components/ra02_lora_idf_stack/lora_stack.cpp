#include "lora_stack.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "lora_stack";

// ===== SETUP =====
void LoraStack::setup() {
    if (driver_ == nullptr) {
        ESP_LOGE(TAG, "Driver not set!");
        return;
    }

    driver_->on_receive_ = [this](auto data, auto rssi) {
        this->handle_rx(data, rssi);
    };

    ESP_LOGI(TAG, "LoRa stack ready");
}

// ===== LOOP =====
void LoraStack::loop() {
    process_tx();
    process_retry();
}

// ===== SET DRIVER =====
void LoraStack::set_driver(Ra02Lora *drv) {
    driver_ = drv;
}

// ===== SEND =====
void LoraStack::send(uint8_t dst, std::vector<uint8_t> payload) {
    LoraFrame f;
    f.dst = dst;
    f.src = node_id_;
    f.type = 0x01;
    f.id = next_id_++;
    f.payload = payload;

    tx_queue_.push(f);
}

// ===== RX =====
void LoraStack::handle_rx(std::vector<uint8_t> data, int16_t rssi) {
    if (data.size() < 4) return;

    LoraFrame f;
    f.dst = data[0];
    f.src = data[1];
    f.type = data[2];
    f.id  = data[3];
    f.payload.assign(data.begin() + 4, data.end());

    if (f.type == 0x02) {
        if (state_ == STACK_WAIT_ACK && f.id == last_sent_id_) {
            state_ = STACK_IDLE;
            retry_count_ = 0;
            tx_queue_.pop();
            ESP_LOGI(TAG, "ACK received");
        }
        return;
    }

    if (f.dst == node_id_) {
        rx_queue_.push(f);
        send_ack(f.src, f.id);
    }
}

// ===== TX =====
void LoraStack::process_tx() {
    if (state_ != STACK_IDLE) return;
    if (tx_queue_.empty()) return;

    auto &f = tx_queue_.front();

    std::vector<uint8_t> raw;
    raw.push_back(f.dst);
    raw.push_back(f.src);
    raw.push_back(f.type);
    raw.push_back(f.id);
    raw.insert(raw.end(), f.payload.begin(), f.payload.end());

    driver_->send_packet(raw);

    last_sent_id_ = f.id;
    last_tx_time_ = millis();
    retry_count_ = 0;

    state_ = STACK_WAIT_ACK;
}

// ===== RETRY =====
void LoraStack::process_retry() {
    if (state_ != STACK_WAIT_ACK) return;

    if (millis() - last_tx_time_ > ack_timeout_) {
        if (retry_count_ < max_retries_) {
            retry_count_++;
            ESP_LOGW(TAG, "Retry %d", retry_count_);
            state_ = STACK_IDLE;
        } else {
            ESP_LOGE(TAG, "TX failed");
            tx_queue_.pop();
            state_ = STACK_IDLE;
        }
    }
}

// ===== ACK =====
void LoraStack::send_ack(uint8_t dst, uint8_t id) {
    std::vector<uint8_t> raw = {
        dst,
        node_id_,
        0x02,
        id
    };

    driver_->send_packet(raw);
}

} // namespace
} // namespace