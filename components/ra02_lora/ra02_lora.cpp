#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora";

void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    
    // Hardwarový reset pinu nepotřebujeme, necháme jen DIO nezapojené
    this->dio0_pin_->setup(); // VYMAZÁNO

    // Hardwarový reset SX1278
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);

    if (this->read_reg(0x42) != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen!");
        this->mark_failed();
        return;
    }

    // 2. Registrace ISR handleru přes ESPHome abstrakci
    this->dio0_pin_->attach_interrupt(
      &Ra02Lora::gpio_intr_handler, // Statická nebo staticky dostupná funkce
      this,                            // Argument předaný do handleru
      gpio::INTERRUPT_RISING_EDGE      // Typ hrany
    );

    // 1. ZÁKLADNÍ NASTAVENÍ
    this->write_reg(0x01, 0x80); // Sleep
    delay(10);
    this->write_reg(0x01, 0x81); // Standby

    // 2. FREKVENCE 434.0 MHz
    this->write_reg(0x06, 0x6C); 
    this->write_reg(0x07, 0x80); 
    this->write_reg(0x08, 0x00);

    // 3. VÝKON VYSÍLAČE (Tohle nám celou dobu chybělo!)
    // 0x8F = Zapne PA_BOOST pin (na kterém je anténa) na maximální výkon
    this->write_reg(0x09, 0x8F); 

    // 4. MODEM (SF7 = rychlé, BW 125, CRC On)
    this->write_reg(0x1D, 0x72); 
    this->write_reg(0x1E, 0x74); 
    this->write_reg(0x39, 0x12); // Sync Word

    //DIO0
    this->write_reg(0x40, 0x00);

    // 5. START DO PŘÍJMU
    this->write_reg(0x01, 0x85); // RX Continuous
    ESP_LOGI(TAG, ">>> LORA BARE-METAL START <<<");
}

// Musí být označen jako IRAM_ATTR pro běh z RAM
void IRAM_ATTR Ra02Lora::gpio_intr_handler(Ra02Lora *arg) {
  // Minimální logika (např. nastavení flagu nebo zápis do fronty)
  arg->interrupt_triggered_ = true;
};

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
    
    // Přečteme si stavové vlajky rovnou z čipu
    uint8_t irq = this->read_reg(0x12);

    // --- A. SEKCE PŘÍJMU (Vyhodnocujeme vždy) ---
    if (irq & 0x40) { // RX Done
        uint8_t len = this->read_reg(0x13);
        this->write_reg(0x0D, this->read_reg(0x10)); // Nastavit ukazatel na začátek dat
        
        this->enable();
        this->transfer_byte(0x00); // Čtení FIFO
        std::string out = "";
        for(int i = 0; i < len; i++) {
            char b[5]; sprintf(b, "%02X ", this->transfer_byte(0x00));
            out += b;
        }
        this->disable();
        
        int16_t rssi = (int16_t)this->read_reg(0x1B) - 164;
        ESP_LOGI(TAG, "===== PRIJATO: [%s] (RSSI: %d dBm) =====", out.c_str(), rssi);
        
        this->write_reg(0x12, 0xFF); // Vyčistit vlajky
    }
    else if (irq & 0x20) { // CRC Error (Něco jsme zaslechli, ale je to rozbité)
        ESP_LOGW(TAG, "Prijato rozbite (CRC Error)!");
        this->write_reg(0x12, 0xFF); // Vyčistit
    }
    else if (irq & 0x08) { // TX Done (Vysílání skončilo)
        ESP_LOGD(TAG, "Vysilani hardwarove dokonceno.");
        this->write_reg(0x12, 0xFF); // Vyčistit
        this->write_reg(0x01, 0x85); // Okamžitě zpět do příjmu
    }

    // --- B. SEKCE VYSÍLÁNÍ (Každých 10 vteřin) ---
    // !!! U PŘIJÍMAČE ZAKOMENTUJTE CELÝ TENTO IF BLOK !!!
    if (now - this->last_transmission_ > 10000) {
        ESP_LOGI(TAG, "Odesilam paket (DE AD BE EF)...");
        this->send_packet({0xDE, 0xAD, 0xBE, 0xEF});
/*        
        this->write_reg(0x01, 0x81); // Standby
        this->write_reg(0x0D, 0x80); // TX Base
        this->write_reg(0x0F, 0x80); // Pointer
        
        this->enable();
        this->transfer_byte(0x00 | 0x80); // Write FIFO
        this->transfer_byte(0xDE);
        this->transfer_byte(0xAD);
        this->transfer_byte(0xBE);
        this->transfer_byte(0xEF);
        this->disable();
        
        this->write_reg(0x22, 4);    // Délka 4 bajty
        this->write_reg(0x01, 0x83); // Start TX
*/        
        this->last_transmission_ = now;
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