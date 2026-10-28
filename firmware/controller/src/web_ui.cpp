#include "web_ui.h"

#include <Arduino.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <math.h>
#include <string.h>

#include "app.h"
#include "espnow_link.h"
#include "radiator_packet.h"
#include "sensor_math.h"
#include "web_page.h"

namespace {

WebServer server(80);
bool uploadAuthorized = false;

// ---------------------------------------------------------------- JSON

void appendEscaped(String& out, const char* s) {
  out += '"';
  for (; s != nullptr && *s != '\0'; ++s) {
    const char c = *s;
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) < 0x20) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%cu%04x", '\\', static_cast<unsigned>(c));
      out += buf;
    } else {
      out += c;
    }
  }
  out += '"';
}

class Json {
 public:
  void num(const char* k, float v, int decimals) {
    key(k);
    if (isnan(v) || isinf(v)) s_ += "null";
    else s_ += String(v, decimals);
  }
  void i(const char* k, long v) {
    key(k);
    s_ += String(v);
  }
  void b(const char* k, bool v) {
    key(k);
    s_ += v ? "true" : "false";
  }
  void str(const char* k, const char* v) {
    key(k);
    appendEscaped(s_, v);
  }
  void raw(const char* k, const String& v) {
    key(k);
    s_ += v;
  }
  String object() const { return "{" + s_ + "}"; }

 private:
  void key(const char* k) {
    if (!first_) s_ += ',';
    first_ = false;
    s_ += '"';
    s_ += k;
    s_ += "\":";
  }
  String s_;
  bool first_ = true;
};

void sendJson(const String& body, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json; charset=utf-8", body);
}

void sendError(int code, const char* msg) {
  Json j;
  j.str("error", msg);
  sendJson(j.object(), code);
}

void sendOk() { sendJson("{\"ok\":true}"); }

// ---------------------------------------------------------------- argomenti

bool argPresent(const char* name) { return server.hasArg(name) && server.arg(name).length() > 0; }

void argFloat(const char* name, float& dst) {
  if (argPresent(name)) dst = server.arg(name).toFloat();
}

long argLong(const char* name, long lo, long hi, long def) {
  long v = server.arg(name).toInt();
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return argPresent(name) ? v : def;
}

template <typename T>
void argInt(const char* name, T& dst, long lo, long hi, long mul = 1) {
  if (argPresent(name)) dst = static_cast<T>(argLong(name, lo, hi, 0) * mul);
}

void argBool(const char* name, bool& dst) {
  if (argPresent(name)) dst = server.arg(name) == "1" || server.arg(name) == "on" || server.arg(name) == "true";
}

void argString(const char* name, char* dst, size_t n) {
  if (!server.hasArg(name)) return;
  strncpy(dst, server.arg(name).c_str(), n - 1);
  dst[n - 1] = '\0';
}

bool authorized() {
  if (app.cfg.webPass[0] == '\0') return true;
  if (server.authenticate("admin", app.cfg.webPass)) return true;
  server.requestAuthentication(BASIC_AUTH, "Termosifone");
  return false;
}

// ---------------------------------------------------------------- pagine

void handleRoot() {
  if (!authorized()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

String wifiJson() {
  Json w;
  const bool ok = WiFi.status() == WL_CONNECTED;
  w.b("ok", ok);
  w.str("ssid", app.cfg.wifiSsid);
  w.str("ip", ok ? WiFi.localIP().toString().c_str() : "");
  w.i("rssi", ok ? WiFi.RSSI() : 0);
  w.i("ch", WiFi.channel());
  w.b("ap", app.apActive);
  w.str("apSsid", app.apSsid);
  w.str("apIp", app.apActive ? WiFi.softAPIP().toString().c_str() : "");
  return w.object();
}

String nodesJson(uint64_t now) {
  NodeInfo nodes[espnow_link::kMaxNodes];
  const int n = espnow_link::snapshot(nodes);
  String out = "[";
  for (int k = 0; k < n; ++k) {
    const NodeInfo& nd = nodes[k];
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", nd.mac[0], nd.mac[1], nd.mac[2], nd.mac[3],
             nd.mac[4], nd.mac[5]);
    Json o;
    o.str("mac", mac);
    o.b("tv", nd.tempValid);
    o.num("t", nd.tempC, 2);
    o.i("batt", nd.battMv);
    o.b("low", (nd.flags & RP_FLAG_BATT_LOW) != 0);
    o.i("rssi", nd.rssi);
    o.i("age", static_cast<long>((now > nd.lastSeenMs ? now - nd.lastSeenMs : 0) / 1000ULL));
    o.i("int", nd.intervalS);
    if (k > 0) out += ',';
    out += o.object();
  }
  out += ']';
  return out;
}

void handleStatus() {
  if (!authorized()) return;
  const uint64_t now = nowMs();
  const ctl::Controller& c = app.ctl;
  const TempReading& r = app.lastReading;
  Json j;
  j.str("fw", FW_VERSION);
  j.i("up", static_cast<long>(now / 1000ULL));
  j.b("tv", c.hasTemperature());
  j.num("t", c.hasTemperature() ? c.temperature() : NAN, 2);
  j.num("raw", r.rawC, 2);
  j.num("amb", r.ambientC, 2);
  j.i("src", static_cast<int>(app.cfg.source));
  j.str("srcName", tempSourceName(app.cfg.source));
  j.str("err", r.valid ? "" : r.error);
  j.b("fault", c.sensorFault());
  j.i("mode", static_cast<int>(c.mode()));
  j.str("modeName", ctl::Controller::modeName(c.mode()));
  j.i("state", static_cast<int>(c.state()));
  j.str("stateName", ctl::Controller::stateName(c.state()));
  j.b("allowed", c.heatingAllowed());
  j.i("inState", static_cast<long>(c.msInState(now) / 1000ULL));
  j.i("rem", static_cast<long>(c.remainingMs(now) / 1000ULL));
  j.str("reason", c.reason());
  j.num("tOff", c.params().tOffC, 1);
  j.num("tOn", c.params().tOnC, 1);
  j.num("tMax", c.params().tMaxC, 1);
  j.i("restart", static_cast<int>(c.params().restart));
  j.i("act", static_cast<int>(app.cfg.actuator));
  j.str("actName", actuatorName(app.cfg.actuator));
  j.b("thermoOn", app.act.thermostatAssumedOn());
  j.i("toRem", app.cfg.timeoutRemoteS);
  j.raw("wifi", wifiJson());
  j.raw("nodes", nodesJson(now));
  sendJson(j.object());
}

void handleConfigGet() {
  if (!authorized()) return;
  const AppSettings& s = app.cfg;
  const ctl::Params& p = s.ctl;
  Json j;
  j.num("t_off", p.tOffC, 1);
  j.num("t_on", p.tOnC, 1);
  j.num("t_max", p.tMaxC, 1);
  j.i("overrun_min", static_cast<long>(p.overrunS / 60));
  j.i("restart", static_cast<int>(p.restart));
  j.i("off_min", static_cast<long>(p.offTimerS / 60));
  j.i("confirm_s", static_cast<long>(p.confirmS));
  j.i("fault", static_cast<int>(p.faultPolicy));
  j.i("src", static_cast<int>(s.source));
  j.num("ir_eps", s.irEmissivity, 3);
  j.num("offset", s.tempOffsetC, 1);
  j.i("to_loc", s.timeoutLocalS);
  j.i("to_rem", s.timeoutRemoteS);
  j.i("n_fast", s.nodeFastS);
  j.i("n_slow", s.nodeSlowS);
  j.i("act", static_cast<int>(s.actuator));
  j.b("rly_low", s.relayActiveLow);
  j.i("sv_allow", s.servoAllowDeg);
  j.i("sv_block", s.servoBlockDeg);
  j.i("sv_rest", s.servoRestDeg);
  j.i("sv_press", s.servoPressDeg);
  j.i("sv_ms", s.servoPressMs);
  j.str("ssid", s.wifiSsid);
  j.b("passSet", s.wifiPass[0] != '\0');
  j.str("host", s.hostname);
  j.b("webpassSet", s.webPass[0] != '\0');
  sendJson(j.object());
}

void handleConfigPost() {
  if (!authorized()) return;
  AppSettings s = app.cfg;
  ctl::Params& p = s.ctl;
  argFloat("t_off", p.tOffC);
  argFloat("t_on", p.tOnC);
  argFloat("t_max", p.tMaxC);
  argInt("overrun_min", p.overrunS, 0, 240, 60);
  argInt("off_min", p.offTimerS, 1, 1440, 60);
  argInt("confirm_s", p.confirmS, 0, 600);
  uint8_t restart = static_cast<uint8_t>(p.restart);
  argInt("restart", restart, 0, 4);
  p.restart = static_cast<ctl::Restart>(restart);
  uint8_t fault = static_cast<uint8_t>(p.faultPolicy);
  argInt("fault", fault, 0, 1);
  p.faultPolicy = static_cast<ctl::FaultPolicy>(fault);
  uint8_t src = static_cast<uint8_t>(s.source);
  argInt("src", src, 0, 2);
  s.source = static_cast<TempSource>(src);
  argFloat("ir_eps", s.irEmissivity);
  argFloat("offset", s.tempOffsetC);
  argInt("to_loc", s.timeoutLocalS, 10, 3600);
  argInt("to_rem", s.timeoutRemoteS, 60, 3600);
  argInt("n_fast", s.nodeFastS, 10, 600);
  argInt("n_slow", s.nodeSlowS, 10, 1800);
  uint8_t act = static_cast<uint8_t>(s.actuator);
  argInt("act", act, 0, 3);
  s.actuator = static_cast<ActuatorType>(act);
  argBool("rly_low", s.relayActiveLow);
  argInt("sv_allow", s.servoAllowDeg, 0, 180);
  argInt("sv_block", s.servoBlockDeg, 0, 180);
  argInt("sv_rest", s.servoRestDeg, 0, 180);
  argInt("sv_press", s.servoPressDeg, 0, 180);
  argInt("sv_ms", s.servoPressMs, 100, 3000);
  s.sanitize();
  app.cfg = s;
  settings::save(app.cfg);
  const uint64_t now = nowMs();
  app.applySettings(now);
  app.events.add(now, "impostazioni salvate (spegne a %.1f C, riaccensione: %s)", app.cfg.ctl.tOffC,
                 ctl::Controller::restartName(app.cfg.ctl.restart));
  sendOk();
}

void handleCalibrate() {
  if (app.cfg.source != TempSource::InfraredMlx) {
    sendError(400, "La calibrazione serve solo con il sensore a infrarossi.");
    return;
  }
  if (!argPresent("real")) {
    sendError(400, "Scrivi la temperatura misurata a contatto.");
    return;
  }
  const float real = server.arg("real").toFloat();
  const TempReading& r = app.lastReading;
  if (!r.valid || isnan(r.rawC) || isnan(r.ambientC)) {
    sendError(409, "Nessuna lettura infrarossi valida in questo momento.");
    return;
  }
  const float e = sm::irEmissivityFromReference(r.rawC, r.ambientC, real);
  if (isnan(e)) {
    sendError(400, "Il termosifone deve essere almeno 8 gradi piu' caldo del sensore: riprova a termosifone caldo.");
    return;
  }
  app.cfg.irEmissivity = e;
  app.cfg.tempOffsetC = 0.0f;
  settings::save(app.cfg);
  const uint64_t now = nowMs();
  app.applySettings(now);
  app.events.add(now, "calibrazione IR: lettura %.1f C, reale %.1f C, emissivita' %.3f", r.rawC, real, e);
  Json j;
  j.b("ok", true);
  j.num("eps", e, 3);
  sendJson(j.object());
}

void handleCmd() {
  if (!authorized()) return;
  const String c = server.arg("c");
  const uint64_t now = nowMs();
  if (c == "calibrate") {
    handleCalibrate();
    return;
  }
  if (c == "auto") {
    app.ctl.setMode(ctl::Mode::Auto, now);
  } else if (c == "bypass") {
    app.ctl.setMode(ctl::Mode::Bypass, now);
  } else if (c == "forceoff") {
    app.ctl.setMode(ctl::Mode::ForceOff, now);
  } else if (c == "forceon") {
    const long minutes = argLong("min", 1, 1440, 60);
    app.ctl.setMode(ctl::Mode::ForceOn, now, static_cast<uint32_t>(minutes) * 60UL);
  } else if (c == "resume") {
    if (app.ctl.mode() == ctl::Mode::Auto) app.ctl.resumeHeating(now);
    else app.ctl.setMode(ctl::Mode::Auto, now);
  } else if (c == "thermo_on" || c == "thermo_off") {
    const bool on = c == "thermo_on";
    app.act.setThermostatAssumedOn(on);
    app.cfg.thermostatAssumedOn = on;
    settings::saveRuntime(app.cfg);
    app.events.add(now, "allineato: il termostato ora e' %s", on ? "acceso" : "spento");
  } else {
    sendError(400, "Comando sconosciuto.");
    return;
  }
  app.ctl.update(now);
  sendOk();
}

void handleHistory() {
  if (!authorized()) return;
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json; charset=utf-8", "");
  const int n = app.history.size();
  String chunk;
  chunk.reserve(1100);
  chunk = "{\"step\":";
  chunk += HISTORY_INTERVAL_S;
  chunk += ",\"t\":[";
  for (int k = 0; k < n; ++k) {
    const int16_t v = app.history.at(k).tempX10;
    if (k > 0) chunk += ',';
    if (v == History<HISTORY_SIZE>::kInvalid) chunk += "null";
    else chunk += v;
    if (chunk.length() > 1000) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  chunk += "],\"f\":[";
  for (int k = 0; k < n; ++k) {
    if (k > 0) chunk += ',';
    chunk += app.history.at(k).flags;
    if (chunk.length() > 1000) {
      server.sendContent(chunk);
      chunk = "";
    }
  }
  chunk += "]}";
  server.sendContent(chunk);
  server.sendContent("");
}

void handleEvents() {
  if (!authorized()) return;
  const uint64_t now = nowMs();
  String out = "[";
  for (int k = 0; k < app.events.size(); ++k) {
    const auto& e = app.events.recent(k);
    Json o;
    o.i("ago", static_cast<long>((now > e.ms ? now - e.ms : 0) / 1000ULL));
    o.str("text", e.text);
    if (k > 0) out += ',';
    out += o.object();
  }
  out += ']';
  sendJson(out);
}

void handleWifi() {
  if (!authorized()) return;
  AppSettings s = app.cfg;
  char ssid[sizeof(s.wifiSsid)];
  strncpy(ssid, s.wifiSsid, sizeof(ssid));
  argString("ssid", s.wifiSsid, sizeof(s.wifiSsid));
  const bool ssidChanged = strcmp(ssid, s.wifiSsid) != 0;
  // Password vuota = invariata, salvo cambio di rete (rete aperta).
  if (ssidChanged || argPresent("pass")) argString("pass", s.wifiPass, sizeof(s.wifiPass));
  if (argPresent("host")) argString("host", s.hostname, sizeof(s.hostname));
  if (server.arg("webpass_clear") == "1") s.webPass[0] = '\0';
  else if (argPresent("webpass")) argString("webpass", s.webPass, sizeof(s.webPass));
  s.sanitize();
  app.cfg = s;
  settings::save(app.cfg);
  app.rebootAtMs = nowMs() + 1500;
  sendOk();
}

void handleReboot() {
  if (!authorized()) return;
  app.rebootAtMs = nowMs() + 800;
  sendOk();
}

void handleFactory() {
  if (!authorized()) return;
  settings::factoryReset();
  app.rebootAtMs = nowMs() + 800;
  sendOk();
}

// ---------------------------------------------------------------- aggiornamento firmware

void handleUpdatePage() {
  if (!authorized()) return;
  server.send_P(200, "text/html; charset=utf-8", UPDATE_HTML);
}

void handleUpdateDone() {
  if (!authorized()) return;
  const bool ok = uploadAuthorized && !Update.hasError() && Update.isFinished();
  server.sendHeader("Connection", "close");
  server.send(ok ? 200 : 500, "text/plain; charset=utf-8",
              ok ? "Aggiornamento riuscito: riavvio in corso." : "Aggiornamento fallito: il firmware precedente resta attivo.");
  if (ok) app.rebootAtMs = nowMs() + 1000;
}

void handleUpdateUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    uploadAuthorized = app.cfg.webPass[0] == '\0' || server.authenticate("admin", app.cfg.webPass);
    if (!uploadAuthorized) return;
    Serial.printf("[ota] ricevo %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
  } else if (!uploadAuthorized) {
    return;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    feedLoopWDT();  // l'upload blocca il loop: tiene buono il watchdog
    if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) Serial.printf("[ota] ok, %u byte\n", static_cast<unsigned>(up.totalSize));
    else Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
  }
}

}  // namespace

void webBegin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/cmd", HTTP_POST, handleCmd);
  server.on("/api/history", HTTP_GET, handleHistory);
  server.on("/api/events", HTTP_GET, handleEvents);
  server.on("/api/wifi", HTTP_POST, handleWifi);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/api/factory", HTTP_POST, handleFactory);
  server.onNotFound([]() { server.send(404, "text/plain; charset=utf-8", "Pagina non trovata"); });
  // Aggiornamento firmware dal browser (file .bin), protetto dalla password della pagina.
  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  server.begin();
}

void webLoop() { server.handleClient(); }
