#include "temp_input.h"

#include <Arduino.h>
#include <Wire.h>

#include "espnow_link.h"

namespace {

const uint32_t kIrPeriodMs = 500;
const uint32_t kWiredPeriodMs = 2000;
const uint32_t kWiredConvMs = 800;  // 12 bit (valore di fabbrica del DS18B20)

bool plausible(float t) { return t > -30.0f && t < 120.0f; }

}  // namespace

void TempInput::begin(const AppSettings& s) {
  ds_.begin();
  reconfigure(s);
}

bool TempInput::reconfigure(const AppSettings& s) {
  const bool changed = s.source != source_;
  if (changed) {
    irMedian_.reset();
    irRawMedian_.reset();
    dsConverting_ = false;
    nextPollMs_ = 0;
  }
  source_ = s.source;
  emissivity_ = s.irEmissivity;
  offsetC_ = s.tempOffsetC;
  remoteTimeoutMs_ = static_cast<uint64_t>(s.timeoutRemoteS) * 1000ULL;
  if (source_ == TempSource::InfraredMlx && !i2cStarted_) startI2c();
  irMedian_.reset();  // nuova emissivita'/offset: riparte la mediana
  return changed;
}

void TempInput::startI2c() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);  // MLX90614: SMBus max 100 kHz
  Wire.setTimeOut(50);
  mlx_.begin(Wire);
  i2cStarted_ = true;
}

bool TempInput::poll(uint64_t nowMs, TempReading* out) {
  switch (source_) {
    case TempSource::InfraredMlx: return pollInfrared(nowMs, out);
    case TempSource::WiredDs18b20: return pollWired(nowMs, out);
    case TempSource::RemoteNode: return pollRemote(nowMs, out);
  }
  return false;
}

bool TempInput::pollInfrared(uint64_t nowMs, TempReading* out) {
  if (nowMs < nextPollMs_) return false;
  nextPollMs_ = nowMs + kIrPeriodMs;
  *out = TempReading();
  float obj = NAN;
  float amb = NAN;
  if (!mlx_.readObjectC(&obj) || !mlx_.readAmbientC(&amb) || !plausible(obj) || !plausible(amb)) {
    out->error = "MLX90614 non risponde (cavi SDA/SCL, alimentazione 3.3 V)";
    if (++irFailures_ % 10 == 0) {
      Wire.end();  // riavvia il bus dopo errori ripetuti
      startI2c();
    }
    return true;
  }
  irFailures_ = 0;
  const float corrected = sm::irCorrect(obj, amb, emissivity_);
  if (isnan(corrected)) {
    out->error = "lettura infrarossi non valida";
    return true;
  }
  out->valid = true;
  out->tempC = irMedian_.add(corrected) + offsetC_;
  out->rawC = irRawMedian_.add(obj);
  out->ambientC = amb;
  return true;
}

bool TempInput::pollWired(uint64_t nowMs, TempReading* out) {
  if (!dsConverting_) {
    if (nowMs < nextPollMs_) return false;
    nextPollMs_ = nowMs + kWiredPeriodMs;
    if (ds_.startConversion()) {
      dsConverting_ = true;
      dsConvStartMs_ = nowMs;
      return false;
    }
    *out = TempReading();
    out->error = "DS18B20 non risponde (cavo, resistenza 4.7k, alimentazione)";
    return true;
  }
  if (nowMs - dsConvStartMs_ < kWiredConvMs) return false;
  dsConverting_ = false;
  *out = TempReading();
  float t = NAN;
  if (!ds_.readTemperature(&t) || !plausible(t)) {
    out->error = "lettura DS18B20 non valida (CRC o valore di accensione)";
    return true;
  }
  out->valid = true;
  out->tempC = t + offsetC_;
  return true;
}

bool TempInput::pollRemote(uint64_t nowMs, TempReading* out) {
  const uint32_t packets = espnow_link::packetCount();
  if (packets == lastPackets_) return false;
  lastPackets_ = packets;
  *out = TempReading();
  float t = NAN;
  int fresh = 0;
  if (!espnow_link::maxTemperature(nowMs, remoteTimeoutMs_, &t, &fresh)) {
    out->nodes = fresh;
    out->error = fresh > 0 ? "il nodo segnala un errore del sensore" : "nessun nodo recente";
    return true;
  }
  out->valid = true;
  out->tempC = t + offsetC_;
  out->nodes = fresh;
  return true;
}
