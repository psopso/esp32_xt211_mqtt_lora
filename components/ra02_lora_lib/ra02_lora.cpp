#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora";

// ================= SETUP =================
void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    this->dio0_pin_->setup();

    // HW reset SX1278
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);

    if (this->read_reg(0x42) != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen!");
        this->mark_failed();
        return;
    }

    // ISR
    this->dio0_pin_->attach_interrupt(
        &Ra02Lora::gpio_intr_handler,
        this,
        gpio::INTERRUPT_RISING_EDGE
    );

    // Sleep → Standby
    this->write_reg(0x01, 0x80);
    delay(10);
    this->write_reg(0x01, 0x81);

    // Frequency 434 MHz
    this->write_reg(0x06, 0x6C);
    this->write_reg(0x07, 0x80);
    this->write_reg(0x08, 0x00);

    // TX power
    this->write_reg(0x09, 0x8F);

    // Modem
    this->write_reg(0x1D, 0x72);
    this->write_reg(0x1E, 0x74);
    this->write_reg(0x39, 0x12);

    // FIFO base
    this->write_reg(0x0F, 0x00); // RX base
    this->write_reg(0x0E, 0x80); // TX base

    // DIO0 = RX_DONE
    this->write_reg(0x40, 0x00);

    // Start RX
    this->write_reg(0x01, 0x85);

    this->state_ = STATE_RX;

    ESP_LOGI(TAG, ">>> LORA START <<<");
}

// ================= ISR =================
void IRAM_ATTR Ra02Lora::gpio_intr_handler(Ra02Lora *arg) {
    arg->interrupt_triggered_ = true;
}

// ================= LOOP =================
void Ra02Lora::loop() {
    uint32_t now = millis();

    // periodické TX (jen když jsme v RX)
    if ((now - this->last_transmission_) > this->interval_ && this->state_ == STATE_RX) {
        this->send_packet({0xDE, 0xAD, 0xBE, 0xEF});
        this->last_transmission_ = now;
    }

    /*
    // ================= FALLBACK IRQ =================
    static uint32_t last_irq_check = 0;

    if (now - last_irq_check > 100) {
        last_irq_check = now;

        uint8_t irq = this->read_reg(0x12);

        if (irq != 0) {
            ESP_LOGW(TAG, "IRQ fallback: 0x%02X", irq);
            this->interrupt_triggered_ = true;
        }

        // DIO0 stuck HIGH ochrana
        if (this->dio0_pin_->digital_read()) {
            if (irq != 0) {
                ESP_LOGW(TAG, "DIO0 stuck HIGH → recovery");
                this->interrupt_triggered_ = true;
            }
        }
    }
    */
    // ================= IRQ processing =================
    if (!this->interrupt_triggered_)
        return;

    this->interrupt_triggered_ = false;

    uint8_t irq = this->read_reg(0x12);
    this->write_reg(0x12, 0xFF); // clear IRQ

    switch (this->state_) {

    // ===== RX =====
    case STATE_RX:
        if (irq & 0x40) { // RX_DONE

            uint8_t len = this->read_reg(0x13);
            uint8_t addr = this->read_reg(0x10);
            this->write_reg(0x0D, addr);

            this->enable();
            this->transfer_byte(0x00);

            std::string out = "";
            for (int i = 0; i < len; i++) {
                char b[5];
                sprintf(b, "%02X ", this->transfer_byte(0x00));
                out += b;
            }
            this->disable();

            int16_t rssi = (int16_t)this->read_reg(0x1B) - 164;

            ESP_LOGI(TAG, "RX: [%s] RSSI=%d dBm", out.c_str(), rssi);
        }
        break;

    // ===== TX =====
    case STATE_TX:
        if (irq & 0x08) { // TX_DONE
            ESP_LOGD(TAG, "TX done → RX");

            // stop TX
            this->write_reg(0x01, 0x81);

            // reset FIFO RX
            this->write_reg(0x0F, 0x00);
            this->write_reg(0x0D, 0x00);

            // přepnout DIO0 zpět na RX
            this->write_reg(0x40, 0x00);

            // clear IRQ znovu pro jistotu
            this->write_reg(0x12, 0xFF);

            // zpět do RX
            this->write_reg(0x01, 0x85);

            this->state_ = STATE_RX;
        }
        break;
    }
}

// ================= SEND =================
void Ra02Lora::send_packet(std::vector<uint8_t> data) {
    this->state_ = STATE_TX;

    // standby
    this->write_reg(0x01, 0x81);

    // TX FIFO
    this->write_reg(0x0E, 0x80);
    this->write_reg(0x0D, 0x80);

    this->enable();
    this->transfer_byte(0x80); // FIFO write
    for (uint8_t b : data)
        this->transfer_byte(b);
    this->disable();

    this->write_reg(0x22, data.size());

    // DIO0 = TX_DONE
    this->write_reg(0x40, 0x40);

    // start TX
    this->write_reg(0x01, 0x83);

    ESP_LOGI(TAG, "TX start");
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

// ================= CONFIG =================
void Ra02Lora::dump_config() {
    ESP_LOGCONFIG(TAG, "RA02 LoRa ready");
}

} // namespace ra02_lora
} // namespace esphome