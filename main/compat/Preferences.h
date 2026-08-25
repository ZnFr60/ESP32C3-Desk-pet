#pragma once
// Preferences.h - Arduino Preferences over ESP-IDF native NVS (nvs_flash).
#include <stdint.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "WString.h"

class Preferences {
public:
    Preferences() : m_handle(0), m_open(false) {}

    bool begin(const char *name, bool readOnly = false) {
        if (m_open) end();
        nvs_open(name, readOnly ? NVS_READONLY : NVS_READWRITE, &m_handle);
        m_open = (m_handle != 0);
        m_ro = readOnly;
        return m_open;
    }
    void end() { if (m_open) { nvs_close(m_handle); m_open = false; } }

    int getInt(const char *key, int def = 0) { int32_t v = def; if (m_open) nvs_get_i32(m_handle, key, &v); return (int)v; }
    unsigned int getUInt(const char *key, unsigned int def = 0) { uint32_t v = def; if (m_open) nvs_get_u32(m_handle, key, &v); return v; }
    unsigned long getULong(const char *key, unsigned long def = 0) { return (unsigned long)getUInt(key, (unsigned int)def); }
    long getLong(const char *key, long def = 0) { return (long)getInt(key, (int)def); }
    bool getBool(const char *key, bool def = false) { uint8_t v = def ? 1 : 0; if (m_open) nvs_get_u8(m_handle, key, &v); return v != 0; }
    String getString(const char *key, const char *def = "") {
        if (!m_open) return String(def);
        size_t len = 0;
        if (nvs_get_str(m_handle, key, nullptr, &len) != ESP_OK || len == 0) return String(def);
        char *buf = (char *)malloc(len);
        if (!buf) return String(def);
        nvs_get_str(m_handle, key, buf, &len);
        String r(buf); free(buf); return r;
    }

    bool putInt(const char *key, int v) { return m_open && nvs_set_i32(m_handle, key, (int32_t)v) == ESP_OK && commit(); }
    bool putUInt(const char *key, unsigned int v) { return m_open && nvs_set_u32(m_handle, key, (uint32_t)v) == ESP_OK && commit(); }
    bool putULong(const char *key, unsigned long v) { return putUInt(key, (unsigned int)v); }
    bool putLong(const char *key, long v) { return putInt(key, (int)v); }
    bool putBool(const char *key, bool v) { return m_open && nvs_set_u8(m_handle, key, v ? 1 : 0) == ESP_OK && commit(); }
    bool putString(const char *key, const char *v) {
        if (!m_open) return false;
        if (nvs_set_str(m_handle, key, v) == ESP_OK) return commit();
        return false;
    }
    bool putString(const char *key, const String &v) { return putString(key, v.c_str()); }

private:
    bool commit() { return nvs_commit(m_handle) == ESP_OK; }
    nvs_handle_t m_handle;
    bool m_open, m_ro;
};
