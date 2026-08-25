#pragma once
// SPI.h - minimal Arduino SPIClass compat (display uses I2C; kept for the Adafruit SPI code paths).
#include <stdint.h>
#include <stddef.h>

#define MSBFIRST 0
#define LSBFIRST 1
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3
#define SPI_HAS_TRANSACTION 1
#define SPI_MSBFIRST MSBFIRST
#define SPI_LSBFIRST LSBFIRST

class SPISettings {
public:
    SPISettings() : clk(1000000), bitOrder(MSBFIRST), dataMode(SPI_MODE0) {}
    SPISettings(uint32_t clock, uint8_t bitOrder_, uint8_t dataMode_) {
        (void)clock; (void)bitOrder_; (void)dataMode_;
        clk = clock ? clock : 1000000; bitOrder = bitOrder_; dataMode = dataMode_;
    }
    uint32_t clk; uint8_t bitOrder; uint8_t dataMode;
};

class SPIClass {
public:
    void begin(int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1) { (void)sck;(void)miso;(void)mosi;(void)ss; }
    void end() {}
    void beginTransaction(SPISettings settings) { (void)settings; }
    void endTransaction() {}
    uint8_t transfer(uint8_t data) { (void)data; return 0; }
    size_t transfer(const uint8_t *tx, uint8_t *rx, size_t len) { (void)tx;(void)rx; return len; }
    void transfer(uint8_t *buf, size_t len) { (void)buf;(void)len; }
    void transfer(uint16_t *buf, size_t count) { (void)buf;(void)count; }
};

extern SPIClass SPI;