#pragma once
#include <stddef.h>
#include <stdint.h>
class Preferences {
 public:
  bool begin(const char* name, bool readOnly = false, const char* partition_label = nullptr);
  void end();
  bool clear();
  bool isKey(const char* key);
  size_t putUChar(const char* key, uint8_t value);
  size_t putUShort(const char* key, uint16_t value);
  size_t putULong(const char* key, uint32_t value);
  size_t putFloat(const char* key, float value);
  size_t putBool(const char* key, bool value);
  size_t putString(const char* key, const char* value);
  uint8_t getUChar(const char* key, uint8_t defaultValue = 0);
  uint16_t getUShort(const char* key, uint16_t defaultValue = 0);
  uint32_t getULong(const char* key, uint32_t defaultValue = 0);
  float getFloat(const char* key, float defaultValue = 0.0f);
  bool getBool(const char* key, bool defaultValue = false);
  size_t getString(const char* key, char* value, size_t maxLen);
};
