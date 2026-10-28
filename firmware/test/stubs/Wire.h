#pragma once
#include <stddef.h>
#include <stdint.h>
class TwoWire {
 public:
  bool begin(int sda, int scl, uint32_t frequency = 0);
  bool end();
  void setTimeOut(uint16_t timeOutMillis);
  void beginTransmission(uint8_t address);
  size_t write(uint8_t data);
  uint8_t endTransmission(bool sendStop);
  uint8_t requestFrom(uint8_t address, uint8_t size);
  int available();
  int read();
};
extern TwoWire Wire;
