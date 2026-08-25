#pragma once
// ESP-IDF native PROGMEM/pgm_read compat (flash data is directly addressable on ESP32-C3).
#include <stdint.h>
#include <stddef.h>
#define PROGMEM
#define PSTR(x) (x)
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))
#define pgm_read_ptr(addr) (*(const void **)(addr))
