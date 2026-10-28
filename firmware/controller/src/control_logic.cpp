#include "control_logic.h"

#include <math.h>

namespace ctl {

namespace {

const uint32_t kMaxOverrunS = 4UL * 3600UL;
const uint32_t kMinOffTimerS = 60UL;
const uint32_t kMaxOffTimerS = 24UL * 3600UL;
const uint32_t kMaxConfirmS = 600UL;
const uint32_t kMinTimeoutS = 10UL;
const uint32_t kMaxTimeoutS = 3600UL;
const uint32_t kMinForceOnS = 60UL;
const uint32_t kMaxForceOnS = 24UL * 3600UL;

bool fixFloat(float& v, float lo, float hi, float def) {
  float old = v;
  if (isnan(v)) v = def;
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return v != old;
}

bool fixU32(uint32_t& v, uint32_t lo, uint32_t hi) {
  uint32_t old = v;
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return v != old;
}

uint64_t remaining(uint64_t nowMs, uint64_t startMs, uint64_t durMs) {
  uint64_t el = nowMs - startMs;
  return el >= durMs ? 0 : durMs - el;
}

}  // namespace

bool Params::sanitize() {
  bool changed = false;
  changed |= fixFloat(tOffC, 25.0f, 90.0f, 50.0f);
  changed |= fixFloat(tOnC, 10.0f, tOffC - 1.0f, tOffC - 20.0f);
  changed |= fixFloat(tMaxC, tOffC + 1.0f, 95.0f, 75.0f);
  changed |= fixU32(overrunS, 0, kMaxOverrunS);
  changed |= fixU32(offTimerS, kMinOffTimerS, kMaxOffTimerS);
  changed |= fixU32(confirmS, 0, kMaxConfirmS);
  changed |= fixU32(sensorTimeoutS, kMinTimeoutS, kMaxTimeoutS);
  if (static_cast<uint8_t>(restart) > static_cast<uint8_t>(Restart::Never)) {
    restart = Restart::Temperature;
    changed = true;
  }
  if (static_cast<uint8_t>(faultPolicy) > static_cast<uint8_t>(FaultPolicy::BlockHeating)) {
    faultPolicy = FaultPolicy::AllowHeating;
    changed = true;
  }
  return changed;
}

void Controller::begin(uint64_t nowMs, State restored) {
  p_.sanitize();
  mode_ = Mode::Auto;
  haveValid_ = false;
  fault_ = false;
  // Periodo di grazia: il guasto sensore scatta solo dopo sensorTimeoutS senza letture.
  lastValidMs_ = nowMs;
  events_ = 0;
  switch (restored) {
    case State::Lockout: enter(State::Lockout, nowMs, "bloccato prima del riavvio"); break;
    case State::Cooldown: enter(State::Cooldown, nowMs, "in pausa prima del riavvio"); break;
    default: enter(State::Heating, nowMs, "avvio"); break;
  }
  allowed_ = computeAllowed();
}

State Controller::persistentState() const {
  if (mode_ != Mode::Auto || state_ == State::Overrun) return State::Heating;
  return state_;
}

void Controller::setParams(const Params& p) {
  p_ = p;
  p_.sanitize();
}

void Controller::onReading(uint64_t nowMs, bool valid, float tempC) {
  if (!valid || isnan(tempC)) return;
  lastTemp_ = tempC;
  lastValidMs_ = nowMs;
  haveValid_ = true;
}

void Controller::update(uint64_t nowMs) {
  fault_ = nowMs - lastValidMs_ > static_cast<uint64_t>(p_.sensorTimeoutS) * 1000ULL;
  // Piu' transizioni nello stesso istante si risolvono subito, senza far
  // lampeggiare l'uscita (es. fine timer con termosifone ancora caldo).
  for (int i = 0; i < 4 && step(nowMs); ++i) {
  }
  allowed_ = computeAllowed();
}

void Controller::setMode(Mode m, uint64_t nowMs, uint32_t forceOnS) {
  Mode old = mode_;
  if (m == Mode::ForceOn) {
    if (forceOnS < kMinForceOnS) forceOnS = kMinForceOnS;
    if (forceOnS > kMaxForceOnS) forceOnS = kMaxForceOnS;
    forceOnStartMs_ = nowMs;
    forceOnMs_ = static_cast<uint64_t>(forceOnS) * 1000ULL;
    forceMaxDeb_.reset();
  }
  mode_ = m;
  if (m == Mode::Auto && old != Mode::Auto) {
    enter(State::Heating, nowMs, "modalita' automatica");
  } else if (m != old || m == Mode::ForceOn) {
    switch (m) {
      case Mode::Bypass: reason_ = "solo termostato (dispositivo escluso)"; break;
      case Mode::ForceOff: reason_ = "spento forzato"; break;
      case Mode::ForceOn: reason_ = "acceso forzato a tempo"; break;
      default: break;
    }
    ++events_;
  }
  allowed_ = computeAllowed();
}

void Controller::resumeHeating(uint64_t nowMs) {
  if (mode_ != Mode::Auto) return;
  enter(State::Heating, nowMs, "riacceso manualmente");
  allowed_ = computeAllowed();
}

uint64_t Controller::remainingMs(uint64_t nowMs) const {
  if (mode_ == Mode::ForceOn) return remaining(nowMs, forceOnStartMs_, forceOnMs_);
  if (mode_ != Mode::Auto) return 0;
  if (state_ == State::Overrun) {
    return remaining(nowMs, stateSinceMs_, static_cast<uint64_t>(p_.overrunS) * 1000ULL);
  }
  if (state_ == State::Cooldown && usesOffTimer()) {
    return remaining(nowMs, offTimerStartMs_, static_cast<uint64_t>(p_.offTimerS) * 1000ULL);
  }
  return 0;
}

bool Controller::usesOffTimer() const {
  return p_.restart == Restart::Timer || p_.restart == Restart::TimerOrTemperature ||
         p_.restart == Restart::TimerAndTemperature;
}

void Controller::enter(State s, uint64_t nowMs, const char* why) {
  state_ = s;
  stateSinceMs_ = nowMs;
  if (s == State::Cooldown) offTimerStartMs_ = nowMs;
  deb_.reset();
  reason_ = why;
  ++events_;
}

void Controller::enterOff(uint64_t nowMs, const char* why) {
  enter(p_.restart == Restart::Never ? State::Lockout : State::Cooldown, nowMs, why);
}

bool Controller::step(uint64_t nowMs) {
  const uint64_t confirmMs = static_cast<uint64_t>(p_.confirmS) * 1000ULL;
  const bool ok = dataOk();
  const float t = lastTemp_;

  if (mode_ == Mode::ForceOn) {
    if (nowMs - forceOnStartMs_ >= forceOnMs_) {
      mode_ = Mode::Auto;
      if (ok && t >= p_.tOffC) {
        enterOff(nowMs, "fine acceso forzato, termosifone gia' caldo");
      } else {
        enter(State::Heating, nowMs, "fine acceso forzato");
      }
      return true;
    }
    if (forceMaxDeb_.update(ok && t >= p_.tMaxC, nowMs, confirmMs)) {
      mode_ = Mode::Auto;
      enterOff(nowMs, "limite di sicurezza durante acceso forzato");
      return true;
    }
    return false;
  }
  if (mode_ != Mode::Auto) return false;

  switch (state_) {
    case State::Heating:
      if (deb_.update(ok && t >= p_.tOffC, nowMs, confirmMs)) {
        if (p_.overrunS > 0 && t < p_.tMaxC) {
          enter(State::Overrun, nowMs, "soglia raggiunta, timer post-soglia avviato");
        } else {
          enterOff(nowMs, "soglia di spegnimento raggiunta");
        }
        return true;
      }
      return false;

    case State::Overrun:
      if (nowMs - stateSinceMs_ >= static_cast<uint64_t>(p_.overrunS) * 1000ULL) {
        enterOff(nowMs, "fine timer post-soglia");
        return true;
      }
      if (deb_.update(ok && t >= p_.tMaxC, nowMs, confirmMs)) {
        enterOff(nowMs, "limite di sicurezza raggiunto");
        return true;
      }
      return false;

    case State::Cooldown: {
      if (p_.restart == Restart::Never) {
        enter(State::Lockout, nowMs, "riaccensione disattivata");
        return true;
      }
      const bool cold = deb_.update(ok && t <= p_.tOnC, nowMs, confirmMs);
      const bool timerDone =
          nowMs - offTimerStartMs_ >= static_cast<uint64_t>(p_.offTimerS) * 1000ULL;
      bool go = false;
      switch (p_.restart) {
        case Restart::Temperature: go = cold; break;
        case Restart::Timer: go = timerDone; break;
        case Restart::TimerOrTemperature: go = cold || timerDone; break;
        case Restart::TimerAndTemperature: go = cold && timerDone; break;
        case Restart::Never: break;
      }
      if (!go) return false;
      if (!cold && ok && t >= p_.tOffC) {
        // Timer scaduto ma il termosifone e' ancora sopra la soglia: riaccendere
        // lo farebbe rispegnere subito. Si riarma il timer.
        offTimerStartMs_ = nowMs;
        reason_ = "timer scaduto ma termosifone ancora caldo: pausa prolungata";
        ++events_;
        return false;
      }
      enter(State::Heating, nowMs, cold ? "termosifone freddo, riaccensione" : "fine pausa a timer");
      return true;
    }

    case State::Lockout:
      if (p_.restart != Restart::Never) {
        enter(State::Cooldown, nowMs, "riaccensione riattivata");
        return true;
      }
      return false;
  }
  return false;
}

bool Controller::computeAllowed() const {
  switch (mode_) {
    case Mode::Bypass: return true;
    case Mode::ForceOff: return false;
    case Mode::ForceOn: return true;
    case Mode::Auto: break;
  }
  if (state_ == State::Lockout) return false;
  if (fault_) return p_.faultPolicy == FaultPolicy::AllowHeating;
  return state_ == State::Heating || state_ == State::Overrun;
}

const char* Controller::stateName(State s) {
  switch (s) {
    case State::Heating: return "Riscaldamento consentito";
    case State::Overrun: return "Post-soglia (timer)";
    case State::Cooldown: return "Pausa (riscaldamento bloccato)";
    case State::Lockout: return "Bloccato fino a sblocco";
  }
  return "?";
}

const char* Controller::modeName(Mode m) {
  switch (m) {
    case Mode::Auto: return "Automatico";
    case Mode::Bypass: return "Solo termostato";
    case Mode::ForceOff: return "Spento forzato";
    case Mode::ForceOn: return "Acceso forzato";
  }
  return "?";
}

const char* Controller::restartName(Restart r) {
  switch (r) {
    case Restart::Temperature: return "Temperatura";
    case Restart::Timer: return "Timer";
    case Restart::TimerOrTemperature: return "Timer o temperatura";
    case Restart::TimerAndTemperature: return "Timer e temperatura";
    case Restart::Never: return "Mai (sblocco manuale)";
  }
  return "?";
}

}  // namespace ctl
