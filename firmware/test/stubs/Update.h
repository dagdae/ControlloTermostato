#pragma once
#include <stddef.h>
#include <stdint.h>
#include "Arduino.h"
#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF
class UpdateClass {
 public:
  bool begin(size_t size = UPDATE_SIZE_UNKNOWN);
  size_t write(uint8_t* data, size_t len);
  bool end(bool evenIfRemaining = false);
  bool hasError();
  bool isFinished();
  void printError(Print& out);
  void abort();
};
extern UpdateClass Update;
