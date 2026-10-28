#pragma once
#include "Arduino.h"
#include "esp_wifi.h"
#define WIFI_OFF WIFI_MODE_NULL
#define WIFI_STA WIFI_MODE_STA
#define WIFI_AP WIFI_MODE_AP
#define WIFI_AP_STA WIFI_MODE_APSTA
typedef enum { WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL = 1, WL_CONNECTED = 3, WL_DISCONNECTED = 6 } wl_status_t;
class IPAddress {
 public:
  String toString() const;
};
class WiFiClass {
 public:
  bool mode(wifi_mode_t m);
  void persistent(bool persistent);
  bool setHostname(const char* hostname);
  bool setSleep(bool enabled);
  bool setAutoReconnect(bool autoReconnect);
  bool disconnect(bool wifioff = false, bool eraseap = false);
  wl_status_t begin(const char* ssid, const char* passphrase = nullptr);
  wl_status_t status();
  IPAddress localIP();
  IPAddress softAPIP();
  int8_t RSSI();
  uint8_t channel();
  uint8_t* macAddress(uint8_t* mac);
  bool softAP(const char* ssid, const char* passphrase = nullptr, int channel = 1, int ssid_hidden = 0,
              int max_connection = 4);
  bool softAPdisconnect(bool wifioff = false);
  uint8_t softAPgetStationNum();
};
extern WiFiClass WiFi;
