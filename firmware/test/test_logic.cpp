// Test sul PC della logica di controllo e dei calcoli dei sensori.
// Esecuzione:  make -C firmware/test
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../controller/src/control_logic.h"
#include "../controller/src/ds18b20.h"
#include "../controller/src/radiator_packet.h"
#include "../controller/src/sensor_math.h"
#include "../sensor_node/src/ntc.h"

using ctl::Controller;
using ctl::FaultPolicy;
using ctl::Mode;
using ctl::Params;
using ctl::Restart;
using ctl::State;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    ++g_checks;                                                         \
    if (!(cond)) {                                                      \
      ++g_failures;                                                     \
      printf("  FALLITO %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                   \
  } while (0)

#define CHECK_NEAR(a, b, eps)                                                          \
  do {                                                                                 \
    ++g_checks;                                                                        \
    const double va_ = (a), vb_ = (b);                                                 \
    if (!(fabs(va_ - vb_) <= (eps))) {                                                 \
      ++g_failures;                                                                    \
      printf("  FALLITO %s:%d: %s = %f, atteso %f\n", __FILE__, __LINE__, #a, va_, vb_); \
    }                                                                                  \
  } while (0)

// Simulatore: avanza il tempo a passi di 1 s con una lettura per passo.
struct Sim {
  Controller c;
  uint64_t now = 5000000;  // istante iniziale arbitrario
  bool everAllowed = false;
  bool everBlocked = false;

  explicit Sim(const Params& p, State restored = State::Heating) {
    c.setParams(p);
    c.begin(now, restored);
    c.update(now);
  }
  void run(double seconds, float temp, bool valid = true) {
    everAllowed = false;
    everBlocked = false;
    const uint64_t end = now + static_cast<uint64_t>(seconds * 1000.0);
    while (now < end) {
      now += 1000;
      c.onReading(now, valid, temp);
      c.update(now);
      if (c.heatingAllowed()) everAllowed = true;
      else everBlocked = true;
    }
  }
  // Porta il termosifone sopra la soglia finche' il controllo non interviene.
  void reachOff(float temp = 51.0f) { run(c.params().confirmS + 1, temp); }
  bool allowed() const { return c.heatingAllowed(); }
  State state() const { return c.state(); }
};

static void testBasicTemperatureCycle() {
  Params p;
  Sim s(p);
  s.run(60, 25);
  CHECK(s.allowed());
  CHECK(s.state() == State::Heating);
  s.run(10, 50.5f);
  CHECK(s.allowed());  // soglia superata da soli 10 s su 20 di conferma
  s.run(15, 50.5f);
  CHECK(!s.allowed());
  CHECK(s.state() == State::Cooldown);
  s.run(600, 35);
  CHECK(!s.allowed());
  s.run(10, 29);
  CHECK(!s.allowed());
  s.run(15, 29);
  CHECK(s.allowed());
  CHECK(s.state() == State::Heating);
}

static void testSpikeRejected() {
  Params p;
  Sim s(p);
  s.run(60, 25);
  s.run(10, 60);
  s.run(5, 25);
  s.run(15, 60);
  CHECK(s.allowed());
  CHECK(s.state() == State::Heating);
}

static void testOverrun() {
  Params p;
  p.overrunS = 15 * 60;
  Sim s(p);
  s.run(60, 25);
  s.run(25, 51);
  CHECK(s.state() == State::Overrun);
  CHECK(s.allowed());
  CHECK_NEAR(s.c.remainingMs(s.now) / 1000.0, 15 * 60 - 5, 1.0);
  s.run(14 * 60, 52);
  CHECK(s.allowed());
  s.run(60, 52);
  CHECK(!s.allowed());
  CHECK(s.state() == State::Cooldown);
}

static void testOverrunSafetyLimit() {
  Params p;
  p.overrunS = 15 * 60;
  p.tMaxC = 75;
  Sim s(p);
  s.run(25, 51);
  CHECK(s.state() == State::Overrun);
  s.run(10, 80);
  CHECK(s.allowed());
  s.run(15, 80);
  CHECK(!s.allowed());
  CHECK(s.state() == State::Cooldown);
  CHECK(strstr(s.c.reason(), "sicurezza") != nullptr);
}

static void testAboveMaxSkipsOverrun() {
  Params p;
  p.overrunS = 15 * 60;
  Sim s(p);
  s.run(60, 25);
  s.run(25, 80);
  CHECK(s.state() == State::Cooldown);
  CHECK(!s.allowed());
}

static void testTimerRestart() {
  Params p;
  p.restart = Restart::Timer;
  p.offTimerS = 90 * 60;
  Sim s(p);
  s.run(25, 51);
  CHECK(s.state() == State::Cooldown);
  CHECK_NEAR(s.c.remainingMs(s.now) / 1000.0, 90 * 60 - 5, 1.0);
  s.run(89 * 60, 45);
  CHECK(!s.allowed());
  s.run(2 * 60, 45);
  CHECK(s.allowed());
  CHECK(s.state() == State::Heating);
}

static void testTimerRearmsWhileStillHot() {
  Params p;
  p.restart = Restart::Timer;
  p.offTimerS = 90 * 60;
  Sim s(p);
  s.run(25, 51);
  s.run(89 * 60, 55);
  s.run(2 * 60, 55);
  CHECK(!s.everAllowed);  // nessun impulso di accensione a fine timer
  CHECK(s.state() == State::Cooldown);
  CHECK(s.c.remainingMs(s.now) > 85ULL * 60ULL * 1000ULL);
  s.run(60, 45);
  CHECK(!s.allowed());
  s.run(90 * 60, 45);
  CHECK(s.allowed());
}

static void testTimerOrTemperature() {
  Params p;
  p.restart = Restart::TimerOrTemperature;
  p.offTimerS = 90 * 60;
  {
    Sim s(p);
    s.run(25, 51);
    s.run(30 * 60, 29);
    CHECK(s.allowed());  // freddo prima del timer
  }
  {
    Sim s(p);
    s.run(25, 51);
    s.run(89 * 60, 40);
    CHECK(!s.allowed());
    s.run(2 * 60, 40);
    CHECK(s.allowed());  // timer prima del freddo
  }
}

static void testTimerAndTemperature() {
  Params p;
  p.restart = Restart::TimerAndTemperature;
  p.offTimerS = 90 * 60;
  {
    Sim s(p);
    s.run(25, 51);
    s.run(30 * 60, 29);
    CHECK(!s.allowed());  // freddo ma pausa minima non finita
    s.run(61 * 60, 29);
    CHECK(s.allowed());
  }
  {
    Sim s(p);
    s.run(25, 51);
    s.run(91 * 60, 40);
    CHECK(!s.allowed());  // timer finito ma non ancora freddo
    s.run(25, 29);
    CHECK(s.allowed());
  }
}

static void testNeverRestart() {
  Params p;
  p.restart = Restart::Never;
  Sim s(p);
  s.run(25, 51);
  CHECK(s.state() == State::Lockout);
  s.run(5 * 3600, 20);
  CHECK(!s.allowed());
  s.c.resumeHeating(s.now);
  CHECK(s.allowed());
  s.run(60, 20);
  CHECK(s.allowed());
  CHECK(s.state() == State::Heating);
}

static void testLockoutReleasedWhenRestartChanges() {
  Params p;
  p.restart = Restart::Never;
  Sim s(p);
  s.run(25, 51);
  CHECK(s.state() == State::Lockout);
  p.restart = Restart::Temperature;
  s.c.setParams(p);
  s.run(25, 20);
  CHECK(s.allowed());
  CHECK(s.state() == State::Heating);
}

static void testSensorFaultAllowsHeating() {
  Params p;
  p.sensorTimeoutS = 120;
  Sim s(p);
  s.run(25, 51);
  CHECK(!s.allowed());
  s.run(100, 0, false);
  CHECK(!s.allowed());
  CHECK(!s.c.sensorFault());
  s.run(30, 0, false);
  CHECK(s.c.sensorFault());
  CHECK(s.allowed());  // guasto: comanda il termostato
  s.run(5, 45);
  CHECK(!s.c.sensorFault());
  CHECK(!s.allowed());
  CHECK(s.state() == State::Cooldown);
}

static void testSensorFaultBlocksHeating() {
  Params p;
  p.sensorTimeoutS = 120;
  p.faultPolicy = FaultPolicy::BlockHeating;
  Sim s(p);
  s.run(30, 25);
  CHECK(s.allowed());
  s.run(200, 0, false);
  CHECK(!s.allowed());
  s.run(5, 25);
  CHECK(s.allowed());
}

static void testLockoutStaysBlockedOnFault() {
  Params p;
  p.restart = Restart::Never;
  p.sensorTimeoutS = 60;
  Sim s(p);
  s.run(25, 51);
  s.run(300, 0, false);
  CHECK(s.c.sensorFault());
  CHECK(!s.allowed());
}

static void testResetSensorGivesGrace() {
  Params p;
  p.sensorTimeoutS = 120;
  Sim s(p);
  s.run(25, 51);
  CHECK(!s.allowed());
  s.run(110, 0, false);  // la vecchia sorgente tace da quasi 2 minuti
  s.c.resetSensor(s.now);
  s.run(60, 0, false);  // la nuova sorgente non ha ancora letto nulla
  CHECK(!s.c.sensorFault());
  CHECK(!s.allowed());  // resta in pausa, nessuno scatto del rele'
  CHECK(!s.c.hasTemperature());
  s.run(25, 29);
  CHECK(s.allowed());
}

static void testBootGracePeriod() {
  Params p;
  p.sensorTimeoutS = 120;
  Sim s(p);
  s.run(60, 0, false);
  CHECK(!s.c.sensorFault());
  CHECK(s.allowed());
  s.run(70, 0, false);
  CHECK(s.c.sensorFault());
  CHECK(s.allowed());
}

static void testManualModes() {
  Params p;
  {
    Sim s(p);
    s.c.setMode(Mode::Bypass, s.now);
    s.run(120, 70);
    CHECK(s.allowed());
    s.c.setMode(Mode::ForceOff, s.now);
    s.run(60, 20);
    CHECK(!s.allowed());
  }
  {
    Sim s(p);
    s.run(25, 51);
    CHECK(!s.allowed());
    s.c.setMode(Mode::ForceOn, s.now, 30 * 60);
    s.c.update(s.now);
    CHECK(s.allowed());
    s.run(29 * 60, 55);
    CHECK(s.allowed());
    s.run(2 * 60, 55);
    CHECK(s.c.mode() == Mode::Auto);
    CHECK(s.state() == State::Cooldown);  // finito e caldo: direttamente in pausa
    CHECK(!s.allowed());
  }
  {
    Sim s(p);
    s.c.setMode(Mode::ForceOn, s.now, 3600);
    s.run(25, 80);
    CHECK(s.c.mode() == Mode::Auto);
    CHECK(s.state() == State::Cooldown);
    CHECK(!s.allowed());
  }
}

static void testBackToAutoRestartsCycle() {
  Params p;
  p.restart = Restart::Never;
  Sim s(p);
  s.run(25, 51);
  CHECK(s.state() == State::Lockout);
  s.c.setMode(Mode::Bypass, s.now);
  s.run(60, 25);
  CHECK(s.allowed());
  s.c.setMode(Mode::Auto, s.now);
  s.run(5, 25);
  CHECK(s.state() == State::Heating);
  CHECK(s.allowed());
}

static void testBeginWithLockout() {
  Params p;
  p.restart = Restart::Never;
  Sim s(p, State::Lockout);
  CHECK(s.state() == State::Lockout);
  CHECK(!s.allowed());
}

static void testResumeCooldownAfterReboot() {
  Params p;  // riaccensione a temperatura
  Sim s(p, State::Cooldown);
  CHECK(!s.allowed());
  s.run(600, 40);
  CHECK(!s.allowed());  // ancora sopra la soglia bassa: la pausa continua
  s.run(25, 29);
  CHECK(s.allowed());
  // Con "mai" una pausa salvata diventa blocco; con altre modalita' un blocco salvato diventa pausa.
  p.restart = Restart::Never;
  Sim n(p, State::Cooldown);
  CHECK(n.state() == State::Lockout);
}

static void testPersistentState() {
  Params p;
  p.overrunS = 600;
  Sim s(p);
  CHECK(s.c.persistentState() == State::Heating);
  s.run(25, 51);
  CHECK(s.state() == State::Overrun);
  CHECK(s.c.persistentState() == State::Heating);  // il post-soglia riparte da capo
  s.run(600, 51);
  CHECK(s.c.persistentState() == State::Cooldown);
  s.c.setMode(Mode::Bypass, s.now);
  CHECK(s.c.persistentState() == State::Heating);
}

static void testZeroConfirmActsImmediately() {
  Params p;
  p.confirmS = 0;
  Sim s(p);
  s.run(1, 50);
  CHECK(s.state() == State::Cooldown);
}

static void testSanitize() {
  Params p;
  p.tOffC = 40;
  p.tOnC = 45;
  p.tMaxC = 30;
  p.offTimerS = 0;
  p.restart = static_cast<Restart>(9);
  CHECK(p.sanitize());
  CHECK_NEAR(p.tOnC, 39, 1e-4);
  CHECK_NEAR(p.tMaxC, 41, 1e-4);
  CHECK(p.offTimerS == 60);
  CHECK(p.restart == Restart::Temperature);
  Params q;
  q.tOffC = NAN;
  q.sanitize();
  CHECK_NEAR(q.tOffC, 50, 1e-4);
  Params ok;
  CHECK(!ok.sanitize());
}

static void testCrcAndPackets() {
  const uint8_t rom[] = {0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00};
  CHECK(crc8Maxim(rom, sizeof(rom)) == 0xA2);  // esempio Maxim AN27
  const uint8_t mlx[] = {0xB4, 0x07, 0xB5, 0xD2, 0x3A};
  CHECK(sm::crc8Smbus(mlx, sizeof(mlx)) == 0x30);  // esempio datasheet MLX90614
  const char* s = "123456789";
  CHECK(rpCrc16(reinterpret_cast<const uint8_t*>(s), 9) == 0x29B1);

  RpData d;
  memset(&d, 0, sizeof(d));
  d.magic = RP_MAGIC;
  d.version = RP_VERSION;
  d.type = RP_TYPE_DATA;
  d.netId = RP_DEFAULT_NET_ID;
  d.seq = 42;
  d.tempCx100 = 4875;
  rpSeal(d);
  RpData out;
  CHECK(rpDecode(reinterpret_cast<const uint8_t*>(&d), sizeof(d), RP_DEFAULT_NET_ID, RP_TYPE_DATA, &out));
  CHECK(out.tempCx100 == 4875);
  CHECK(!rpDecode(reinterpret_cast<const uint8_t*>(&d), sizeof(d), 0x1234, RP_TYPE_DATA, &out));
  CHECK(!rpDecode(reinterpret_cast<const uint8_t*>(&d), sizeof(d), RP_DEFAULT_NET_ID, RP_TYPE_ACK, &out));
  uint8_t bad[sizeof(d)];
  memcpy(bad, &d, sizeof(d));
  bad[12] ^= 0x01;
  CHECK(!rpDecode(bad, sizeof(bad), RP_DEFAULT_NET_ID, RP_TYPE_DATA, &out));
}

static void makeScratchpad(uint8_t sp[9], uint16_t raw, uint8_t cfg) {
  sp[0] = raw & 0xFF;
  sp[1] = raw >> 8;
  sp[2] = 0x4B;
  sp[3] = 0x46;
  sp[4] = cfg;
  sp[5] = 0xFF;
  sp[6] = 0x0C;
  sp[7] = 0x10;
  sp[8] = crc8Maxim(sp, 8);
}

static void testDs18b20Parse() {
  uint8_t sp[9];
  float t = 0;
  makeScratchpad(sp, 0x0191, 0x7F);
  CHECK(ds18b20ParseScratchpad(sp, &t));
  CHECK_NEAR(t, 25.0625, 1e-6);
  makeScratchpad(sp, 0xFF5E, 0x7F);
  CHECK(ds18b20ParseScratchpad(sp, &t));
  CHECK_NEAR(t, -10.125, 1e-6);
  makeScratchpad(sp, 0x0193, 0x3F);  // 10 bit: i due bit bassi non sono definiti
  CHECK(ds18b20ParseScratchpad(sp, &t));
  CHECK_NEAR(t, 25.0, 1e-6);
  makeScratchpad(sp, 0x0550, 0x7F);
  CHECK(!ds18b20ParseScratchpad(sp, &t));  // 85 C di accensione
  makeScratchpad(sp, 0x0320, 0x7F);
  sp[8] ^= 0x55;
  CHECK(!ds18b20ParseScratchpad(sp, &t));  // CRC errato
  uint8_t zeros[9] = {0};
  CHECK(!ds18b20ParseScratchpad(zeros, &t));
  uint8_t ones[9];
  memset(ones, 0xFF, sizeof(ones));
  CHECK(!ds18b20ParseScratchpad(ones, &t));
  CHECK(ds18b20ConversionMs(10) == 188);
}

static void testMlxAndEmissivity() {
  float t = 0;
  CHECK(sm::mlxRawToCelsius(0x3AD2, &t));
  CHECK_NEAR(t, 28.01, 0.005);
  CHECK(!sm::mlxRawToCelsius(0x8000 | 0x3AD2, &t));
  CHECK_NEAR(sm::irCorrect(45.0f, 21.0f, 1.0f), 45.0, 1e-4);
  // Con emissivita' < 1 la temperatura reale e' piu' alta di quella letta.
  const float corrected = sm::irCorrect(45.0f, 21.0f, 0.9f);
  CHECK(corrected > 46.5f && corrected < 49.0f);
  // Calibrazione: l'emissivita' calcolata riporta esattamente il valore reale.
  const float e = sm::irEmissivityFromReference(44.0f, 21.0f, 50.0f);
  CHECK(e > 0.7f && e < 0.85f);
  CHECK_NEAR(sm::irCorrect(44.0f, 21.0f, e), 50.0, 0.01);
  CHECK(isnan(sm::irEmissivityFromReference(24.0f, 21.0f, 25.0f)));  // troppo vicino all'ambiente
}

static void testMedian() {
  sm::Median5 m;
  CHECK_NEAR(m.add(40), 40, 1e-6);
  m.add(41);
  m.add(90);  // picco isolato
  m.add(42);
  CHECK_NEAR(m.add(43), 42, 1e-6);
}

static void testNtc() {
  CHECK_NEAR(ntcCelsius(10000.0f, 10000.0f, 3950.0f), 25.0, 0.01);
  CHECK_NEAR(ntcCelsius(3588.0f, 10000.0f, 3950.0f), 50.0, 0.1);
  // 3300 mV sul partitore con 10k fissa e NTC a 10k -> meta' tensione.
  CHECK_NEAR(ntcResistanceOhm(1650.0f, 3300.0f, 10000.0f), 10000.0, 0.5);
  CHECK(isnan(ntcResistanceOhm(0.0f, 3300.0f, 10000.0f)));
  CHECK(isnan(ntcResistanceOhm(3300.0f, 3300.0f, 10000.0f)));
}

int main() {
  struct {
    const char* name;
    void (*fn)();
  } tests[] = {
      {"ciclo base a temperatura", testBasicTemperatureCycle},
      {"picco isolato ignorato", testSpikeRejected},
      {"timer post-soglia", testOverrun},
      {"limite di sicurezza in post-soglia", testOverrunSafetyLimit},
      {"sopra il limite salta il post-soglia", testAboveMaxSkipsOverrun},
      {"riaccensione a timer", testTimerRestart},
      {"timer riarmato se ancora caldo", testTimerRearmsWhileStillHot},
      {"timer o temperatura", testTimerOrTemperature},
      {"timer e temperatura", testTimerAndTemperature},
      {"mai riaccendere", testNeverRestart},
      {"sblocco cambiando modalita' di riaccensione", testLockoutReleasedWhenRestartChanges},
      {"guasto sensore -> consenti", testSensorFaultAllowsHeating},
      {"guasto sensore -> blocca", testSensorFaultBlocksHeating},
      {"blocco resta con sensore guasto", testLockoutStaysBlockedOnFault},
      {"periodo di grazia all'avvio", testBootGracePeriod},
      {"periodo di grazia al cambio di sorgente", testResetSensorGivesGrace},
      {"modalita' manuali", testManualModes},
      {"ritorno in automatico", testBackToAutoRestartsCycle},
      {"avvio in blocco", testBeginWithLockout},
      {"pausa ripresa dopo un riavvio", testResumeCooldownAfterReboot},
      {"stato da salvare", testPersistentState},
      {"conferma zero", testZeroConfirmActsImmediately},
      {"validazione parametri", testSanitize},
      {"CRC e pacchetti", testCrcAndPackets},
      {"DS18B20", testDs18b20Parse},
      {"MLX90614 ed emissivita'", testMlxAndEmissivity},
      {"mediana", testMedian},
      {"NTC", testNtc},
  };
  for (const auto& t : tests) {
    const int before = g_failures;
    t.fn();
    printf("%s %s\n", g_failures == before ? "[ok]  " : "[KO]  ", t.name);
  }
  printf("\n%d controlli, %d falliti\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
