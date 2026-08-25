#pragma once
// Minimal Arduino-compatible String class implemented on ESP-IDF native APIs.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

class String {
public:
    String() : m_len(0), m_cap(0), m_buf(nullptr) {}
    String(const char *s) { init(s ? s : "", s ? strlen(s) : 0); }
    String(const String &o) { init(o.c_str(), o.length()); }
    String(char c) { init(&c, 1); }
    String(unsigned char c) { init((const char *)&c, 1); }
    String(int v)  { fromInt((long)v); }
    String(unsigned int v)  { fromUInt((unsigned long)v); }
    String(long v) { fromInt(v); }
    String(unsigned long v) { fromUInt(v); }
    String(float v, unsigned char dp = 2) { fromFloat(v, dp); }
    String(double v, unsigned char dp = 2) { fromFloat((float)v, dp); }

    ~String() { if (m_buf) free(m_buf); }

    String &operator=(const String &o) { if (this != &o) init(o.c_str(), o.length()); return *this; }
    String &operator=(const char *s)  { init(s ? s : "", s ? strlen(s) : 0); return *this; }
    String &operator=(char c)         { init(&c, 1); return *this; }

    String &operator+=(const String &o) { concat(o.c_str(), o.length()); return *this; }
    String &operator+=(const char *s)   { concat(s ? s : "", s ? strlen(s) : 0); return *this; }
    String &operator+=(char c)          { concat(&c, 1); return *this; }
    String &operator+=(int v)           { return appendFmt("%d", (int)v); }
    String &operator+=(unsigned int v)  { return appendFmt("%u", (unsigned)v); }
    String &operator+=(long v)          { return appendFmt("%ld", v); }
    String &operator+=(unsigned long v) { return appendFmt("%lu", v); }
    String &operator+=(float v)         { return appendFmt("%.2f", (double)v); }

    String operator+(const String &o) const { String r(*this); r += o; return r; }
    String operator+(const char *s)   const { String r(*this); r += s; return r; }
    String operator+(char c)          const { String r(*this); r += c; return r; }
    String operator+(int v)           const { String r(*this); r += v; return r; }
    String operator+(unsigned int v)  const { String r(*this); r += v; return r; }
    String operator+(long v)          const { String r(*this); r += v; return r; }
    String operator+(unsigned long v) const { String r(*this); r += v; return r; }

    bool operator==(const String &o) const { return equals(o.c_str(), o.length()); }
    bool operator==(const char *s) const   { return equals(s ? s : "", s ? strlen(s) : 0); }
    bool operator!=(const String &o) const { return !equals(o.c_str(), o.length()); }
    bool operator!=(const char *s) const   { return !equals(s ? s : "", s ? strlen(s) : 0); }
    bool operator>(const String &o) const { return strcmp(c_str(), o.c_str()) > 0; }
    bool operator<(const String &o) const { return strcmp(c_str(), o.c_str()) < 0; }

    const char *c_str() const { return m_buf ? m_buf : ""; }
    unsigned int length() const { return m_len; }
    unsigned int size() const { return m_len; }
    bool isEmpty() const { return m_len == 0; }
    char charAt(unsigned int i) const { return (i < m_len) ? m_buf[i] : 0; }
    char operator[](unsigned int i) const { return (i < m_len) ? m_buf[i] : 0; }

    long toInt() const { return m_len ? strtol(m_buf, nullptr, 10) : 0; }
    float toFloat() const { return m_len ? strtof(m_buf, nullptr) : 0.0f; }

    String &replace(char find, char rep) {
        for (unsigned int i = 0; i < m_len; i++) if (m_buf[i] == find) m_buf[i] = rep;
        return *this;
    }
    String &replace(const char *find, const char *rep) {
        if (!find || !*find) return *this;
        String out;
        const char *p = m_buf, *q;
        while ((q = strstr(p, find)) != nullptr) {
            out.concat(p, (unsigned int)(q - p));
            out += rep;
            p = q + strlen(find);
        }
        out.concat(p, (unsigned int)strlen(p));
        *this = out;
        return *this;
    }
    String substring(unsigned int from, unsigned int to = 0) const {
        if (to == 0 || to > m_len) to = m_len;
        if (from >= to) return String();
        char *tmp = (char *)malloc(to - from + 1);
        if (!tmp) return String();
        memcpy(tmp, m_buf + from, to - from); tmp[to - from] = 0;
        String r(tmp); free(tmp); return r;
    }
    int indexOf(char c, unsigned int from = 0) const {
        for (unsigned int i = from; i < m_len; i++) if (m_buf[i] == c) return (int)i;
        return -1;
    }
    void trim() {
        while (m_len > 0 && (m_buf[0] == ' ' || m_buf[0] == '\t' || m_buf[0] == '\n' || m_buf[0] == '\r')) {
            memmove(m_buf, m_buf + 1, m_len - 1); m_len--;
        }
        while (m_len > 0 && (m_buf[m_len-1] == ' ' || m_buf[m_len-1] == '\t' || m_buf[m_len-1] == '\n' || m_buf[m_len-1] == '\r')) m_len--;
        ensure(m_len);
    }

private:
    void init(const char *s, unsigned int n) {
        m_len = n; m_cap = n + 1;
        m_buf = (char *)malloc(m_cap ? m_cap : 1);
        if (!m_buf) { m_buf = (char *)malloc(1); if (m_buf) { m_cap = 1; m_len = 0; m_buf[0] = 0; } return; }
        memcpy(m_buf, s, n); m_buf[n] = 0;
    }
    void ensure(unsigned int need) {
        if (need + 1 <= m_cap) { if (m_buf) m_buf[m_len] = 0; return; }
        unsigned int nc = m_cap ? m_cap : 16;
        while (nc < need + 1) nc *= 2;
        char *nb = (char *)realloc(m_buf, nc);
        if (nb) { m_buf = nb; m_cap = nc; }
        if (m_buf) m_buf[m_len] = 0;
    }
    void concat(const char *s, unsigned int n) {
        ensure(m_len + n);
        if (!m_buf) return;
        memcpy(m_buf + m_len, s, n);
        m_len += n;
        m_buf[m_len] = 0;
    }
    bool equals(const char *s, unsigned int n) const {
        if (m_len != n) return false;
        return memcmp(m_buf, s, n) == 0;
    }
    String &appendFmt(const char *fmt, ...) __attribute__((format(printf,2,3))) {
        char tmp[64];
        va_list ap; va_start(ap, fmt); int n = vsnprintf(tmp, sizeof(tmp), fmt, ap); va_end(ap);
        if (n > 0) concat(tmp, (unsigned int)n);
        return *this;
    }
    void fromInt(long v) { char tmp[32]; snprintf(tmp, sizeof(tmp), "%ld", v); init(tmp, strlen(tmp)); }
    void fromUInt(unsigned long v) { char tmp[32]; snprintf(tmp, sizeof(tmp), "%lu", v); init(tmp, strlen(tmp)); }
    void fromFloat(float v, unsigned char dp) {
        char tmp[48]; snprintf(tmp, sizeof(tmp), "%.*f", (int)dp, (double)v); init(tmp, strlen(tmp));
    }
    unsigned int m_len, m_cap;
    char *m_buf;
};

static inline String operator+(const char *lhs, const String &rhs) { String r(lhs); r += rhs; return r; }
static inline String operator+(char lhs, const String &rhs) { String r(lhs); r += rhs; return r; }