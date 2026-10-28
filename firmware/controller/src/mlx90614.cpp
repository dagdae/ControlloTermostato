#include "mlx90614.h"

#include "sensor_math.h"

bool Mlx90614::readWord(uint8_t reg, uint16_t* out) {
  if (wire_ == nullptr) return false;
  wire_->beginTransmission(address_);
  wire_->write(reg);
  if (wire_->endTransmission(false) != 0) return false;  // restart, senza stop
  if (wire_->requestFrom(address_, static_cast<uint8_t>(3)) != 3) {
    while (wire_->available()) wire_->read();
    return false;
  }
  const uint8_t lsb = static_cast<uint8_t>(wire_->read());
  const uint8_t msb = static_cast<uint8_t>(wire_->read());
  const uint8_t pec = static_cast<uint8_t>(wire_->read());
  const uint8_t frame[5] = {static_cast<uint8_t>(address_ << 1), reg,
                            static_cast<uint8_t>((address_ << 1) | 1), lsb, msb};
  if (sm::crc8Smbus(frame, sizeof(frame)) != pec) return false;
  *out = static_cast<uint16_t>((static_cast<uint16_t>(msb) << 8) | lsb);
  return true;
}

bool Mlx90614::readTemp(uint8_t reg, float* outC) {
  uint16_t raw = 0;
  if (!readWord(reg, &raw)) return false;
  return sm::mlxRawToCelsius(raw, outC);
}
