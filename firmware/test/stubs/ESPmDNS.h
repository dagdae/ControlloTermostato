#pragma once
#include <stdint.h>
class MDNSResponder {
 public:
  bool begin(const char* hostName);
  bool addService(const char* service, const char* proto, uint16_t port);
};
extern MDNSResponder MDNS;
