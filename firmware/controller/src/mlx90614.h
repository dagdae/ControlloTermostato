// Lettura minima del termometro a infrarossi MLX90614 (SMBus/I2C, max 100 kHz),
// con verifica del PEC (CRC-8) per scartare letture corrotte.
#pragma once

#include <Wire.h>
#include <stdint.h>

class Mlx90614 {
 public:
  static const uint8_t kDefaultAddress = 0x5A;

  void begin(TwoWire& wire, uint8_t address = kDefaultAddress) {
    wire_ = &wire;
    address_ = address;
  }
  // Temperatura dell'oggetto inquadrato (emissivita' di fabbrica 1.0).
  bool readObjectC(float* outC) { return readTemp(0x07, outC); }
  // Temperatura del sensore stesso (~ temperatura della stanza).
  bool readAmbientC(float* outC) { return readTemp(0x06, outC); }

 private:
  bool readTemp(uint8_t reg, float* outC);
  bool readWord(uint8_t reg, uint16_t* out);

  TwoWire* wire_ = nullptr;
  uint8_t address_ = kDefaultAddress;
};
