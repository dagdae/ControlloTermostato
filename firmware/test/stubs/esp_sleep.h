#pragma once
#include <stdint.h>
typedef int esp_err_t;
esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us);
void esp_deep_sleep_start(void) __attribute__((noreturn));
