#pragma once

#include "esphome/core/component.h"
#include "ra02_lora.h"
#include <queue>
#include <vector>

namespace esphome {
namespace ra02_lora {

// ===== Frame =====
struct LoraFrame {
    uint8_t dst;
    uint8_t src;
    uint8_t type;
    uint8_t id;
    std::vector<uint8_t> payload;
};

// ===== State =====
enum StackState {
    STACK_IDLE,
    STACK_WAIT_ACK
};

// ===== Class =====
class LoraStack : public Component {
public:
    void setup() override;
    void loop() override;

    void set_driver(Ra02Lora *drv);

    void send(uint8_t dst, std::vector<uint8_t> payload);

protected:
    Ra02Lora *driver_{nullptr};

    std::queue<LoraFrame> tx_queue_;
    std::queue<LoraFrame> rx_queue_;

    uint8_t node_id_{1};
    uint8_t next_id_{0};

    StackState state_{STACK_IDLE};

    uint8_t retry_count_{0};
    uint32_t last_tx_time_{0};

    uint32_t ack_timeout_{1000};
    uint8_t max_retries_{3};

    uint8_t last_sent_id_{0};

    // interní metody
    void handle_rx(std::vector<uint8_t> data, int16_t rssi);
    void process_tx();
    void process_retry();
    void send_ack(uint8_t dst, uint8_t id);
};

} // namespace ra02_lora
} // namespace esphome