#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora";

void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    this->dio0_pin_->setup();
    this->dio0_pin_->make_input(); // Explicitně nastavit jako vstup

    // Rychlý reset
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);

    // Verifikace čipu
    if (this->read_reg(0x42) != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen!");
        this->mark_failed();
        return;
    }

    // KONFIGURACE MODEMU
    this->write_reg(0x01, 0x81); // Standby mode (nutné pro zápis registrů)
    
    // Frekvence 434.0 MHz
    this->write_reg(0x06, 0x6C); 
    this->write_reg(0x07, 0x80); 
    this->write_reg(0x08, 0x00);

    // --- KLÍČOVÉ NASTAVENÍ VÝKONU ---
    this->write_reg(0x09, 0x8F); // PA_BOOST zapnut (pro Ra-02 kritické!)

    // Parametry přenosu (SF7, BW 125kHz, CR 4/5)
    this->write_reg(0x1D, 0x72); 
    this->write_reg(0x1E, 0x74); // SF7 + CRC On
    this->write_reg(0x39, 0x12); // Sync Word

    // Nastavení DIO0: 0x00 znamená, že v režimu RX vyvolá RXDone a v TX vyvolá TXDone
    this->write_reg(0x40, 0x00); 

    // Start v trvalém příjmu
    this->write_reg(0x01, 0x85); 
    ESP_LOGI(TAG, "Ra02 Ready (434MHz, SF7, PA_BOOST)");
}

void Ra02Lora::start_cad() {
    this->waiting_for_cad_ = true;
    this->write_reg(0x01, 0x81); // Standby
    this->write_reg(0x40, 0x80); // DIO0 = CAD Done
    this->write_reg(0x01, 0x87); // Režim CAD

    // --- VERIFIKACE REŽIMU ---
    uint8_t current_mode = this->read_reg(0x01);
    if (false) { //(current_mode != 0x81) {
        ESP_LOGE(TAG, "CHYBA: Rezim Standby nedrzi! Precteno: 0x%02X (ocekavano 0x81)", current_mode);
        // Neukončujeme, zkusíme pokračovat, ale víme, že je něco špatně
    } else {
        ESP_LOGI(TAG, "Rezim : 0x%02X", current_mode);
    }
}

void Ra02Lora::loop() {
    uint32_t now = millis();

    // A. LOGIKA VYSÍLÁNÍ (Maják každých 10s)
    // --- U PŘIJÍMAČE TENTO BLOK SMAŽTE NEBO ZAKOMENTUJTE ---
    if (now - this->last_transmission_ > 10000) {
        this->send_packet({0xDE, 0xAD, 0xBE, 0xEF});
        this->last_transmission_ = now;
    }

    // B. OBSLUHA PŘERUŠENÍ (DIO0)
    if (this->dio0_pin_->digital_read()) {
        ESP_LOGI(TAG, "DIO pin interrupt");
        uint8_t irq = this->read_reg(0x12);
        this->write_reg(0x12, 0xFF); // Okamžitý reset vlajek

        if (irq & 0x40) { // RX Done
            uint8_t len = this->read_reg(0x13);
            this->write_reg(0x0D, this->read_reg(0x10)); // Nastavit pointer na začátek
            
            this->enable();
            this->transfer_byte(0x00); // Read FIFO
            std::string out = "";
            for(int i = 0; i < len; i++) {
                char b[5]; sprintf(b, "%02X ", this->transfer_byte(0x00));
                out += b;
            }
            this->disable();

            int16_t rssi = (int16_t)this->read_reg(0x1B) - 164;
            ESP_LOGI(TAG, ">>> Prijato: [%s] (%d dBm)", out.c_str(), rssi);
        }
        
        else if (irq & 0x08) { // TX Done
            ESP_LOGD(TAG, "Vysilani dokonceno, navrat do prijmu.");
            this->write_reg(0x01, 0x85); // Zpět do RX Continuous
        }
    }
}

void Ra02Lora::send_packet(std::vector<uint8_t> data) {
    this->write_reg(0x01, 0x81); // Standby
    this->write_reg(0x0D, 0x80); // TX Base pointer
    this->write_reg(0x0F, 0x80); // FIFO pointer
    
    this->enable();
    this->transfer_byte(0x00 | 0x80); // Write FIFO
    for (uint8_t b : data) this->transfer_byte(b);
    this->disable();
    
    this->write_reg(0x22, data.size());
    this->write_reg(0x01, 0x83); // Start TX Mode
    ESP_LOGI(TAG, "Odesilam...");
}

void Ra02Lora::dump_config() {
    ESP_LOGCONFIG(TAG, "Ra02 LoRa Component (Formal)");
}

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

} // namespace ra02_lora
} // namespace esphome