// Nodo sensore a batteria: si sveglia, misura il termosifone, trasmette via
// ESP-NOW solo se serve e torna in deep sleep. Tutto il lavoro e' in setup().
#include <Arduino.h>
#include <WiFi.h>
#include <esp_idf_version.h>
#include <esp_now.h>
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#if ESP_IDF_VERSION_MAJOR >= 5
#include <esp_random.h>
#endif

#include "config.h"
#include "radiator_packet.h"
#if NODE_SENSOR == NODE_SENSOR_DS18B20
#include "ds18b20.h"
#else
#include "ntc.h"
#endif

#if NODE_DEBUG
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...) \
  do {           \
  } while (0)
#endif

namespace {

const uint32_t kRtcMagic = 0x4E4F4431;
const uint8_t kBroadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Stato conservato nella RAM RTC durante il deep sleep.
RTC_DATA_ATTR uint32_t rtcMagic;
RTC_DATA_ATTR uint32_t rtcSeq;
RTC_DATA_ATTR uint8_t rtcChannel;
RTC_DATA_ATTR int16_t rtcLastSentCx100;
RTC_DATA_ATTR uint64_t rtcClockMs;  // orologio "virtuale": somma di veglia e sonno
RTC_DATA_ATTR uint64_t rtcLastTxMs;
RTC_DATA_ATTR uint16_t rtcIntervalS;
RTC_DATA_ATTR uint8_t rtcFailStreak;
RTC_DATA_ATTR bool rtcLastTxOk;

volatile bool g_ackReceived = false;
volatile uint16_t g_ackInterval = 0;
volatile uint8_t g_ackChannel = 0;
volatile uint32_t g_waitSeq = 0;
uint8_t g_myMac[6];

void handleAck(const uint8_t* data, int len) {
  if (data == nullptr || len <= 0) return;
  RpAck ack;
  if (!rpDecode(data, static_cast<size_t>(len), RP_DEFAULT_NET_ID, RP_TYPE_ACK, &ack)) return;
  if (memcmp(ack.nodeMac, g_myMac, 6) != 0 || ack.seq != g_waitSeq) return;
  g_ackInterval = ack.nextIntervalS;
  g_ackChannel = ack.channel;
  g_ackReceived = true;
}

#if ESP_IDF_VERSION_MAJOR >= 5
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  handleAck(data, len);
}
#else
void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  handleAck(data, len);
}
#endif

bool readTemperature(float* outC) {
  pinMode(PIN_SENSOR_POWER, OUTPUT);
  digitalWrite(PIN_SENSOR_POWER, HIGH);
  bool ok = false;
#if NODE_SENSOR == NODE_SENSOR_DS18B20
  delay(5);
  Ds18b20 ds(PIN_SENSOR_DATA);
  ds.begin();
  const uint32_t convMs = ds18b20ConversionMs(DS18B20_BITS);
  for (int attempt = 0; attempt < 2 && !ok; ++attempt) {
    // La risoluzione sta in RAM: va riscritta a ogni accensione del sensore.
    if (!ds.setResolution(DS18B20_BITS) || !ds.startConversion()) continue;
    const uint32_t t0 = millis();
    delay(convMs / 2);
    while (!ds.conversionDone() && millis() - t0 < convMs + 20) delay(5);
    ok = ds.readTemperature(outC);
  }
  ds.release();
#else
  delay(2);
  uint32_t sum = 0;
  for (int i = 0; i < NTC_SAMPLES; ++i) sum += analogReadMilliVolts(PIN_SENSOR_DATA);
  const float mv = static_cast<float>(sum) / NTC_SAMPLES;
  const float t = ntcCelsius(ntcResistanceOhm(mv, NTC_SUPPLY_MV, NTC_R_FIXED_OHM), NTC_R25_OHM, NTC_BETA);
  ok = !isnan(t) && t > -30.0f && t < 120.0f;
  if (ok) *outC = t;
#endif
  digitalWrite(PIN_SENSOR_POWER, LOW);
  return ok;
}

uint16_t readBatteryMv() {
#if BATT_MEASURE
  uint32_t sum = 0;
  for (int i = 0; i < 8; ++i) sum += analogReadMilliVolts(PIN_BATT_ADC);
  return static_cast<uint16_t>(static_cast<float>(sum) / 8.0f * BATT_DIVIDER);
#else
  return 0;
#endif
}

bool sendOnChannel(uint8_t channel, const RpData& pkt) {
  // Il passaggio per la modalita' promiscua rende il cambio di canale affidabile
  // anche sulle versioni di ESP-IDF che lo rifiutano a stazione non connessa.
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  g_ackReceived = false;
  g_waitSeq = pkt.seq;
  if (esp_now_send(kBroadcast, reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt)) != ESP_OK) return false;
  const uint32_t t0 = millis();
  while (!g_ackReceived && millis() - t0 < NODE_ACK_WAIT_MS) delay(1);
  return g_ackReceived;
}

// Prova il canale salvato; se la centralina non risponde li prova tutti (1-13).
bool transmit(const RpData& pkt) {
  WiFi.mode(WIFI_STA);
  WiFi.macAddress(g_myMac);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, kBroadcast, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  uint8_t channel = rtcChannel >= 1 && rtcChannel <= 13 ? rtcChannel : 1;
  bool ok = sendOnChannel(channel, pkt) || sendOnChannel(channel, pkt);
  for (uint8_t c = 1; c <= 13 && !ok; ++c) {
    if (c == channel) continue;
    if (sendOnChannel(c, pkt)) {
      ok = true;
      channel = c;
    }
  }
  if (ok) rtcChannel = g_ackChannel >= 1 && g_ackChannel <= 13 ? g_ackChannel : channel;
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
  return ok;
}

void blink(int times, int onMs, int offMs) {
#if PIN_STATUS_LED >= 0
  pinMode(PIN_STATUS_LED, OUTPUT);
  for (int i = 0; i < times; ++i) {
    digitalWrite(PIN_STATUS_LED, STATUS_LED_ON);
    delay(onMs);
    digitalWrite(PIN_STATUS_LED, !STATUS_LED_ON);
    delay(offMs);
  }
#else
  (void)times;
  (void)onMs;
  (void)offMs;
#endif
}

}  // namespace

void setup() {
#if NODE_DEBUG
  Serial.begin(115200);
  delay(300);
#endif
  const bool coldBoot = rtcMagic != kRtcMagic;
  if (coldBoot) {
    rtcMagic = kRtcMagic;
    rtcSeq = esp_random();
    rtcChannel = 1;
    rtcLastSentCx100 = RP_TEMP_INVALID;
    rtcClockMs = 0;
    rtcLastTxMs = 0;
    rtcIntervalS = NODE_DEFAULT_INTERVAL_S;
    rtcFailStreak = 0;
    rtcLastTxOk = false;
  }

  float t = NAN;
  const bool tempOk = readTemperature(&t);
  const uint16_t battMv = readBatteryMv();
  const bool battLow = battMv != 0 && battMv < BATT_LOW_MV;
  const bool battCritical = battMv != 0 && battMv < BATT_CRITICAL_MV;
  const int16_t cx100 = tempOk ? static_cast<int16_t>(lroundf(t * 100.0f)) : RP_TEMP_INVALID;

  const bool changed = cx100 == RP_TEMP_INVALID || rtcLastSentCx100 == RP_TEMP_INVALID ||
                       abs(cx100 - rtcLastSentCx100) >= NODE_DELTA_CX100;
  const bool heartbeat = rtcClockMs - rtcLastTxMs >= static_cast<uint64_t>(NODE_HEARTBEAT_S) * 1000ULL;
  const bool mustSend = coldBoot || changed || heartbeat || !rtcLastTxOk;
  LOG("[nodo] T=%.2f ok=%d batt=%u mV invio=%d\n", t, tempOk, battMv, mustSend);

  bool sent = false;
  if (mustSend) {
    RpData pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.magic = RP_MAGIC;
    pkt.version = RP_VERSION;
    pkt.type = RP_TYPE_DATA;
    pkt.netId = RP_DEFAULT_NET_ID;
    pkt.seq = ++rtcSeq;
    pkt.tempCx100 = cx100;
    pkt.battMv = battMv;
    pkt.intervalS = rtcIntervalS;
    pkt.flags = static_cast<uint8_t>((battLow ? RP_FLAG_BATT_LOW : 0) | (tempOk ? 0 : RP_FLAG_SENSOR_ERROR) |
                                     (coldBoot ? RP_FLAG_FIRST_BOOT : 0));
    pkt.sensorType = NODE_SENSOR == NODE_SENSOR_DS18B20 ? RP_SENSOR_DS18B20 : RP_SENSOR_NTC;
    rpSeal(pkt);
    sent = transmit(pkt);
    rtcLastTxOk = sent;
    if (sent) {
      rtcLastSentCx100 = cx100;
      rtcLastTxMs = rtcClockMs;
      rtcFailStreak = 0;
      uint16_t next = g_ackInterval;
      if (next != 0) {
        if (next < NODE_MIN_INTERVAL_S) next = NODE_MIN_INTERVAL_S;
        if (next > NODE_MAX_INTERVAL_S) next = NODE_MAX_INTERVAL_S;
        rtcIntervalS = next;
      }
    } else if (rtcFailStreak < 255) {
      ++rtcFailStreak;
    }
    LOG("[nodo] inviato=%d canale=%u prossimo=%u s\n", sent, rtcChannel, rtcIntervalS);
  }
  // Al primo avvio il LED dice se la centralina ha risposto: 3 lampi lenti = si', 10 rapidi = no.
  if (coldBoot) {
    if (sent) blink(3, 300, 300);
    else blink(10, 60, 60);
  }

  uint32_t sleepS = rtcIntervalS;
  if (mustSend && !sent) {
    // Centralina spenta o lontana: attese sempre piu' lunghe per non scaricare la batteria.
    const uint8_t shift = rtcFailStreak < 5 ? rtcFailStreak : 5;
    sleepS = static_cast<uint32_t>(rtcIntervalS) << shift;
    if (sleepS > NODE_MAX_INTERVAL_S) sleepS = NODE_MAX_INTERVAL_S;
  }
  if (battCritical) sleepS = 3600;
  rtcClockMs += millis() + static_cast<uint64_t>(sleepS) * 1000ULL;
#if NODE_DEBUG
  Serial.flush();
#endif
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(sleepS) * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {}
