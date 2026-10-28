#include "espnow_link.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_idf_version.h>
#include <esp_now.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <string.h>

#include "radiator_packet.h"

namespace espnow_link {

namespace {

struct RxItem {
  uint8_t mac[6];
  int8_t rssi;
  RpData pkt;
};

const uint8_t kBroadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

QueueHandle_t s_queue = nullptr;
portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
NodeInfo s_nodes[kMaxNodes];
volatile uint32_t s_packets = 0;
volatile uint16_t s_nextIntervalS = 60;
volatile uint8_t s_controllerState = 0;
uint32_t s_netId = RP_DEFAULT_NET_ID;
bool s_started = false;

// Gira nel task Wi-Fi: solo controllo e accodamento, niente operazioni lunghe.
void handleRx(const uint8_t* mac, const uint8_t* data, int len, int8_t rssi) {
  if (mac == nullptr || data == nullptr || len <= 0 || s_queue == nullptr) return;
  RxItem item;
  if (!rpDecode(data, static_cast<size_t>(len), s_netId, RP_TYPE_DATA, &item.pkt)) return;
  memcpy(item.mac, mac, 6);
  item.rssi = rssi;
  xQueueSend(s_queue, &item, 0);
}

#if ESP_IDF_VERSION_MAJOR >= 5
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (info == nullptr) return;
  const int8_t rssi = info->rx_ctrl != nullptr ? static_cast<int8_t>(info->rx_ctrl->rssi) : 0;
  handleRx(info->src_addr, data, len, rssi);
}
#else
void onRecv(const uint8_t* mac, const uint8_t* data, int len) { handleRx(mac, data, len, 0); }
#endif

int findSlot(const uint8_t* mac) {
  int freeSlot = -1;
  int oldest = 0;
  for (int i = 0; i < kMaxNodes; ++i) {
    if (s_nodes[i].used && memcmp(s_nodes[i].mac, mac, 6) == 0) return i;
    if (!s_nodes[i].used && freeSlot < 0) freeSlot = i;
    if (s_nodes[i].lastSeenMs < s_nodes[oldest].lastSeenMs) oldest = i;
  }
  return freeSlot >= 0 ? freeSlot : oldest;
}

// Task dedicato: risponde subito al nodo (che resta in ascolto pochi ms)
// anche se il loop principale e' occupato a servire la pagina web.
void ackTask(void*) {
  RxItem item;
  for (;;) {
    if (xQueueReceive(s_queue, &item, portMAX_DELAY) != pdTRUE) continue;

    RpAck ack;
    memset(&ack, 0, sizeof(ack));
    ack.magic = RP_MAGIC;
    ack.version = RP_VERSION;
    ack.type = RP_TYPE_ACK;
    ack.netId = s_netId;
    memcpy(ack.nodeMac, item.mac, 6);
    ack.seq = item.pkt.seq;
    ack.nextIntervalS = s_nextIntervalS;
    uint8_t channel = 0;
    wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    if (esp_wifi_get_channel(&channel, &second) == ESP_OK) ack.channel = channel;
    ack.controllerState = s_controllerState;
    rpSeal(ack);
    esp_now_send(kBroadcast, reinterpret_cast<const uint8_t*>(&ack), sizeof(ack));

    const uint64_t now = static_cast<uint64_t>(esp_timer_get_time() / 1000LL);
    portENTER_CRITICAL(&s_mux);
    const int i = findSlot(item.mac);
    NodeInfo& n = s_nodes[i];
    if (!n.used || memcmp(n.mac, item.mac, 6) != 0) {
      n = NodeInfo();
      n.used = true;
      memcpy(n.mac, item.mac, 6);
    }
    n.tempValid = item.pkt.tempCx100 != RP_TEMP_INVALID && (item.pkt.flags & RP_FLAG_SENSOR_ERROR) == 0;
    n.tempC = item.pkt.tempCx100 / 100.0f;
    n.battMv = item.pkt.battMv;
    n.rssi = item.rssi;
    n.flags = item.pkt.flags;
    n.sensorType = item.pkt.sensorType;
    n.intervalS = item.pkt.intervalS;
    n.seq = item.pkt.seq;
    n.packets++;
    n.lastSeenMs = now;
    s_packets = s_packets + 1;
    portEXIT_CRITICAL(&s_mux);
  }
}

}  // namespace

bool begin(uint32_t netId) {
  s_netId = netId;
  if (s_started) return true;
  if (esp_now_init() != ESP_OK) return false;
  if (s_queue == nullptr) {
    s_queue = xQueueCreate(8, sizeof(RxItem));
    xTaskCreate(ackTask, "espnow_ack", 4096, nullptr, 5, nullptr);
  }
  esp_now_register_recv_cb(onRecv);
  s_started = true;
  ensureBroadcastPeer();
  return true;
}

bool started() { return s_started; }

void ensureBroadcastPeer() {
  if (!s_started || esp_now_is_peer_exist(kBroadcast)) return;
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, kBroadcast, 6);
  peer.channel = 0;  // canale corrente
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void setAckInfo(uint16_t nextIntervalS, uint8_t controllerState) {
  s_nextIntervalS = nextIntervalS;
  s_controllerState = controllerState;
}

int snapshot(NodeInfo out[kMaxNodes]) {
  int count = 0;
  portENTER_CRITICAL(&s_mux);
  for (int i = 0; i < kMaxNodes; ++i) {
    if (s_nodes[i].used) out[count++] = s_nodes[i];
  }
  portEXIT_CRITICAL(&s_mux);
  return count;
}

uint32_t packetCount() { return s_packets; }

bool maxTemperature(uint64_t nowMs, uint64_t timeoutMs, float* outC, int* freshNodes) {
  bool any = false;
  float best = 0.0f;
  int fresh = 0;
  portENTER_CRITICAL(&s_mux);
  for (int i = 0; i < kMaxNodes; ++i) {
    const NodeInfo& n = s_nodes[i];
    if (!n.used) continue;
    // lastSeenMs viene scritto da un altro task: puo' essere di poco nel futuro.
    const uint64_t age = nowMs > n.lastSeenMs ? nowMs - n.lastSeenMs : 0;
    if (age > timeoutMs) continue;
    ++fresh;
    if (n.tempValid && (!any || n.tempC > best)) {
      best = n.tempC;
      any = true;
    }
  }
  portEXIT_CRITICAL(&s_mux);
  if (freshNodes != nullptr) *freshNodes = fresh;
  if (any) *outC = best;
  return any;
}

}  // namespace espnow_link
