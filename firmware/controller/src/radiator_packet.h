// Pacchetti ESP-NOW tra nodo sensore a batteria e centralina.
// QUESTO FILE E' IDENTICO in firmware/controller/src e firmware/sensor_node/src:
// se lo modifichi, copialo anche nell'altra cartella (i test lo verificano).
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define RP_MAGIC 0x5254u
#define RP_VERSION 1
// Identificativo dell'impianto: deve essere uguale su nodo e centralina.
// Cambialo se nelle vicinanze c'e' un altro impianto uguale.
#define RP_DEFAULT_NET_ID 0x54524D31u
#define RP_TEMP_INVALID INT16_MIN

enum : uint8_t { RP_TYPE_DATA = 1, RP_TYPE_ACK = 2 };
enum : uint8_t { RP_FLAG_BATT_LOW = 0x01, RP_FLAG_SENSOR_ERROR = 0x02, RP_FLAG_FIRST_BOOT = 0x04 };
enum : uint8_t { RP_SENSOR_DS18B20 = 1, RP_SENSOR_NTC = 2 };

// Nodo -> centralina (broadcast).
struct __attribute__((packed)) RpData {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
  uint32_t netId;
  uint32_t seq;
  int16_t tempCx100;  // gradi C x 100, RP_TEMP_INVALID se il sensore non risponde
  uint16_t battMv;    // 0 = non misurata
  uint16_t intervalS; // intervallo di misura in uso
  uint8_t flags;
  uint8_t sensorType;
  uint16_t crc;
};

// Centralina -> nodo (broadcast, il nodo riconosce il proprio MAC).
struct __attribute__((packed)) RpAck {
  uint16_t magic;
  uint8_t version;
  uint8_t type;
  uint32_t netId;
  uint8_t nodeMac[6];
  uint32_t seq;            // stesso seq del pacchetto dati confermato
  uint16_t nextIntervalS;  // intervallo di misura suggerito al nodo
  uint8_t channel;         // canale Wi-Fi della centralina
  uint8_t controllerState;
  uint16_t crc;
};

static_assert(sizeof(RpData) == 22, "RpData deve restare di 22 byte");
static_assert(sizeof(RpAck) == 24, "RpAck deve restare di 24 byte");

// CRC-16/CCITT-FALSE (polinomio 0x1021, valore iniziale 0xFFFF).
inline uint16_t rpCrc16(const uint8_t* d, size_t n) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < n; ++i) {
    crc ^= static_cast<uint16_t>(d[i]) << 8;
    for (int b = 0; b < 8; ++b) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

template <typename T>
inline void rpSeal(T& p) {
  p.crc = rpCrc16(reinterpret_cast<const uint8_t*>(&p), sizeof(T) - sizeof(p.crc));
}

template <typename T>
inline bool rpDecode(const uint8_t* buf, size_t len, uint32_t netId, uint8_t type, T* out) {
  if (buf == nullptr || len != sizeof(T)) return false;
  memcpy(out, buf, sizeof(T));
  if (out->magic != RP_MAGIC || out->version != RP_VERSION || out->type != type || out->netId != netId) {
    return false;
  }
  return out->crc == rpCrc16(buf, sizeof(T) - sizeof(out->crc));
}
