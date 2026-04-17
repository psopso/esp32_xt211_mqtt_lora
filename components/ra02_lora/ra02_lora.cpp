#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora";

void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    this->dio0_pin_->setup();
    this->dio1_pin_->setup();

    // Reset sekvence
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);

    if (this->read_reg(0x42) != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen! Registr 0x42 vratil: 0x%02X", this->read_reg(0x42));
        this->mark_failed();
        return;
    }

    this->write_reg(0x01, 0x80); // LoRa Mode
    this->write_reg(0x06, 0x6C); // 433 MHz
    this->write_reg(0x07, 0x40); 
    this->write_reg(0x08, 0x00);

    this->write_reg(0x1D, 0x72); // BW 125, CR 4/5
    this->write_reg(0x1E, 0x74); // SF7, CRC On
    this->write_reg(0x39, 0x12); // Sync Word
    this->write_reg(0x0C, 0x23); // LNA Max Gain

    this->write_reg(0x01, 0x85); // Start v RX Continuous
    ESP_LOGI(TAG, "LoRa modem inicializován s podporou CAD.");
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

void Ra02Lora::send_packet(std::vector<uint8_t> data) {
    this->write_reg(0x01, 0x81); // Standby
    this->write_reg(0x0D, 0x80); // TX Base
    this->write_reg(0x0F, 0x80); // Pointer
    
    this->enable();
    this->transfer_byte(0x00 | 0x80); // Write FIFO
    for (uint8_t b : data) this->transfer_byte(b);
    this->disable();
    
    this->write_reg(0x22, data.size());
    this->write_reg(0x40, 0x40); // DIO0 = TX Done
    this->write_reg(0x01, 0x83); // TX Mode
}

void Ra02Lora::loop() {
    uint32_t now = millis();

    // 1. Logika odesílání "majáku"
    if (!this->waiting_for_cad_ && (now - this->last_transmission_ > this->interval_)) {
        this->start_cad();
    }

    // 2. Obsluha DIO0 (IRQ)
    if (this->dio0_pin_->digital_read()) {
        uint8_t irq = this->read_reg(0x12);

        // --- PRIORITA 1: CAD Done (bit 2 v registru 0x12) ---
        if (irq & 0x04) { 
            this->write_reg(0x12, 0xFF); // Smazat IRQ hned

            if (irq & 0x01) { // CAD Detected (Kanal je obsazen)
                ESP_LOGW(TAG, "Kanal obsazen, odklad...");
                this->interval_ = 500 + (random_uint32() % 500);
                
                // Nevysíláme, takže se musíme hned vrátit do příjmu
                this->write_reg(0x01, 0x85); 
                this->write_reg(0x40, 0x00); // DIO0 zpět na RXDone
                
            } else { // Cisto (Budeme vysílat)
                ESP_LOGW(TAG, "Kanal volny, odesilam paket.");
                uint8_t current_rssi = this->read_reg(0x1B);
                ESP_LOGI(TAG, "RSSI pred vysilanim: %d dBm", current_rssi - 164);
                
                // Funkce send_packet modul přepne do TX (0x83)
                this->send_packet({0x55, 0xAA, 0x01});
                this->interval_ = 10000 + (random_uint32() % 2000);
                
                // ZDE SE NIC DALŠÍHO NEPŘEPÍNÁ! 
                // Modul teď vysílá. Necháme ho v TX režimu, dokud nevyvolá TX Done.
            }
            this->waiting_for_cad_ = false;
            this->last_transmission_ = now;
        } 
        // --- PRIORITA 2: RX Done (bit 6) ---
        else if (irq & 0x40) { 
            uint8_t len = this->read_reg(0x13);
            this->write_reg(0x0D, this->read_reg(0x10)); // RX Base do Pointeru
            
            this->enable();
            this->transfer_byte(0x00); // Čtení FIFO
            std::string out = "";
            for(int i = 0; i < len; i++) {
                char b[5]; sprintf(b, "%02X ", this->transfer_byte(0x00));
                out += b;
            }
            this->disable();
            
            // Doplnění toho vašeho RSSI z přijatého paketu
            int16_t pkt_rssi = (int16_t)this->read_reg(0x1B) - 164;
            ESP_LOGI(TAG, "Prijato HEX: [%s] (RSSI: %d dBm)", out.c_str(), pkt_rssi);
            
            this->write_reg(0x12, 0xFF); // Vyčistit IRQ
        }
        // --- PRIORITA 3: TX Done (bit 3) ---
        else if (irq & 0x08) { 
            // SEM se program dostane až cca sekundu po zavolání send_packet()
            ESP_LOGD(TAG, "Vysilani OK.");
            this->write_reg(0x12, 0xFF); // Vyčistit IRQ
            
            // AŽ TADY vracíme modul bezpečně zpět do příjmu!
            this->write_reg(0x01, 0x85); // Zpět do RX Continuous
            this->write_reg(0x40, 0x00); // DIO0 zpět na hlídání příjmu (RXDone)
        }
    }
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