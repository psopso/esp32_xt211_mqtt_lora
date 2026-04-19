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

    // VŠIMNĚTE SI: Blok "// ===== TX interval =====" byl KOMPLETNĚ VYMAZÁN!

    // ... (timeouty a CAD fallback zůstávají beze změny) ...

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

            // Naplnění struktury paketu
            LoraPacket pkt;
            for (int i = 0; i < len; i++) {
                pkt.data.push_back(this->transfer_byte(0x00));
            }
            this->disable();

            pkt.rssi = (int16_t)this->read_reg(0x1B) - 164;

            // Místo logování prostě vložíme paket do fronty pro aplikaci
            this->rx_queue_.push(pkt);
        }

        // ... (CAD done logika zůstává beze změny) ...
        break;

    // ... (STATE_TX zůstává beze změny) ...
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

// ================= APLIKAČNÍ API =================
bool Ra02Lora::available() {
    return !this->rx_queue_.empty();
}

LoraPacket Ra02Lora::read_packet() {
    if (this->rx_queue_.empty()) {
        return LoraPacket(); // Vrátí prázdný paket jako pojistku
    }
    LoraPacket pkt = this->rx_queue_.front();
    this->rx_queue_.pop();
    return pkt;
}

} // namespace ra02_lora
} // namespace esphome