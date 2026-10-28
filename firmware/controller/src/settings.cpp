#include "settings.h"

#include <Preferences.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

namespace {

const char* kNamespace = "termo";

template <typename T>
void clampValue(T& v, T lo, T hi) {
  if (v < lo) v = lo;
  if (v > hi) v = hi;
}

void terminate(char* s, size_t n) { s[n - 1] = '\0'; }

void sanitizeHostname(char* h, size_t n) {
  terminate(h, n);
  size_t out = 0;
  for (size_t i = 0; h[i] != '\0' && out < n - 1; ++i) {
    const char c = static_cast<char>(tolower(static_cast<unsigned char>(h[i])));
    if (isalnum(static_cast<unsigned char>(c)) || (c == '-' && out > 0)) h[out++] = c;
  }
  h[out] = '\0';
  if (out == 0) strncpy(h, DEFAULT_HOSTNAME, n - 1);
}

void readString(Preferences& p, const char* key, char* dst, size_t n) {
  if (p.isKey(key)) p.getString(key, dst, n);
  terminate(dst, n);
}

}  // namespace

void AppSettings::sanitize() {
  ctl.sanitize();
  if (mode != ctl::Mode::Auto && mode != ctl::Mode::Bypass && mode != ctl::Mode::ForceOff) {
    mode = ctl::Mode::Auto;
  }
  if (savedState != ctl::State::Cooldown && savedState != ctl::State::Lockout) savedState = ctl::State::Heating;
  if (static_cast<uint8_t>(source) > static_cast<uint8_t>(TempSource::RemoteNode)) {
    source = TempSource::InfraredMlx;
  }
  if (!(irEmissivity >= 0.2f && irEmissivity <= 1.0f)) irEmissivity = 0.95f;
  if (!(tempOffsetC >= -20.0f && tempOffsetC <= 20.0f)) tempOffsetC = 0.0f;
  clampValue<uint16_t>(timeoutLocalS, 10, 3600);
  clampValue<uint16_t>(timeoutRemoteS, 60, 3600);
  clampValue<uint16_t>(nodeFastS, 10, 600);
  clampValue<uint16_t>(nodeSlowS, nodeFastS, 1800);
  if (static_cast<uint8_t>(actuator) > static_cast<uint8_t>(ActuatorType::ServoPress)) {
    actuator = ActuatorType::RelayEnergizeToBlock;
  }
  clampValue<uint8_t>(servoAllowDeg, 0, 180);
  clampValue<uint8_t>(servoBlockDeg, 0, 180);
  clampValue<uint8_t>(servoRestDeg, 0, 180);
  clampValue<uint8_t>(servoPressDeg, 0, 180);
  clampValue<uint16_t>(servoPressMs, 100, 3000);
  terminate(wifiSsid, sizeof(wifiSsid));
  terminate(wifiPass, sizeof(wifiPass));
  terminate(webPass, sizeof(webPass));
  sanitizeHostname(hostname, sizeof(hostname));
}

const char* tempSourceName(TempSource s) {
  switch (s) {
    case TempSource::InfraredMlx: return "Infrarossi MLX90614";
    case TempSource::WiredDs18b20: return "Sonda DS18B20 a filo";
    case TempSource::RemoteNode: return "Nodo a batteria (ESP-NOW)";
  }
  return "?";
}

const char* actuatorName(ActuatorType a) {
  switch (a) {
    case ActuatorType::RelayEnergizeToBlock: return "Rele' (NC: eccitato = blocca)";
    case ActuatorType::RelayEnergizeToAllow: return "Rele' (NO: eccitato = consente)";
    case ActuatorType::ServoPosition: return "Servo a due posizioni";
    case ActuatorType::ServoPress: return "Servo che preme un pulsante";
  }
  return "?";
}

namespace settings {

void load(AppSettings& s) {
  Preferences p;
  if (!p.begin(kNamespace, true)) {  // primo avvio: namespace non ancora creato
    s.sanitize();
    return;
  }
  ctl::Params& c = s.ctl;
  c.tOffC = p.getFloat("t_off", c.tOffC);
  c.tOnC = p.getFloat("t_on", c.tOnC);
  c.tMaxC = p.getFloat("t_max", c.tMaxC);
  c.overrunS = p.getULong("overrun", c.overrunS);
  c.restart = static_cast<ctl::Restart>(p.getUChar("restart", static_cast<uint8_t>(c.restart)));
  c.offTimerS = p.getULong("off_tmr", c.offTimerS);
  c.confirmS = p.getULong("confirm", c.confirmS);
  c.faultPolicy = static_cast<ctl::FaultPolicy>(p.getUChar("fault", static_cast<uint8_t>(c.faultPolicy)));
  s.mode = static_cast<ctl::Mode>(p.getUChar("mode", static_cast<uint8_t>(s.mode)));
  s.savedState = static_cast<ctl::State>(p.getUChar("state", static_cast<uint8_t>(s.savedState)));
  s.source = static_cast<TempSource>(p.getUChar("src", static_cast<uint8_t>(s.source)));
  s.irEmissivity = p.getFloat("ir_eps", s.irEmissivity);
  s.tempOffsetC = p.getFloat("offset", s.tempOffsetC);
  s.timeoutLocalS = p.getUShort("to_loc", s.timeoutLocalS);
  s.timeoutRemoteS = p.getUShort("to_rem", s.timeoutRemoteS);
  s.nodeFastS = p.getUShort("n_fast", s.nodeFastS);
  s.nodeSlowS = p.getUShort("n_slow", s.nodeSlowS);
  s.actuator = static_cast<ActuatorType>(p.getUChar("act", static_cast<uint8_t>(s.actuator)));
  s.relayActiveLow = p.getBool("rly_low", s.relayActiveLow);
  s.servoAllowDeg = p.getUChar("sv_allow", s.servoAllowDeg);
  s.servoBlockDeg = p.getUChar("sv_block", s.servoBlockDeg);
  s.servoRestDeg = p.getUChar("sv_rest", s.servoRestDeg);
  s.servoPressDeg = p.getUChar("sv_press", s.servoPressDeg);
  s.servoPressMs = p.getUShort("sv_ms", s.servoPressMs);
  s.thermostatAssumedOn = p.getBool("sv_on", s.thermostatAssumedOn);
  readString(p, "ssid", s.wifiSsid, sizeof(s.wifiSsid));
  readString(p, "pass", s.wifiPass, sizeof(s.wifiPass));
  readString(p, "host", s.hostname, sizeof(s.hostname));
  readString(p, "webpass", s.webPass, sizeof(s.webPass));
  p.end();
  s.sanitize();
}

void save(const AppSettings& s) {
  Preferences p;
  if (!p.begin(kNamespace, false)) return;
  const ctl::Params& c = s.ctl;
  p.putFloat("t_off", c.tOffC);
  p.putFloat("t_on", c.tOnC);
  p.putFloat("t_max", c.tMaxC);
  p.putULong("overrun", c.overrunS);
  p.putUChar("restart", static_cast<uint8_t>(c.restart));
  p.putULong("off_tmr", c.offTimerS);
  p.putULong("confirm", c.confirmS);
  p.putUChar("fault", static_cast<uint8_t>(c.faultPolicy));
  p.putUChar("src", static_cast<uint8_t>(s.source));
  p.putFloat("ir_eps", s.irEmissivity);
  p.putFloat("offset", s.tempOffsetC);
  p.putUShort("to_loc", s.timeoutLocalS);
  p.putUShort("to_rem", s.timeoutRemoteS);
  p.putUShort("n_fast", s.nodeFastS);
  p.putUShort("n_slow", s.nodeSlowS);
  p.putUChar("act", static_cast<uint8_t>(s.actuator));
  p.putBool("rly_low", s.relayActiveLow);
  p.putUChar("sv_allow", s.servoAllowDeg);
  p.putUChar("sv_block", s.servoBlockDeg);
  p.putUChar("sv_rest", s.servoRestDeg);
  p.putUChar("sv_press", s.servoPressDeg);
  p.putUShort("sv_ms", s.servoPressMs);
  p.putString("ssid", s.wifiSsid);
  p.putString("pass", s.wifiPass);
  p.putString("host", s.hostname);
  p.putString("webpass", s.webPass);
  p.end();
  saveRuntime(s);
}

void saveRuntime(const AppSettings& s) {
  Preferences p;
  if (!p.begin(kNamespace, false)) return;
  p.putUChar("mode", static_cast<uint8_t>(s.mode));
  p.putUChar("state", static_cast<uint8_t>(s.savedState));
  p.putBool("sv_on", s.thermostatAssumedOn);
  p.end();
}

void factoryReset() {
  Preferences p;
  if (!p.begin(kNamespace, false)) return;
  p.clear();
  p.end();
}

}  // namespace settings
