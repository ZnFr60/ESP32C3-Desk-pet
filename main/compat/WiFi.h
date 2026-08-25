#pragma once
// WiFi.h - Arduino WiFi over ESP-IDF native esp_wifi / esp_netif / esp_event.
#include <stdint.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "WString.h"

// ---- Arduino-style WiFi mode constants (mapped to esp_wifi internally) ----
typedef enum { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 } WiFiMode_t;
typedef enum {
    WL_IDLE_STATUS = 0,
    WL_NO_SSID_AVAIL = 1,
    WL_SCAN_COMPLETED = 2,
    WL_CONNECTED = 3,
    WL_CONNECT_FAILED = 4,
    WL_CONNECTION_LOST = 5,
    WL_DISCONNECTED = 6
} wl_status_t;

class IPAddress {
public:
    IPAddress() { set(0, 0, 0, 0); }
    IPAddress(uint32_t ip) {
        m_b[0] = (ip >> 0) & 0xFF; m_b[1] = (ip >> 8) & 0xFF;
        m_b[2] = (ip >> 16) & 0xFF; m_b[3] = (ip >> 24) & 0xFF;
    }
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) { set(a, b, c, d); }
    void set(uint8_t a, uint8_t b, uint8_t c, uint8_t d) { m_b[0]=a; m_b[1]=b; m_b[2]=c; m_b[3]=d; }
    bool isSet() const { return m_b[0]|m_b[1]|m_b[2]|m_b[3]; }
    String toString() const {
        char buf[24]; snprintf(buf, sizeof(buf), "%u.%u.%u.%u", m_b[0], m_b[1], m_b[2], m_b[3]);
        return String(buf);
    }
    uint8_t operator[](int i) const { return m_b[i]; }
    bool operator==(const IPAddress &o) const { return memcmp(m_b, o.m_b, 4) == 0; }
    uint8_t m_b[4];
};

class WiFiClass {
public:
    static bool init();
    static void mode(int m);
    static wl_status_t begin(const char *ssid, const char *password = nullptr);
    static wl_status_t status();
    static void disconnect(bool wifioff = false);
    static IPAddress localIP();
    static bool softAP(const char *ssid, const char *password = nullptr);
    static IPAddress softAPIP();
    static void softAPdisconnect(bool wifioff = true);
    static void softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet) {
        (void)local_ip; (void)gateway; (void)subnet;
    }
    static int scanNetworks();
    static String SSID(int i);
    static int32_t RSSI(int i);
};

extern WiFiClass WiFi;

// ---- internal shared state ----
namespace wifi_internal {
extern bool g_initialized;
extern bool g_connected;
extern bool g_staRunning;
extern bool g_apRunning;
extern IPAddress g_ip;
extern int g_scanCount;
extern wifi_ap_record_t g_aps[32];
}