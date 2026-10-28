// Calcoli per i sensori, senza dipendenze da Arduino (testati sul PC).
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace sm {

// CRC-8 SMBus (PEC del MLX90614): polinomio 0x07, valore iniziale 0.
uint8_t crc8Smbus(const uint8_t* data, size_t len);

// Registro RAM del MLX90614 (0.02 K per bit) -> gradi C.
// Restituisce false se il bit di errore (bit 15) e' attivo.
bool mlxRawToCelsius(uint16_t raw, float* outC);

// Il MLX90614 esce dalla fabbrica con emissivita' 1.0. Questa funzione corregge
// la lettura per un'emissivita' "efficace" (vernice del termosifone + parte del
// campo visivo occupata dal muro), assumendo che l'ambiente riflesso sia alla
// temperatura del sensore. Restituisce NAN se i dati non hanno senso.
float irCorrect(float measuredC, float ambientC, float emissivity);

// Emissivita' efficace che fa coincidere la lettura IR con una temperatura reale
// misurata a contatto (calibrazione). Restituisce NAN se la differenza tra
// termosifone e ambiente e' troppo piccola per calibrare con precisione.
float irEmissivityFromReference(float measuredC, float ambientC, float realC);

// Mediana delle ultime 5 letture: elimina i picchi isolati (es. una persona che
// passa davanti al sensore IR).
class Median5 {
 public:
  void reset() {
    n_ = 0;
    idx_ = 0;
  }
  float add(float v);
  int count() const { return n_; }

 private:
  float buf_[5] = {0, 0, 0, 0, 0};
  int n_ = 0;
  int idx_ = 0;
};

}  // namespace sm
