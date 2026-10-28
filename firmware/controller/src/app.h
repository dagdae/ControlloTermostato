// Stato dell'applicazione condiviso tra il loop principale e la pagina web.
#pragma once

#include <stdint.h>

#include "actuator.h"
#include "config.h"
#include "control_logic.h"
#include "history.h"
#include "settings.h"
#include "temp_input.h"

struct App {
  AppSettings cfg;
  ctl::Controller ctl;
  TempInput input;
  Actuator act;
  TempReading lastReading;
  History<HISTORY_SIZE> history;
  EventLog<EVENT_LOG_SIZE> events;
  bool apActive = false;
  char apSsid[33] = "";
  uint64_t rebootAtMs = 0;  // 0 = nessun riavvio programmato

  // Applica cfg a controllo, sensori e attuatore (dopo una modifica dalla pagina web).
  void applySettings(uint64_t nowMs);
};

extern App app;

// Millisecondi dall'accensione (64 bit: nessun ritorno a zero).
uint64_t nowMs();
