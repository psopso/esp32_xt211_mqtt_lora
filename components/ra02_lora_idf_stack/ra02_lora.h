#pragma once
#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include <vector>

struct LoraPacket {
    std::vector<uint8_t> data;
    int16_t rssi;
};

namespace esphome {
namespace ra02_lora {

enum LoraState {
    STATE_RX,
    STATE_TX
};

class Ra02Lora : public Component, public spi::SPIDevice<
    spi::BIT_ORDER_MSB_FIRST,
    spi::CLOCK_POLARITY_LOW,
    spi::CLOCK_PHASE_LEADING,
    spi::DATA_RATE_1MHZ> {

 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_reset_pin(InternalGPIOPin *pin) { reset_pin_ = pin; }
  void set_dio0_pin(InternalGPIOPin *pin) { dio0_pin_ = pin; }
  void set_dio1_pin(InternalGPIOPin *pin) { dio1_pin_ = pin; }

  void send_packet(std::vector<uint8_t> data);
  void start_cad();

  // 🔥 změněná signatura
  static void IRAM_ATTR gpio_intr_handler(void *arg);

 protected:
  InternalGPIOPin *reset_pin_;
  InternalGPIOPin *dio0_pin_;
  InternalGPIOPin *dio1_pin_;

  uint32_t last_transmission_{0};
  uint32_t interval_{10000};

  // optional robustness
  uint32_t last_irq_check_{0};
  uint32_t tx_started_{0};
  uint32_t tx_timeout_ms_{2000};

  uint32_t last_rx_time_{0};
  uint32_t rx_timeout_ms_{15000};

  bool cad_running_{false};
  bool waiting_for_cad_{false};

  void write_reg(uint8_t reg, uint8_t val);
  uint8_t read_reg(uint8_t reg);

  volatile bool interrupt_triggered_{false};
  volatile LoraState state_{STATE_RX};
};

} // namespace ra02_lora
} // namespace esphome