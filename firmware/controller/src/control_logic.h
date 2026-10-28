// Logica di controllo del termosifone: macchina a stati pura, senza dipendenze
// da Arduino, cosi' si puo' testare sul PC (vedi firmware/test).
//
// Uscita: heatingAllowed() == true  -> il termostato comanda la caldaia come sempre
//         heatingAllowed() == false -> il riscaldamento e' bloccato dal dispositivo
#pragma once

#include <stdint.h>

namespace ctl {

enum class Mode : uint8_t {
  Auto = 0,      // ciclo automatico a soglie/timer
  Bypass = 1,    // il dispositivo non interviene: comanda solo il termostato
  ForceOff = 2,  // riscaldamento sempre bloccato
  ForceOn = 3,   // riscaldamento consentito per un tempo limitato, poi torna Auto
};

enum class Restart : uint8_t {
  Temperature = 0,          // riaccende quando T <= soglia bassa
  Timer = 1,                // riaccende dopo offTimerS
  TimerOrTemperature = 2,   // il primo dei due
  TimerAndTemperature = 3,  // entrambi: pausa minima + termosifone freddo
  Never = 4,                // resta spento fino allo sblocco manuale
};

enum class FaultPolicy : uint8_t {
  AllowHeating = 0,  // sensore guasto -> lascia lavorare il termostato
  BlockHeating = 1,  // sensore guasto -> blocca il riscaldamento
};

enum class State : uint8_t {
  Heating = 0,   // riscaldamento consentito
  Overrun = 1,   // soglia raggiunta, consentito ancora per overrunS
  Cooldown = 2,  // bloccato, in attesa della condizione di riaccensione
  Lockout = 3,   // bloccato fino allo sblocco manuale
};

struct Params {
  float tOffC = 50.0f;               // soglia di spegnimento
  float tOnC = 30.0f;                // soglia di riaccensione
  float tMaxC = 75.0f;               // limite di sicurezza: blocca anche in post-soglia e in "acceso forzato"
  uint32_t overrunS = 0;             // riscaldamento extra dopo aver raggiunto tOffC (0 = spegne subito)
  Restart restart = Restart::Temperature;
  uint32_t offTimerS = 90UL * 60UL;  // durata della pausa nelle modalita' a timer
  uint32_t confirmS = 20;            // per quanto una soglia deve restare superata prima di agire
  uint32_t sensorTimeoutS = 120;     // senza letture valide per questo tempo -> guasto sensore
  FaultPolicy faultPolicy = FaultPolicy::AllowHeating;

  // Riporta i valori in intervalli sensati. Restituisce true se ha corretto qualcosa.
  bool sanitize();
};

// Vero quando la condizione e' rimasta vera ininterrottamente per almeno holdMs.
struct Debounce {
  bool active = false;
  uint64_t sinceMs = 0;

  bool update(bool cond, uint64_t nowMs, uint64_t holdMs) {
    if (!cond) {
      active = false;
      return false;
    }
    if (!active) {
      active = true;
      sinceMs = nowMs;
    }
    return nowMs - sinceMs >= holdMs;
  }
  void reset() { active = false; }
};

class Controller {
 public:
  // restored: stato salvato prima di un riavvio (Pausa e Bloccato riprendono,
  // Post-soglia riparte da Heating). I timer ripartono da zero.
  void begin(uint64_t nowMs, State restored = State::Heating);
  void setParams(const Params& p);
  const Params& params() const { return p_; }

  // Nuova lettura dal sensore (valid=false se la lettura e' fallita).
  void onReading(uint64_t nowMs, bool valid, float tempC);
  // Da chiamare spesso: fa avanzare timer e stati e ricalcola l'uscita.
  void update(uint64_t nowMs);

  // forceOnS e' usato solo con Mode::ForceOn.
  void setMode(Mode m, uint64_t nowMs, uint32_t forceOnS = 0);
  // "Riaccendi ora": sblocca e riparte da Heating (solo in Auto).
  void resumeHeating(uint64_t nowMs);

  bool heatingAllowed() const { return allowed_; }
  Mode mode() const { return mode_; }
  State state() const { return state_; }
  bool sensorFault() const { return fault_; }
  bool hasTemperature() const { return haveValid_ && !fault_; }
  float temperature() const { return lastTemp_; }
  uint64_t msInState(uint64_t nowMs) const { return nowMs - stateSinceMs_; }
  // Tempo residuo del timer attivo (post-soglia, pausa a timer, acceso forzato); 0 se nessuno.
  uint64_t remainingMs(uint64_t nowMs) const;
  // Motivo dell'ultimo cambio di stato o modalita' (testo statico, per log e interfaccia).
  const char* reason() const { return reason_; }
  // Incrementato a ogni cambio di stato o modalita'.
  uint32_t eventCount() const { return events_; }

  // Stato da salvare per ripartire dopo un riavvio (vedi begin()).
  State persistentState() const;

  static const char* stateName(State s);
  static const char* modeName(Mode m);
  static const char* restartName(Restart r);

 private:
  bool step(uint64_t nowMs);
  void enter(State s, uint64_t nowMs, const char* why);
  void enterOff(uint64_t nowMs, const char* why);
  bool computeAllowed() const;
  bool usesOffTimer() const;
  bool dataOk() const { return haveValid_ && !fault_; }

  Params p_;
  Mode mode_ = Mode::Auto;
  State state_ = State::Heating;
  uint64_t stateSinceMs_ = 0;
  uint64_t offTimerStartMs_ = 0;
  uint64_t forceOnStartMs_ = 0;
  uint64_t forceOnMs_ = 0;
  uint64_t lastValidMs_ = 0;
  bool haveValid_ = false;
  float lastTemp_ = 0.0f;
  bool fault_ = false;
  bool allowed_ = true;
  Debounce deb_;
  Debounce forceMaxDeb_;
  const char* reason_ = "avvio";
  uint32_t events_ = 0;
};

}  // namespace ctl
