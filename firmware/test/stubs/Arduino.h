// Stub minimale di Arduino.h (core ESP32 3.x) per la compilazione di controllo.
#pragma once
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "esp_random.h"

#define ESP_ARDUINO_VERSION_MAJOR 3
#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define PROGMEM
#define PGM_P const char*
#define RTC_DATA_ATTR
typedef bool boolean;

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
uint32_t analogReadMilliVolts(uint8_t pin);
unsigned long millis();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);
bool ledcWrite(uint8_t pin, uint32_t duty);
bool ledcDetach(uint8_t pin);
void enableLoopWDT();
void feedLoopWDT();

class String {
 public:
  String() {}
  String(const char* s) : s_(s ? s : "") {}
  String(const String& o) = default;
  String& operator=(const String& o) = default;
  explicit String(char c) : s_(1, c) {}
  explicit String(int v) : s_(std::to_string(v)) {}
  explicit String(unsigned int v) : s_(std::to_string(v)) {}
  explicit String(long v) : s_(std::to_string(v)) {}
  explicit String(unsigned long v) : s_(std::to_string(v)) {}
  explicit String(float v, unsigned int decimals = 2) : s_(fmt(v, decimals)) {}
  explicit String(double v, unsigned int decimals = 2) : s_(fmt(v, decimals)) {}
  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  String& operator+=(const char* o) { s_ += o; return *this; }
  String& operator+=(char c) { s_ += c; return *this; }
  String& operator+=(unsigned char v) { s_ += std::to_string(v); return *this; }
  String& operator+=(int v) { s_ += std::to_string(v); return *this; }
  String& operator+=(unsigned int v) { s_ += std::to_string(v); return *this; }
  String& operator+=(long v) { s_ += std::to_string(v); return *this; }
  String& operator+=(unsigned long v) { s_ += std::to_string(v); return *this; }
  bool operator==(const char* o) const { return s_ == o; }
  bool operator==(const String& o) const { return s_ == o.s_; }
  unsigned int length() const { return static_cast<unsigned int>(s_.size()); }
  const char* c_str() const { return s_.c_str(); }
  long toInt() const { return atol(s_.c_str()); }
  float toFloat() const { return static_cast<float>(atof(s_.c_str())); }
  bool reserve(unsigned int n) { s_.reserve(n); return true; }

 private:
  static std::string fmt(double v, unsigned int d) {
    char b[48];
    snprintf(b, sizeof(b), "%.*f", static_cast<int>(d), v);
    return b;
  }
  std::string s_;
};
inline String operator+(const String& a, const String& b) { String r(a); r += b; return r; }
inline String operator+(const String& a, const char* b) { String r(a); r += b; return r; }
inline String operator+(const char* a, const String& b) { String r(a); r += b; return r; }

class Print {
 public:
  size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
  size_t println(const char* s);
};
class HardwareSerial : public Print {
 public:
  void begin(unsigned long baud);
  void flush();
};
extern HardwareSerial Serial;

class EspClass {
 public:
  void restart();
};
extern EspClass ESP;

// FreeRTOS (inclusi da Arduino.h nel core vero)
typedef void* QueueHandle_t;
typedef int BaseType_t;
typedef uint32_t TickType_t;
typedef void (*TaskFunction_t)(void*);
#define portMAX_DELAY 0xFFFFFFFFu
#define pdTRUE 1
QueueHandle_t xQueueCreate(uint32_t length, uint32_t itemSize);
BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t wait);
BaseType_t xQueueReceive(QueueHandle_t q, void* item, TickType_t wait);
BaseType_t xTaskCreate(TaskFunction_t fn, const char* name, uint32_t stack, void* arg, uint32_t prio, void* handle);
typedef struct {
  uint32_t owner;
  uint32_t count;
} portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED {0, 0}
void portENTER_CRITICAL(portMUX_TYPE* mux);
void portEXIT_CRITICAL(portMUX_TYPE* mux);

// Nomi dei pin delle schede XIAO (nel core vero stanno in pins_arduino.h della variante).
#if defined(ARDUINO_XIAO_ESP32C6) || defined(ARDUINO_XIAO_ESP32C3) || defined(ARDUINO_XIAO_ESP32S3)
static const uint8_t A0 = 0;
static const uint8_t D1 = 1;
static const uint8_t D2 = 2;
#endif
