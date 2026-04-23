#include "ra02_lora.h"
#include "esphome/core/log.h"
#include "driver/gpio.h" // ESP-IDF zůstává

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora_lib";

// ================= ISR =================
void IRAM_ATTR Ra02Lora::gpio_intr_handler(void *arg) {
    auto *self = static_cast<Ra02Lora *>(arg);
    self->interrupt_triggered_ = true;
}

// ================= SETUP =================
// (Zůstává naprosto identický s vaším funkčním kódem)
void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    this->dio0_pin_->setup();

    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);

    if (this->read_reg(0x42) != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen!");
        this->mark_failed();
        return;
    }

    // GPIO ISR (ESP-IDF) - zachováno!
    gpio_num_t pin = (gpio_num_t)this->dio0_pin_->get_pin();

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << pin);
    gpio_config(&io_conf);

    static bool isr_installed = false;
    if (!isr_installed) {
        gpio_install_isr_service(0);
        isr_installed = true;
    }

    gpio_isr_handler_add(pin, gpio_intr_handler, this);

    // LoRa init
    this->write_reg(0x01, 0x80);
    delay(10);
    this->write_reg(0x01, 0x81);
    this->write_reg(0x06, 0x6C);
    this->write_reg(0x07, 0x40);
    this->write_reg(0x08, 0x00);
    this->write_reg(0x09, 0x8F);
    this->write_reg(0x1D, 0x72);
    this->write_reg(0x1E, 0x74);
    this->write_reg(0x39, 0x12);
    this->write_reg(0x0F, 0x00);
    this->write_reg(0x0E, 0x80);
    this->write_reg(0x40, 0x00); // RX_DONE
    this->write_reg(0x01, 0x85); // RX

    this->state_ = STATE_RX;
    this->last_rx_time_ = millis();

    ESP_LOGI(TAG, ">>> LORA READY (IDF ISR + watchdogs) <<<");
}

// ================= LOOP =================
void Ra02Lora::loop() {
    uint32_t now = millis();

    // ODSTRANĚN BLOK TX INTERVAL - to už bude dělat aplikace!

// ===== TIMEOUTY =====
    if (this->state_ == STATE_TX && (now - this->tx_started_ > this->tx_timeout_ms_)) {
        ESP_LOGW(TAG, "TX timeout -> vynuceny navrat do RX");
        this->write_reg(0x01, 0x81); // Standby
        this->write_reg(0x12, 0xFF); // Vycisteni zablokovanych IRQ
        this->write_reg(0x40, 0x00); // DIO0 = RX_DONE
        this->write_reg(0x01, 0x85); // RX continuous
        
        this->state_ = STATE_RX;
        this->last_rx_time_ = now;   // OPRAVA: Musime vynulovat casovac!
    }
    else if (this->state_ == STATE_RX && (now - this->last_rx_time_ > this->rx_timeout_ms_)) {
        // Zde pouzivame 'else if', aby nedoslo ke kaskadovemu spusteni
        ESP_LOGD(TAG, "RX timeout -> cisteni a restart RX");
        this->write_reg(0x01, 0x81);
        this->write_reg(0x12, 0xFF);
        this->write_reg(0x40, 0x00);
        this->write_reg(0x01, 0x85);
        this->last_rx_time_ = now;
    }
    // ===== CAD trigger =====
    if (!this->cad_running_ && (now % 5000 < 50) && this->state_ == STATE_RX) {
        this->start_cad();
    }

    // ===== IRQ fallback =====
    uint8_t irq = this->read_reg(0x12);
    if (irq != 0 && this->dio0_pin_->digital_read()) {
        this->interrupt_triggered_ = true;
    }

    if (!this->interrupt_triggered_)
        return;

    this->interrupt_triggered_ = false;

    irq = this->read_reg(0x12);
    this->write_reg(0x12, 0xFF);

    // ===== STATE MACHINE =====
    switch (this->state_) {
    case STATE_RX:
        if (irq & 0x40) {
            this->last_rx_time_ = now;
            uint8_t len = this->read_reg(0x13);
            uint8_t addr = this->read_reg(0x10);
            this->write_reg(0x0D, addr);

            this->enable();
            this->transfer_byte(0x00);

            LoraPacket pkt;
            for (int i = 0; i < len; i++) {
                pkt.data.push_back(this->transfer_byte(0x00));
            }
            this->disable();

            pkt.rssi = (int16_t)this->read_reg(0x1A) - 164;
            ESP_LOGD(TAG, "RX RSSI: %d", pkt.rssi);

            // Místo tisknutí strčíme do fronty!
            this->rx_queue_.push(pkt);
            ESP_LOGI(TAG, "rx_queue_.push");
        }

        // CAD done
        if (irq & 0x04) {
            bool detected = irq & 0x01;
            this->cad_running_ = false;
            this->write_reg(0x40, 0x00);
            this->write_reg(0x01, 0x85);
        }
        break;

    case STATE_TX:
        if (irq & 0x08) {
            this->write_reg(0x01, 0x81);
            this->write_reg(0x0F, 0x00);
            this->write_reg(0x0D, 0x00);
            this->write_reg(0x40, 0x00);
            this->write_reg(0x01, 0x85);
            this->state_ = STATE_RX;
        }
        break;
    }
}

// ================= API PRO APLIKACI =================
bool Ra02Lora::available() {
    return !this->rx_queue_.empty();
}

LoraPacket Ra02Lora::read_packet() {
    if (this->rx_queue_.empty()) return LoraPacket();
    LoraPacket pkt = this->rx_queue_.front();
    this->rx_queue_.pop();
    return pkt;
}

// ================= SEND =================
void Ra02Lora::send_packet(std::vector<uint8_t> data) {
    this->state_ = STATE_TX;
    this->tx_started_ = millis();

    this->write_reg(0x01, 0x81);

    this->write_reg(0x0E, 0x80);
    this->write_reg(0x0D, 0x80);

    this->enable();
    this->transfer_byte(0x80);
    for (auto b : data)
        this->transfer_byte(b);
    this->disable();

    this->write_reg(0x22, data.size());

    this->write_reg(0x40, 0x40); // TX_DONE
    this->write_reg(0x01, 0x83);

    ESP_LOGI(TAG, "TX start");
}

// ================= CAD =================
void Ra02Lora::start_cad() {
    ESP_LOGD(TAG, "Start CAD");

    this->cad_running_ = true;

    this->write_reg(0x40, 0x80); // DIO0 = CAD_DONE
    this->write_reg(0x01, 0x87); // CAD mode
}

// ================= SPI =================
void Ra02Lora::write_reg(uint8_t reg, uint8_t val) {
    this->enable();
    this->transfer_byte(reg | 0x80);
    this->transfer_byte(val);
    this->disable();
}

uint8_t Ra02Lora::read_reg(uint8_t reg) {
    this->enable();
    this->transfer_byte(reg & 0x7F);
    uint8_t val = this->transfer_byte(0x00);
    this->disable();
    return val;
}

void Ra02Lora::dump_config() {
    ESP_LOGCONFIG(TAG, "RA02 LoRa FULL (ISR + watchdog + CAD)");
}

} // namespace
} // namespace