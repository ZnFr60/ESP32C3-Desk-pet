#pragma once
// Serial.h - Arduino-compatible Serial over the IDF console (stdout).
#include "Print.h"
#include "WString.h"

class HardwareSerial : public Stream {
public:
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
public:
    void begin(unsigned long baud = 115200) { (void)baud; }
    void end() {}
    void flush() {}
    size_t write(uint8_t b) override { putchar((int)b); return 1; }
    size_t write(const uint8_t *buf, size_t size) override {
        for (size_t i = 0; i < size; i++) putchar((int)buf[i]);
        return size;
    }
};

extern HardwareSerial Serial;