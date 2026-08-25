#pragma once
// Print.h - Arduino-compatible output base class (ESP-IDF native).
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "WString.h"

struct __FlashStringHelper;

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print {
public:
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) { n += write(*buffer++); }
        return n;
    }

    size_t print(const String &s) { return write((const uint8_t *)s.c_str(), s.length()); }
    size_t print(const char *s)   { return write((const uint8_t *)s, strlen(s)); }
    size_t print(char c)          { return write((uint8_t)c); }
    size_t print(unsigned char c, int base = DEC) { return printNumber((unsigned long)c, base); }
    size_t print(int v, int base = DEC)           { return printNumber((unsigned long)v, base); }
    size_t print(unsigned int v, int base = DEC)  { return printNumber((unsigned long)v, base); }
    size_t print(long v, int base = DEC)          { return printNumber((unsigned long)v, base); }
    size_t print(unsigned long v, int base = DEC) { return printNumber(v, base); }
    size_t print(float v, int digits = 2)         { return printFloat(v, digits); }
    size_t print(double v, int digits = 2)        { return printFloat((float)v, digits); }

    size_t println(const String &s) { size_t n = print(s); n += println(); return n; }
    size_t println(const char *s)   { size_t n = print(s); n += println(); return n; }
    size_t println(char c)          { size_t n = print(c); n += println(); return n; }
    size_t println(unsigned char c, int base = DEC) { size_t n = print(c, base); n += println(); return n; }
    size_t println(int v, int base = DEC)           { size_t n = print(v, base); n += println(); return n; }
    size_t println(unsigned int v, int base = DEC)  { size_t n = print(v, base); n += println(); return n; }
    size_t println(long v, int base = DEC)          { size_t n = print(v, base); n += println(); return n; }
    size_t println(unsigned long v, int base = DEC) { size_t n = print(v, base); n += println(); return n; }
    size_t println(float v, int digits = 2)         { size_t n = print(v, digits); n += println(); return n; }
    size_t println(double v, int digits = 2)        { size_t n = print(v, digits); n += println(); return n; }
    size_t println(void) { return write((uint8_t)'\n'); }

    size_t printf(const char *fmt, ...) __attribute__((format(printf,2,3))) {
        char buf[256];
        va_list ap; va_start(ap, fmt); int n = vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
        if (n <= 0) return 0;
        return write((const uint8_t *)buf, (size_t)((n < (int)sizeof(buf)) ? n : sizeof(buf) - 1));
    }

protected:
    size_t printNumber(unsigned long v, int base) {
        char buf[8 * sizeof(unsigned long) + 1];
        char *str = &buf[sizeof(buf) - 1];
        *str = '\0';
        if (base < 2) base = 10;
        do { char c = (char)(v % base); v /= base; *--str = c < 10 ? ('0' + c) : ('A' + c - 10); } while (v);
        return print(str);
    }
    size_t printFloat(float v, int digits) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.*f", (digits < 0 ? 0 : digits), (double)v);
        return print(buf);
    }
};

class Stream : public Print {
public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void setTimeout(unsigned long timeout) { (void)timeout; }
    int parseInt() { int c; int r = 0; while ((c = read()) != -1 && c >= '0' && c <= '9') r = r * 10 + (c - '0'); return r; }
    float parseFloat() { return (float)parseInt(); }
    String readString() { String s; int c; while ((c = read()) != -1) s += (char)c; return s; }
};