// Driver minimo per un solo DS18B20 su bus 1-Wire (nessuna libreria esterna).
// QUESTO FILE E' IDENTICO in firmware/controller/src e firmware/sensor_node/src:
// se lo modifichi, copialo anche nell'altra cartella (i test lo verificano).
#pragma once

#include <stddef.h>
#include <stdint.h>

// CRC-8 Dallas/Maxim (polinomio x^8 + x^5 + x^4 + 1).
uint8_t crc8Maxim(const uint8_t* data, size_t len);

// Decodifica lo scratchpad (9 byte). Restituisce false se il CRC o il formato
// non sono validi (bus in corto, sensore scollegato) o se il valore e' quello
// di accensione (85.0 C = conversione non avvenuta).
bool ds18b20ParseScratchpad(const uint8_t sp[9], float* outC);

// Tempo massimo di conversione da datasheet per la risoluzione data (9..12 bit).
uint32_t ds18b20ConversionMs(uint8_t bits);

#if defined(ARDUINO)
// Linea dati con resistenza di pull-up esterna da 4.7 kOhm (obbligatoria).
// Il sensore deve essere alimentato su VDD (non in modalita' parassita).
class Ds18b20 {
 public:
  explicit Ds18b20(int pin) : pin_(pin) {}
  void begin();
  bool present();
  // Solo in RAM: va riscritta ogni volta che il sensore viene riacceso.
  bool setResolution(uint8_t bits);
  bool startConversion();
  // Con alimentazione esterna il sensore risponde 1 a fine conversione.
  bool conversionDone();
  bool readTemperature(float* outC);
  // Linea in alta impedenza (prima di togliere alimentazione al sensore).
  void release();

 private:
  bool reset();
  void writeBit(bool bit);
  bool readBit();
  void writeByte(uint8_t v);
  uint8_t readByte();

  int pin_;
};
#endif
