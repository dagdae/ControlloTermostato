// Comando fisico: rele' in serie al contatto del termostato oppure servo ("ditino").
#pragma once

#include <stdint.h>

#include "settings.h"

class Actuator {
 public:
  // Porta subito l'uscita nello stato sicuro (riscaldamento consentito).
  void begin(const AppSettings& s, uint64_t nowMs);
  void reconfigure(const AppSettings& s);
  // Non bloccante: da chiamare a ogni giro del loop.
  void apply(bool allowHeating, uint64_t nowMs);

  bool commanded() const { return target_; }
  bool busy() const { return phase_ != Phase::Idle; }
  // Solo servo a pulsante: stato in cui si presume sia il termostato.
  bool thermostatAssumedOn() const { return thermoOn_; }
  void setThermostatAssumedOn(bool on) { thermoOn_ = on; }
  // true una volta dopo ogni pressione (per salvare lo stato presunto).
  bool takeThermostatChanged();

 private:
  enum class Phase : uint8_t { Idle, Moving, Pressing, Releasing };

  bool isRelay() const {
    return type_ == ActuatorType::RelayEnergizeToBlock || type_ == ActuatorType::RelayEnergizeToAllow;
  }
  void relayWrite(bool allow);
  void relayRelease();
  void servoOn();
  void servoOff();
  void servoWriteDeg(uint8_t deg);

  ActuatorType type_ = ActuatorType::RelayEnergizeToBlock;
  bool activeLow_ = false;
  uint8_t allowDeg_ = 30;
  uint8_t blockDeg_ = 120;
  uint8_t restDeg_ = 90;
  uint8_t pressDeg_ = 60;
  uint16_t pressMs_ = 400;

  bool initialized_ = false;
  bool target_ = true;
  bool thermoOn_ = true;
  bool thermoChanged_ = false;
  bool servoAttached_ = false;
  Phase phase_ = Phase::Idle;
  uint64_t phaseUntilMs_ = 0;
  uint64_t lastPressMs_ = 0;
};
