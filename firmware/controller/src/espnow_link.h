// Ricezione ESP-NOW dai nodi sensore a batteria e invio delle conferme (ACK).
#pragma once

#include <stdint.h>

struct NodeInfo {
  bool used = false;
  uint8_t mac[6] = {0, 0, 0, 0, 0, 0};
  bool tempValid = false;
  float tempC = 0.0f;
  uint16_t battMv = 0;
  int8_t rssi = 0;
  uint8_t flags = 0;
  uint8_t sensorType = 0;
  uint16_t intervalS = 0;
  uint32_t seq = 0;
  uint32_t packets = 0;
  uint64_t lastSeenMs = 0;
};

namespace espnow_link {

const int kMaxNodes = 4;

// Da chiamare dopo WiFi.mode(...). La radio deve restare accesa (WiFi.setSleep(false)).
bool begin(uint32_t netId);
bool started();
// Da richiamare dopo ogni cambio di modalita' Wi-Fi.
void ensureBroadcastPeer();
// Dati inseriti negli ACK: intervallo di misura suggerito e stato della centralina.
void setAckInfo(uint16_t nextIntervalS, uint8_t controllerState);
// Copia della tabella nodi; restituisce quanti elementi sono validi in out.
int snapshot(NodeInfo out[kMaxNodes]);
// Numero di pacchetti dati validi ricevuti dall'accensione.
uint32_t packetCount();
// Massima temperatura valida tra i nodi sentiti negli ultimi timeoutMs.
bool maxTemperature(uint64_t nowMs, uint64_t timeoutMs, float* outC, int* freshNodes);

}  // namespace espnow_link
