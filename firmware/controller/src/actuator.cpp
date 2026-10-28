#include "actuator.h"

#include <Arduino.h>

namespace {

const uint32_t kServoFreqHz = 50;
const uint8_t kServoResolutionBits = 14;  // supportata da tutti gli ESP32
const uint32_t kServoMoveMs = 1000;       // tempo concesso al servo per arrivare in posizione
const uint32_t kReleaseMs = 500;
const uint32_t kMinPressGapMs = 3000;     // pausa minima tra due pressioni
#if ESP_ARDUINO_VERSION_MAJOR < 3
const uint8_t kServoChannel = 0;
#endif

void servoPower(bool on) {
#if PIN_SERVO_POWER >= 0
  pinMode(PIN_SERVO_POWER, OUTPUT);
  digitalWrite(PIN_SERVO_POWER, on ? HIGH : LOW);
#else
  (void)on;
#endif
}

}  // namespace

void Actuator::begin(const AppSettings& s, uint64_t nowMs) {
  thermoOn_ = s.thermostatAssumedOn;
  reconfigure(s);
  apply(true, nowMs);
}

void Actuator::reconfigure(const AppSettings& s) {
  const bool wasRelay = isRelay();
  const bool typeChanged = s.actuator != type_;
  type_ = s.actuator;
  activeLow_ = s.relayActiveLow;
  allowDeg_ = s.servoAllowDeg;
  blockDeg_ = s.servoBlockDeg;
  restDeg_ = s.servoRestDeg;
  pressDeg_ = s.servoPressDeg;
  pressMs_ = s.servoPressMs;
  if (typeChanged) {
    if (servoAttached_) servoOff();
    phase_ = Phase::Idle;
    if (wasRelay && !isRelay()) relayRelease();
  }
  if (!servoAttached_) servoPower(false);
  if (!isRelay()) relayRelease();
  initialized_ = false;  // riscrive l'uscita al prossimo apply()
}

bool Actuator::takeThermostatChanged() {
  const bool c = thermoChanged_;
  thermoChanged_ = false;
  return c;
}

void Actuator::relayWrite(bool allow) {
  const bool energize = type_ == ActuatorType::RelayEnergizeToBlock ? !allow : allow;
  const bool level = activeLow_ ? !energize : energize;
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, level ? HIGH : LOW);
}

void Actuator::relayRelease() {
  // Rele' diseccitato.
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, activeLow_ ? HIGH : LOW);
}

void Actuator::servoOn() {
  if (servoAttached_) return;
  servoPower(true);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_SERVO, kServoFreqHz, kServoResolutionBits);
#else
  ledcSetup(kServoChannel, kServoFreqHz, kServoResolutionBits);
  ledcAttachPin(PIN_SERVO, kServoChannel);
#endif
  servoAttached_ = true;
}

void Actuator::servoOff() {
  if (!servoAttached_) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcDetach(PIN_SERVO);
#else
  ledcDetachPin(PIN_SERVO);
#endif
  // Niente impulsi: il servo resta fermo senza consumare e senza ronzare.
  pinMode(PIN_SERVO, OUTPUT);
  digitalWrite(PIN_SERVO, LOW);
  servoPower(false);
  servoAttached_ = false;
}

void Actuator::servoWriteDeg(uint8_t deg) {
  if (deg > 180) deg = 180;
  const uint32_t us = 500u + static_cast<uint32_t>(deg) * 2000u / 180u;  // 0.5..2.5 ms
  const uint32_t duty = us * ((1u << kServoResolutionBits) - 1u) / 20000u;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_SERVO, duty);
#else
  ledcWrite(kServoChannel, duty);
#endif
}

void Actuator::apply(bool allow, uint64_t nowMs) {
  switch (type_) {
    case ActuatorType::RelayEnergizeToBlock:
    case ActuatorType::RelayEnergizeToAllow:
      if (!initialized_ || allow != target_) {
        target_ = allow;
        relayWrite(allow);
        initialized_ = true;
      }
      return;

    case ActuatorType::ServoPosition:
      if (phase_ == Phase::Moving && nowMs >= phaseUntilMs_) {
        servoOff();
        phase_ = Phase::Idle;
      }
      if (!initialized_ || allow != target_) {
        target_ = allow;
        initialized_ = true;
        servoOn();
        servoWriteDeg(allow ? allowDeg_ : blockDeg_);
        phase_ = Phase::Moving;
        phaseUntilMs_ = nowMs + kServoMoveMs;
      }
      return;

    case ActuatorType::ServoPress:
      target_ = allow;
      initialized_ = true;
      if (phase_ == Phase::Pressing && nowMs >= phaseUntilMs_) {
        servoWriteDeg(restDeg_);
        phase_ = Phase::Releasing;
        phaseUntilMs_ = nowMs + kReleaseMs;
      } else if (phase_ == Phase::Releasing && nowMs >= phaseUntilMs_) {
        servoOff();
        phase_ = Phase::Idle;
        lastPressMs_ = nowMs;
      }
      if (phase_ == Phase::Idle && target_ != thermoOn_ &&
          (lastPressMs_ == 0 || nowMs - lastPressMs_ >= kMinPressGapMs)) {
        servoOn();
        servoWriteDeg(pressDeg_);
        phase_ = Phase::Pressing;
        phaseUntilMs_ = nowMs + pressMs_;
        thermoOn_ = target_;  // si presume che ogni pressione inverta il termostato
        thermoChanged_ = true;
      }
      return;
  }
}
