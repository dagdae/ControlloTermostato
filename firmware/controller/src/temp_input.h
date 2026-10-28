// Lettura della temperatura del termosifone dalla sorgente scelta nelle impostazioni.
#pragma once

#include <math.h>
#include <stdint.h>

#include "ds18b20.h"
#include "mlx90614.h"
#include "sensor_math.h"
#include "settings.h"

struct TempReading {
  bool valid = false;
  float tempC = NAN;     // valore finale usato dal controllo (corretto)
  float rawC = NAN;      // infrarossi: lettura con emissivita' 1.0 (serve alla calibrazione)
  float ambientC = NAN;  // infrarossi: temperatura del sensore (~ stanza)
  int nodes = 0;         // nodo remoto: quanti nodi recenti
  const char* error = "";
};

class TempInput {
 public:
  void begin(const AppSettings& s);
  // Da richiamare quando cambiano le impostazioni.
  void reconfigure(const AppSettings& s);
  // Da chiamare spesso; restituisce true quando *out contiene una nuova lettura.
  bool poll(uint64_t nowMs, TempReading* out);

 private:
  bool pollInfrared(uint64_t nowMs, TempReading* out);
  bool pollWired(uint64_t nowMs, TempReading* out);
  bool pollRemote(uint64_t nowMs, TempReading* out);
  void startI2c();

  TempSource source_ = TempSource::InfraredMlx;
  float emissivity_ = 0.95f;
  float offsetC_ = 0.0f;
  uint64_t remoteTimeoutMs_ = 900000;

  Mlx90614 mlx_;
  bool i2cStarted_ = false;
  int irFailures_ = 0;
  sm::Median5 irMedian_;
  sm::Median5 irRawMedian_;

  Ds18b20 ds_{PIN_ONEWIRE};
  bool dsConverting_ = false;
  uint64_t dsConvStartMs_ = 0;

  uint64_t nextPollMs_ = 0;
  uint32_t lastPackets_ = 0;
};
