#pragma once
// ============================================================================
// Arduino.h - ESP-IDF native compatibility core for the pet-robot migration.
// Replaces the Arduino-ESP32 core with direct ESP-IDF native driver calls
// (driver/gpio, esp_timer, esp_random, freertos, esp_sntp). No arduino-esp32.
// ============================================================================
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <time.h>

#include "esp_timer.h"
#include "esp_random.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "WString.h"
#include "Print.h"
#include "Serial.h"
#include "pgmspace.h"

// ---- build/arch markers the Adafruit libs branch on ----
#ifndef ESP32
#define ESP32
#endif
#ifndef ESP32C3
#define ESP32C3
#endif
#ifndef ARDUINO
#define ARDUINO 10607
#endif
#define F(x) (x)
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

// ---- Arduino data type aliases ----
#ifndef ARDUINO_BYTE_TYPE
#define ARDUINO_BYTE_TYPE
typedef uint8_t byte;
typedef uint16_t word;
typedef bool boolean;
#endif

// ---- pin / logic constants ----
#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
#define INPUT_PULLDOWN 0x3
#define CHANGE 1
#define FALLING 2
#define RISING 3
#ifndef LED_BUILTIN
#define LED_BUILTIN 8
#endif

// ---- GPIO via driver/gpio ----
static inline void pinMode(uint8_t pin, uint8_t mode) {
    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << pin);
    io.mode = (mode == INPUT || mode == INPUT_PULLUP || mode == INPUT_PULLDOWN)
                  ? GPIO_MODE_INPUT
                  : GPIO_MODE_OUTPUT;
    io.pull_up_en = (mode == INPUT_PULLUP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io.pull_down_en = (mode == INPUT_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);
}
static inline void digitalWrite(uint8_t pin, uint8_t val) { gpio_set_level((gpio_num_t)pin, val ? 1 : 0); }
static inline int digitalRead(uint8_t pin) { return gpio_get_level((gpio_num_t)pin); }

// Fast port IO macros used only by Adafruit's software-SPI bit-bang path,
// which is never active at runtime in this firmware (the OLED/MPU are I2C).
#define digitalPinToPort(p) ((uint8_t)0)
#define portOutputRegister(port) ((volatile uint32_t *)(uintptr_t)(port))
#define portInputRegister(port) ((volatile uint32_t *)(uintptr_t)(port))
#define digitalPinToBitMask(p) (1UL << (p))

// ---- time (esp_timer) ----
static inline unsigned long millis(void) { return (unsigned long)(esp_timer_get_time() / 1000); }
static inline unsigned long micros(void) { return (unsigned long)esp_timer_get_time(); }
static inline void delay(unsigned long ms) {
    if (ms == 0) { taskYIELD(); }
    else { vTaskDelay(pdMS_TO_TICKS(ms)); }
}
static inline void delayMicroseconds(unsigned int us) { if (us) ets_delay_us(us); }

// ---- PRNG (esp_random) ----
static uint64_t g_rngState = 0x9E3779B97F4A7C15ULL;
static inline uint32_t nextRand(void) {
    g_rngState ^= g_rngState >> 12;
    g_rngState ^= g_rngState << 25;
    g_rngState ^= g_rngState >> 27;
    return (uint32_t)((g_rngState * 0x2545F4914F6CDD1DULL) >> 32);
}
static inline void randomSeed(unsigned long seed) { g_rngState = seed ? seed : 1; }
static inline long random(long howbig) {
    if (howbig == 0) return 0;
    return (long)(nextRand() % (uint32_t)howbig);
}
static inline long random(long howsmall, long howbig) {
    if (howsmall >= howbig) return howsmall;
    long diff = howbig - howsmall;
    return howsmall + (long)(nextRand() % (uint32_t)diff);
}

// ---- math helpers (functions, NOT macros: macros would corrupt <map>/<algorithm>) ----
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
template<typename T> static inline const T& min(const T& a, const T& b) { return (a < b) ? a : b; }
template<typename T> static inline const T& max(const T& a, const T& b) { return (a > b) ? a : b; }
static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
static inline float radians(float deg) { return deg * (float)(PI / 180.0); }
static inline float degrees(float rad) { return rad * (float)(180.0 / PI); }
static inline float sq(float x) { return x * x; }

// ---- CPU frequency scaling: no-op (DVFS off; functionality preserved) ----
static inline void setCpuFrequencyMhz(uint32_t mhz) { (void)mhz; }

// ---- timezone / SNTP (esp_sntp) ----
static inline void configTime(long gmtOffset_sec, int daylightOffset_sec,
                              const char *server1, const char *server2 = nullptr,
                              const char *server3 = nullptr) {
    (void)server2; (void)server3; (void)daylightOffset_sec;
    char tz[24];
    long off = gmtOffset_sec;
    char sign = (off >= 0) ? '-' : '+';
    long hrs = (off < 0 ? -off : off) / 3600;
    snprintf(tz, sizeof(tz), "GMT%c%ld", sign, hrs);
    setenv("TZ", tz, 1);
    tzset();
    esp_netif_init();
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, server1);
    esp_sntp_init();
}
static inline bool getLocalTime(struct tm *info, uint32_t ms = 5000) {
    (void)ms;
    time_t now = time(nullptr);
    if (now < 100000) return false;
    struct tm t;
    if (localtime_r(&now, &t) == nullptr) return false;
    *info = t;
    return true;
}