// Centralina: legge la temperatura del termosifone e consente o blocca il
// riscaldamento comandato dal termostato (rele' in serie o servo).
#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>

#include "app.h"
#include "espnow_link.h"
#include "radiator_packet.h"
#include "web_ui.h"

App app;

uint64_t nowMs() { return static_cast<uint64_t>(esp_timer_get_time() / 1000LL); }

namespace {

ctl::Params paramsFor(const AppSettings& s) {
  ctl::Params p = s.ctl;
  p.sensorTimeoutS = s.source == TempSource::RemoteNode ? s.timeoutRemoteS : s.timeoutLocalS;
  return p;
}

// ---------------------------------------------------------------- Wi-Fi

uint64_t staStartMs = 0;
uint64_t apStartMs = 0;
uint64_t nextStaRetryMs = 0;   // con la rete di configurazione attiva: prossimo tentativo verso la rete di casa
uint64_t staAttemptEndMs = 0;  // fine del tentativo in corso
bool wasConnected = false;
bool mdnsStarted = false;
const uint64_t kStaRetryMs = 5ULL * 60000ULL;
const uint64_t kStaAttemptMs = 30000ULL;

// Mentre cerca la rete di casa la radio salta tra i canali e la rete di
// configurazione diventa instabile: con l'AP attivo si riprova solo ogni 5 minuti.
void pauseStation(uint64_t now) {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
  staAttemptEndMs = 0;
  nextStaRetryMs = now + kStaRetryMs;
}

void startSetupAp(uint64_t now, const char* why) {
  if (app.apActive) return;
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(app.apSsid, sizeof(app.apSsid), "Termosifone-%02X%02X", mac[4], mac[5]);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(app.apSsid, AP_PASSWORD, AP_CHANNEL);
  app.apActive = true;
  apStartMs = now;
  if (app.cfg.wifiSsid[0] != '\0' && WiFi.status() != WL_CONNECTED) pauseStation(now);
  espnow_link::ensureBroadcastPeer();
  app.events.add(now, "rete di configurazione %s attiva (%s)", app.apSsid, why);
  Serial.printf("[wifi] rete di configurazione %s, password %s, http://192.168.4.1\n", app.apSsid, AP_PASSWORD);
}

void wifiStart(uint64_t now) {
  WiFi.persistent(false);
  WiFi.setHostname(app.cfg.hostname);
  const bool haveSsid = app.cfg.wifiSsid[0] != '\0';
  WiFi.mode(haveSsid ? WIFI_STA : WIFI_AP_STA);
  WiFi.setSleep(false);  // radio sempre accesa: serve a ricevere i nodi ESP-NOW
  if (haveSsid) {
    WiFi.setAutoReconnect(true);
    WiFi.begin(app.cfg.wifiSsid, app.cfg.wifiPass);
    staStartMs = now;
  }
  if (!espnow_link::begin(RP_DEFAULT_NET_ID)) app.events.add(now, "ERRORE: ESP-NOW non avviato");
  if (!haveSsid) startSetupAp(now, "nessuna rete configurata");
}

void wifiLoop(uint64_t now) {
  const bool haveSsid = app.cfg.wifiSsid[0] != '\0';
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wasConnected) {
    app.events.add(now, "Wi-Fi connesso a %s, IP %s, canale %d", app.cfg.wifiSsid,
                   WiFi.localIP().toString().c_str(), static_cast<int>(WiFi.channel()));
    Serial.printf("[wifi] connesso: http://%s/  oppure http://%s.local/\n", WiFi.localIP().toString().c_str(),
                  app.cfg.hostname);
  } else if (!connected && wasConnected) {
    app.events.add(now, "Wi-Fi perso, riprovo");
    staStartMs = now;
  }
  wasConnected = connected;

  // Rete di casa irraggiungibile per 1 minuto: si apre anche la rete di configurazione.
  if (haveSsid && !connected && !app.apActive && now - staStartMs > 60000ULL) {
    startSetupAp(now, "rete di casa non raggiungibile");
  }
  if (haveSsid && !connected && app.apActive) {
    if (staAttemptEndMs != 0 && now >= staAttemptEndMs) pauseStation(now);
    if (staAttemptEndMs == 0 && nextStaRetryMs != 0 && now >= nextStaRetryMs) {
      nextStaRetryMs = now + kStaRetryMs;
      if (WiFi.softAPgetStationNum() == 0) {  // non disturbare chi sta configurando
        WiFi.setAutoReconnect(true);
        WiFi.begin(app.cfg.wifiSsid, app.cfg.wifiPass);
        staAttemptEndMs = now + kStaAttemptMs;
      }
    }
  }
  if (connected) {
    staAttemptEndMs = 0;
    nextStaRetryMs = 0;
  }
  // Rete di casa tornata: si chiude la rete di configurazione se nessuno la usa.
  if (haveSsid && connected && app.apActive && now - apStartMs > 15ULL * 60000ULL &&
      WiFi.softAPgetStationNum() == 0) {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    app.apActive = false;
    app.apSsid[0] = '\0';
    espnow_link::ensureBroadcastPeer();
    app.events.add(now, "rete di configurazione chiusa");
  }
  if (!mdnsStarted && (connected || app.apActive)) {
    if (MDNS.begin(app.cfg.hostname)) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
    }
  }
}

// ---------------------------------------------------------------- LED e tasto

void ledWrite(bool on) {
#if PIN_LED >= 0
  digitalWrite(PIN_LED, on == (LED_ACTIVE_HIGH != 0) ? HIGH : LOW);
#else
  (void)on;
#endif
}

void ledUpdate(uint64_t now) {
  bool on;
  if (app.ctl.sensorFault()) on = (now % 250) < 125;             // lampeggio veloce: sensore guasto
  else if (app.ctl.heatingAllowed()) on = true;                    // acceso fisso: riscaldamento consentito
  else on = (now % 2000) < 80;                                     // lampo ogni 2 s: riscaldamento bloccato
  if (app.apActive && (now % 4000) >= 3700) on = !on;              // rete di configurazione attiva
  ledWrite(on);
}

struct Button {
  bool down = false;
  bool longDone = false;
  uint64_t since = 0;
} button;

void toggleBypass(uint64_t now) {
  const bool bypass = app.ctl.mode() == ctl::Mode::Bypass;
  app.ctl.setMode(bypass ? ctl::Mode::Auto : ctl::Mode::Bypass, now);
}

void buttonUpdate(uint64_t now) {
  const bool pressed = digitalRead(PIN_BUTTON) == LOW;
  if (pressed && !button.down) {
    button.since = now;
    button.longDone = false;
  }
  if (pressed && !button.longDone && now - button.since >= 10000ULL) {
    button.longDone = true;
    startSetupAp(now, "tasto premuto 10 s");
  }
  if (!pressed && button.down && !button.longDone) {
    const uint64_t held = now - button.since;
    if (held >= 3000ULL) {
      toggleBypass(now);  // 3 s: alterna Automatico / Solo termostato
    } else if (held >= 50ULL) {
      // pressione breve: riaccendi ora (o torna in automatico)
      if (app.ctl.mode() == ctl::Mode::Auto) app.ctl.resumeHeating(now);
      else app.ctl.setMode(ctl::Mode::Auto, now);
    }
  }
  button.down = pressed;
}

// ---------------------------------------------------------------- stato

uint16_t suggestedNodeInterval() {
  const uint16_t fast = app.cfg.nodeFastS;
  const uint16_t slow = app.cfg.nodeSlowS;
  const ctl::Controller& c = app.ctl;
  if (!c.hasTemperature()) return fast;
  const ctl::Params& p = c.params();
  const float t = c.temperature();
  switch (c.mode()) {
    case ctl::Mode::ForceOn: return fast;
    case ctl::Mode::Bypass:
    case ctl::Mode::ForceOff: return slow;
    case ctl::Mode::Auto: break;
  }
  switch (c.state()) {
    case ctl::State::Heating: return t >= p.tOffC - 10.0f ? fast : slow;
    case ctl::State::Overrun: return fast;
    case ctl::State::Cooldown:
      return (p.restart != ctl::Restart::Timer && t <= p.tOnC + 5.0f) ? fast : slow;
    case ctl::State::Lockout: return slow;
  }
  return slow;
}

uint32_t lastEvents = 0;
bool lastAllowed = true;
uint64_t nextHistoryMs = 0;

void trackChanges(uint64_t now) {
  const ctl::Controller& c = app.ctl;
  if (c.eventCount() != lastEvents) {
    lastEvents = c.eventCount();
    if (c.mode() == ctl::Mode::Auto) app.events.add(now, "%s: %s", ctl::Controller::stateName(c.state()), c.reason());
    else app.events.add(now, "%s: %s", ctl::Controller::modeName(c.mode()), c.reason());
    Serial.printf("[ctl] %s / %s - %s\n", ctl::Controller::modeName(c.mode()), ctl::Controller::stateName(c.state()),
                  c.reason());
  }
  if (c.heatingAllowed() != lastAllowed) {
    lastAllowed = c.heatingAllowed();
    app.events.add(now, "uscita: riscaldamento %s", lastAllowed ? "CONSENTITO" : "BLOCCATO");
  }
  // Salva in memoria solo cio' che serve a ripartire correttamente dopo un riavvio
  // (poche scritture al giorno: nessun problema di usura della flash).
  const ctl::Mode persistMode = c.mode() == ctl::Mode::ForceOn ? ctl::Mode::Auto : c.mode();
  const ctl::State persistState = c.persistentState();
  const bool thermoChanged = app.act.takeThermostatChanged();
  if (persistMode != app.cfg.mode || persistState != app.cfg.savedState || thermoChanged) {
    app.cfg.mode = persistMode;
    app.cfg.savedState = persistState;
    app.cfg.thermostatAssumedOn = app.act.thermostatAssumedOn();
    settings::saveRuntime(app.cfg);
  }
  if (now >= nextHistoryMs) {
    nextHistoryMs = now + HISTORY_INTERVAL_S * 1000ULL;
    app.history.add(c.hasTemperature(), c.temperature(), c.heatingAllowed(), static_cast<uint8_t>(c.state()),
                    static_cast<uint8_t>(c.mode()));
  }
}

}  // namespace

void App::applySettings(uint64_t now) {
  ctl.setParams(paramsFor(cfg));
  if (input.reconfigure(cfg)) {
    ctl.resetSensor(now);
    lastReading = TempReading();
  }
  act.reconfigure(cfg);
  ctl.update(now);
}

void setup() {
  Serial.begin(115200);
  const uint64_t now = nowMs();
  settings::load(app.cfg);
  app.act.begin(app.cfg, now);  // per prima cosa: uscita nello stato sicuro
#if PIN_LED >= 0
  pinMode(PIN_LED, OUTPUT);
#endif
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  app.ctl.setParams(paramsFor(app.cfg));
  app.ctl.begin(now, app.cfg.mode == ctl::Mode::Auto ? app.cfg.savedState : ctl::State::Heating);
  if (app.cfg.mode != ctl::Mode::Auto) app.ctl.setMode(app.cfg.mode, now);
  lastEvents = app.ctl.eventCount();
  lastAllowed = app.ctl.heatingAllowed();
  app.input.begin(app.cfg);
  app.events.add(now, "avvio firmware %s, sorgente: %s, attuatore: %s", FW_VERSION, tempSourceName(app.cfg.source),
                 actuatorName(app.cfg.actuator));
  Serial.printf("\n[avvio] Controllo termosifone %s\n", FW_VERSION);

  wifiStart(now);
  webBegin();
  // Watchdog: se il loop resta bloccato per 10 s l'ESP32 si riavvia (e il rele' torna a
  // riposo). Configurato qui per avere il riavvio garantito qualunque sia l'impostazione del core.
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t wdt = {};
  wdt.timeout_ms = 10000;
  wdt.idle_core_mask = 1;  // come il core Arduino: controlla l'idle del core 0
  wdt.trigger_panic = true;
  esp_task_wdt_reconfigure(&wdt);
#endif
  enableLoopWDT();
}

void loop() {
  const uint64_t now = nowMs();
  buttonUpdate(now);

  TempReading r;
  if (app.input.poll(now, &r)) {
    app.lastReading = r;
    app.ctl.onReading(now, r.valid, r.tempC);
  }
  app.ctl.update(now);
  app.act.apply(app.ctl.heatingAllowed(), now);
  espnow_link::setAckInfo(suggestedNodeInterval(), static_cast<uint8_t>(app.ctl.state()));
  trackChanges(now);

  wifiLoop(now);
  webLoop();
  ledUpdate(now);

  if (app.rebootAtMs != 0 && now >= app.rebootAtMs) {
    Serial.println("[avvio] riavvio richiesto");
    delay(100);
    ESP.restart();
  }
  delay(2);
}
