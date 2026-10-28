#include "sensor_math.h"

#include <math.h>

namespace sm {

uint8_t crc8Smbus(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

bool mlxRawToCelsius(uint16_t raw, float* outC) {
  if (raw & 0x8000) return false;
  *outC = static_cast<float>(raw) * 0.02f - 273.15f;
  return true;
}

float irCorrect(float measuredC, float ambientC, float emissivity) {
  if (isnan(measuredC) || isnan(ambientC) || !(emissivity > 0.05f)) return NAN;
  if (emissivity >= 0.999f) return measuredC;
  const double tm = measuredC + 273.15;
  const double ta = ambientC + 273.15;
  const double tm4 = tm * tm * tm * tm;
  const double ta4 = ta * ta * ta * ta;
  const double v = (tm4 - (1.0 - emissivity) * ta4) / emissivity;
  if (v <= 0.0) return NAN;
  return static_cast<float>(sqrt(sqrt(v)) - 273.15);
}

float irEmissivityFromReference(float measuredC, float ambientC, float realC) {
  if (isnan(measuredC) || isnan(ambientC) || isnan(realC)) return NAN;
  if (realC - ambientC < 8.0f) return NAN;  // termosifone troppo vicino alla temperatura ambiente
  const double tm = measuredC + 273.15;
  const double ta = ambientC + 273.15;
  const double tr = realC + 273.15;
  const double num = tm * tm * tm * tm - ta * ta * ta * ta;
  const double den = tr * tr * tr * tr - ta * ta * ta * ta;
  if (num <= 0.0 || den <= 0.0) return NAN;
  double e = num / den;
  if (e < 0.2) e = 0.2;
  if (e > 1.0) e = 1.0;
  return static_cast<float>(e);
}

float Median5::add(float v) {
  buf_[idx_] = v;
  idx_ = (idx_ + 1) % 5;
  if (n_ < 5) ++n_;
  float tmp[5];
  for (int i = 0; i < n_; ++i) tmp[i] = buf_[i];
  for (int i = 1; i < n_; ++i) {
    float x = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j] > x) {
      tmp[j + 1] = tmp[j];
      --j;
    }
    tmp[j + 1] = x;
  }
  return tmp[n_ / 2];
}

}  // namespace sm
