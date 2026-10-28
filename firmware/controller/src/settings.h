// Impostazioni modificabili dalla pagina web, salvate nella memoria NVS.
#pragma once

#include <stdint.h>

#include "config.h"
#include "control_logic.h"

enum class TempSource : uint8_t {
  InfraredMlx = 0,   // MLX90614 puntato sul termosifone da 1-3 m
  WiredDs18b20 = 1,  // sonda DS18B20 a contatto collegata con un cavo
  RemoteNode = 2,    // nodo a batteria sul termosifone (ESP-NOW)
};

enum class ActuatorType : uint8_t {
  RelayEnergizeToBlock = 0,  // contatto NC in serie al termostato: rele' eccitato = riscaldamento bloccato
  RelayEnergizeToAllow = 1,  // contatto NO in serie al termostato: rele' eccitato = riscaldamento consentito
  ServoPosition = 2,         // servo con due posizioni (cursore, manopola, interruttore)
  ServoPress = 3,            // servo che preme un pulsante ON/OFF (ogni pressione inverte)
};

struct AppSettings {
  ctl::Params ctl;
  ctl::Mode mode = ctl::Mode::Auto;  // "Acceso forzato" non viene salvato
  ctl::State savedState = ctl::State::Heating;  // per riprendere pausa o blocco dopo un riavvio

  TempSource source = TempSource::InfraredMlx;
  float irEmissivity = 0.95f;
  float tempOffsetC = 0.0f;
  uint16_t timeoutLocalS = 120;
  uint16_t timeoutRemoteS = 900;
  uint16_t nodeFastS = 30;
  uint16_t nodeSlowS = 120;

  ActuatorType actuator = ActuatorType::RelayEnergizeToBlock;
  bool relayActiveLow = false;
  uint8_t servoAllowDeg = 30;
  uint8_t servoBlockDeg = 120;
  uint8_t servoRestDeg = 90;
  uint8_t servoPressDeg = 60;
  uint16_t servoPressMs = 400;
  bool thermostatAssumedOn = true;

  char wifiSsid[33] = "";
  char wifiPass[65] = "";
  char hostname[32] = DEFAULT_HOSTNAME;
  char webPass[33] = "";

  void sanitize();
};

const char* tempSourceName(TempSource s);
const char* actuatorName(ActuatorType a);

namespace settings {
void load(AppSettings& s);
void save(const AppSettings& s);
// Solo modalita', stato del ciclo e stato presunto del termostato (cambiano durante il funzionamento).
void saveRuntime(const AppSettings& s);
void factoryReset();
}  // namespace settings
