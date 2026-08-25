#pragma once
// util/delay.h compatibility (ESP-IDF native busy-wait delays).
#include <stdint.h>
#include "esp_timer.h"
static inline void _delay_ms(unsigned long ms) {
    int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < (int64_t)ms * 1000) { }
}
static inline void _delay_us(unsigned long us) {
    int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < (int64_t)us) { }
}
