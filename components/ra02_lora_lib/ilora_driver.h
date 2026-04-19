#pragma once
#include <vector>
#include <stdint.h>

//struct LoraPacket {
//    std::vector<uint8_t> data;
//    int16_t rssi;
//};

// Toto je naše rozhraní. "I" na začátku značí Interface.
class ILoraDriver {
 public:
  virtual ~ILoraDriver() = default;
  
  // Každý driver, který toto rozhraní implementuje, MUSÍ mít tyto metody:
  virtual bool available() = 0;
  virtual LoraPacket read_packet() = 0;
  virtual void send_packet(std::vector<uint8_t> data) = 0;
};