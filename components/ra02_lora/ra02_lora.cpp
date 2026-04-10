#include "ra02_lora.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ra02_lora {

static const char *const TAG = "ra02_lora";

void Ra02Lora::setup() {
    this->spi_setup();
    this->reset_pin_->setup();
    this->dio0_pin_->setup();
    
    // Hardwarový reset
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10); // Krátká pauza po resetu je jistota

    // Kontrola komunikace
    uint8_t version = this->read_reg(0x42);
    if (version != 0x12) {
        ESP_LOGE(TAG, "SX1278 nenalezen! Registr 0x42 vratil: 0x%02X", version);
        this->mark_failed();
        return;
    }

    ESP_LOGI(TAG, "SX1278 uspesne inicializovan, verze: 0x%02X", version);

    // 1. Přepnutí do SLEEP módu (nutné pro změnu na LoRa)
    this->write_reg(0x01, 0x00); // RegOpMode -> Sleep
    delay(10);

    // 2. Zapnutí LoRa módu
    this->write_reg(0x01, 0x80); // RegOpMode -> bit LoRa zapnut
    delay(10);

    // 3. Nastavení frekvence (příklad pro 433 MHz)
    // Výpočet: f_step = 61.035 Hz. 433MHz / f_step = 7094272 -> 0x6C4000
    this->write_reg(0x06, 0x6C); // RegFrfMsb
    this->write_reg(0x07, 0x40); // RegFrfMid
    this->write_reg(0x08, 0x00); // RegFrfLsb

    // 4. Přepnutí do STANDBY, abychom viděli, že drží
    this->write_reg(0x01, 0x81); // RegOpMode -> LoRa Standby
    delay(10);

    uint8_t mode = this->read_reg(0x01);
    ESP_LOGI(TAG, "Rezim nastaven na: 0x%02X (mělo by být 0x81)", mode);

    // 1. Nastavení Bandwidth (125 kHz) a Coding Rate (4/5)
    // Registr 0x1D: BW (bity 7-4), CR (bity 3-1), Implicit Header (bit 0)
    this->write_reg(0x1D, 0x72); // 0x70 = 125kHz, 0x02 = CR 4/5
    
    this->write_reg(0x39, 0x34);  //Původně nic
    // 2. Nastavení Spreading Factor (SF7)
    // Registr 0x1E: SF (bity 7-4), CRC On (bit 2)
    this->write_reg(0x1E, 0xC4); // 0x70 = SF7, 0x04 = Payload CRC zapnuto původně 0x74

    // 3. Nastavení LNA (zesilovač nízkého šumu) pro lepší citlivost
    this->write_reg(0x0C, 0x23); // LNA gain na max, LNA boost zapnut

    // 1. Nastavení DIO0 pinu, aby vyvolal přerušení při příjmu (PayloadReady)
    // Registr 0x40: Mapování DIO pinů. 0x00 znamená DIO0 -> RXDONE
    this->write_reg(0x40, 0x00);

    // 2. Nastavení adresy v paměti FIFO (kde začíná RX buffer)
    this->write_reg(0x0D, 0x00); // FIFO base address
    this->write_reg(0x0F, 0x00); // SPI pointer na začátek

    // 3. Přepnutí do režimu RX Continuous (stálý příjem)
    this->write_reg(0x01, 0x85); 

    ESP_LOGI(TAG, "SX1278 prepnut do rezimu nepretrziteho prijmu (RX)...");

}

void Ra02Lora::loop() {
// ESP_LOGD(TAG, "Stav DIO0 pinu: %d", this->dio0_pin_->digital_read());
  if (this->dio0_pin_->digital_read()) {
    uint8_t irq_flags = this->read_reg(0x12);
    
    if (irq_flags & 0x40) { // RX Done
      uint8_t length = this->read_reg(0x13);
      uint8_t current_addr = this->read_reg(0x10);
      this->write_reg(0x0D, current_addr);
      
      this->enable();
      this->transfer_byte(0x00); // Adresa FIFO pro čtení
      
      char hex_buffer[5];
      std::string hex_output = "";
      
      for (int i = 0; i < length; i++) {
          uint8_t b = this->transfer_byte(0x00);
          sprintf(hex_buffer, "%02X ", b); // Převede bajt na "FF "
          hex_output += hex_buffer;
      }
      this->disable();

      int8_t rssi = this->read_reg(0x1B) - 164;
      // Výpis v HEX formátu
      ESP_LOGI(TAG, "Paket HEX: [ %s] (Delka: %d, RSSI: %d dBm)", 
               hex_output.c_str(), length, rssi);

      this->write_reg(0x12, 0xFF); // Reset IRQ příznaků
    }
  }
}

void Ra02Lora::dump_config() {
    ESP_LOGCONFIG(TAG, "Ra02 LoRa Component");
    LOG_PIN("  Reset Pin: ", this->reset_pin_);
    LOG_PIN("  DIO0 Pin: ", this->dio0_pin_);
}

void Ra02Lora::write_reg(uint8_t reg, uint8_t val) {
    this->enable();
    this->transfer_byte(reg | 0x80); // MSB = 1 pro zápis
    this->transfer_byte(val);
    this->disable();
}

uint8_t Ra02Lora::read_reg(uint8_t reg) {
    this->enable();
    this->transfer_byte(reg & 0x7F); // MSB = 0 pro čtení
    uint8_t val = this->transfer_byte(0x00);
    this->disable();
    return val;
}

} // namespace ra02_lora
} // namespace esphome