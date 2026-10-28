// Conversione per termistore NTC (modello Beta), senza dipendenze da Arduino.
#pragma once

#include <math.h>

// Partitore: VCC -- R fissa -- (ADC) -- NTC -- GND.
// Restituisce la resistenza dell'NTC in ohm, NAN se la tensione e' fuori scala.
inline float ntcResistanceOhm(float vAdcMv, float vSupplyMv, float rFixedOhm) {
  if (!(vAdcMv > 1.0f) || !(vAdcMv < vSupplyMv - 1.0f)) return NAN;
  return rFixedOhm * vAdcMv / (vSupplyMv - vAdcMv);
}

// 1/T = 1/T25 + ln(R/R25)/Beta
inline float ntcCelsius(float rOhm, float r25Ohm, float beta) {
  if (!(rOhm > 0.0f)) return NAN;
  const float invT = 1.0f / 298.15f + logf(rOhm / r25Ohm) / beta;
  return 1.0f / invT - 273.15f;
}
