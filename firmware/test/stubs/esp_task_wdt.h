#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef int esp_err_t;
typedef struct {
  uint32_t timeout_ms;
  uint32_t idle_core_mask;
  bool trigger_panic;
} esp_task_wdt_config_t;
esp_err_t esp_task_wdt_reconfigure(const esp_task_wdt_config_t* config);
