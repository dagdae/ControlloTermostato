// QUESTO FILE E' IDENTICO in firmware/controller/src e firmware/sensor_node/src.
#include "ds18b20.h"

uint8_t crc8Maxim(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; ++i) {
    uint8_t in = data[i];
    for (int b = 0; b < 8; ++b) {
      const uint8_t mix = (crc ^ in) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      in >>= 1;
    }
  }
  return crc;
}

bool ds18b20ParseScratchpad(const uint8_t sp[9], float* outC) {
  if (crc8Maxim(sp, 8) != sp[8]) return false;
  // Registro di configurazione: 0 R1 R0 1 1 1 1 1. Scarta letture tutte 0x00
  // (bus in corto, il CRC di zeri e' zero) o tutte 0xFF (nessun sensore).
  if ((sp[4] & 0x9F) != 0x1F) return false;
  int16_t raw = static_cast<int16_t>((static_cast<uint16_t>(sp[1]) << 8) | sp[0]);
  if (raw == 0x0550) return false;  // 85.0 C: valore di accensione
  const uint8_t res = (sp[4] >> 5) & 0x03;  // 0 = 9 bit ... 3 = 12 bit
  if (res == 0) raw &= ~7;
  else if (res == 1) raw &= ~3;
  else if (res == 2) raw &= ~1;
  const float t = static_cast<float>(raw) / 16.0f;
  if (t < -55.0f || t > 125.0f) return false;
  *outC = t;
  return true;
}

uint32_t ds18b20ConversionMs(uint8_t bits) {
  switch (bits) {
    case 9: return 94;
    case 10: return 188;
    case 11: return 375;
    default: return 750;
  }
}

#if defined(ARDUINO)
#include <Arduino.h>
#include <driver/gpio.h>

// Tempi presi dalla libreria OneWire (collaudati), Maxim AN126.
static portMUX_TYPE s_owMux = portMUX_INITIALIZER_UNLOCKED;

void Ds18b20::begin() {
  const gpio_num_t g = static_cast<gpio_num_t>(pin_);
  gpio_reset_pin(g);
  gpio_set_pull_mode(g, GPIO_FLOATING);
  gpio_set_direction(g, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_set_level(g, 1);
}

void Ds18b20::release() {
  const gpio_num_t g = static_cast<gpio_num_t>(pin_);
  gpio_set_direction(g, GPIO_MODE_INPUT);
  gpio_set_pull_mode(g, GPIO_FLOATING);
}

bool Ds18b20::reset() {
  const gpio_num_t g = static_cast<gpio_num_t>(pin_);
  gpio_set_level(g, 1);
  // Attende che la linea sia alta (max ~250 us), altrimenti il bus e' in corto.
  for (int i = 0; !gpio_get_level(g); ++i) {
    if (i > 125) return false;
    delayMicroseconds(2);
  }
  gpio_set_level(g, 0);
  delayMicroseconds(480);
  portENTER_CRITICAL(&s_owMux);
  gpio_set_level(g, 1);
  delayMicroseconds(70);
  const bool presence = gpio_get_level(g) == 0;
  portEXIT_CRITICAL(&s_owMux);
  delayMicroseconds(410);
  return presence;
}

void Ds18b20::writeBit(bool bit) {
  const gpio_num_t g = static_cast<gpio_num_t>(pin_);
  portENTER_CRITICAL(&s_owMux);
  gpio_set_level(g, 0);
  if (bit) {
    delayMicroseconds(10);
    gpio_set_level(g, 1);
    delayMicroseconds(55);
  } else {
    delayMicroseconds(65);
    gpio_set_level(g, 1);
    delayMicroseconds(5);
  }
  portEXIT_CRITICAL(&s_owMux);
}

bool Ds18b20::readBit() {
  const gpio_num_t g = static_cast<gpio_num_t>(pin_);
  portENTER_CRITICAL(&s_owMux);
  gpio_set_level(g, 0);
  delayMicroseconds(3);
  gpio_set_level(g, 1);
  delayMicroseconds(10);
  const bool r = gpio_get_level(g) != 0;
  portEXIT_CRITICAL(&s_owMux);
  delayMicroseconds(53);
  return r;
}

void Ds18b20::writeByte(uint8_t v) {
  for (int i = 0; i < 8; ++i) {
    writeBit(v & 0x01);
    v >>= 1;
  }
}

uint8_t Ds18b20::readByte() {
  uint8_t v = 0;
  for (int i = 0; i < 8; ++i) {
    if (readBit()) v |= static_cast<uint8_t>(1u << i);
  }
  return v;
}

bool Ds18b20::present() { return reset(); }

bool Ds18b20::setResolution(uint8_t bits) {
  if (bits < 9) bits = 9;
  if (bits > 12) bits = 12;
  if (!reset()) return false;
  writeByte(0xCC);  // Skip ROM: un solo sensore sul bus
  writeByte(0x4E);  // Write Scratchpad: TH, TL, configurazione
  writeByte(0x4B);
  writeByte(0x46);
  writeByte(static_cast<uint8_t>(0x1F | ((bits - 9) << 5)));
  return true;
}

bool Ds18b20::startConversion() {
  if (!reset()) return false;
  writeByte(0xCC);
  writeByte(0x44);  // Convert T
  return true;
}

bool Ds18b20::conversionDone() { return readBit(); }

bool Ds18b20::readTemperature(float* outC) {
  if (!reset()) return false;
  writeByte(0xCC);
  writeByte(0xBE);  // Read Scratchpad
  uint8_t sp[9];
  for (int i = 0; i < 9; ++i) sp[i] = readByte();
  return ds18b20ParseScratchpad(sp, outC);
}
#endif
