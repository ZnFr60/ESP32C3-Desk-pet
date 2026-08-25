// compat_impl.cpp - definitions for the ESP-IDF native Arduino-compat globals.
#include "Arduino.h"
#include "Serial.h"
#include "Wire.h"
#include "SPI.h"
#include "WiFi.h"

HardwareSerial Serial;
TwoWire Wire;

// ---------------- SPI (unused at runtime; display is I2C) ----------------
SPIClass SPI;

// ---------------- WiFi over esp_wifi / esp_netif / esp_event ----------------
namespace wifi_internal {
bool g_initialized = false;
bool g_connected = false;
bool g_staRunning = false;
bool g_apRunning = false;
IPAddress g_ip;
int g_scanCount = 0;
wifi_ap_record_t g_aps[32];
}

using namespace wifi_internal;

static void wifiEventHandler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_DISCONNECTED:
                g_connected = false;
                break;
            default:
                break;
        }
    } else if (base == IP_EVENT) {
        if (id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
            g_ip = IPAddress(e->ip_info.ip.addr);
            g_connected = true;
        }
    }
}

bool WiFiClass::init() {
    if (g_initialized) return true;
    esp_err_t e;
    e = esp_netif_init();
    e = esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    e = esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, nullptr);
    esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, nullptr);
    g_initialized = (e == ESP_OK);
    return g_initialized;
}

void WiFiClass::mode(int m) {
    init();
    wifi_mode_t wm;
    switch (m) {
        case WIFI_STA: wm = WIFI_MODE_STA; break;
        case WIFI_AP:  wm = WIFI_MODE_AP;  break;
        case WIFI_AP_STA: wm = WIFI_MODE_APSTA; break;
        default:       wm = WIFI_MODE_NULL; break;
    }
    esp_wifi_set_mode(wm);
    if (wm == WIFI_MODE_NULL) {
        esp_wifi_stop();
        g_staRunning = g_apRunning = false;
    } else {
        esp_wifi_start();
    }
}

wl_status_t WiFiClass::begin(const char *ssid, const char *password) {
    init();
    esp_wifi_set_mode(WIFI_MODE_STA);
    wifi_config_t wc = {};
    if (ssid) strncpy((char *)wc.sta.ssid, ssid, sizeof(wc.sta.ssid) - 1);
    if (password) strncpy((char *)wc.sta.password, password, sizeof(wc.sta.password) - 1);
    wc.sta.threshold.authmode = WIFI_AUTH_OPEN;
    esp_wifi_set_config(WIFI_IF_STA, &wc);
    esp_wifi_start();
    g_staRunning = true;
    g_connected = false;
    esp_wifi_connect();
    return WL_DISCONNECTED;
}

wl_status_t WiFiClass::status() {
    if (!g_initialized) return WL_IDLE_STATUS;
    if (g_connected) return WL_CONNECTED;
    return WL_DISCONNECTED;
}

void WiFiClass::disconnect(bool wifioff) {
    if (wifioff) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        g_staRunning = g_apRunning = false;
    } else {
        esp_wifi_disconnect();
    }
    g_connected = false;
}

IPAddress WiFiClass::localIP() { return g_ip; }

bool WiFiClass::softAP(const char *ssid, const char *password) {
    init();
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t wc = {};
    if (ssid) strncpy((char *)wc.ap.ssid, ssid, sizeof(wc.ap.ssid) - 1);
    wc.ap.ssid_len = (uint8_t)strlen(ssid ? ssid : "");
    if (password && *password) {
        strncpy((char *)wc.ap.password, password, sizeof(wc.ap.password) - 1);
        wc.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wc.ap.authmode = WIFI_AUTH_OPEN;
    }
    wc.ap.max_connection = 4;
    esp_wifi_set_config(WIFI_IF_AP, &wc);
    esp_wifi_start();
    g_apRunning = true;
    return true;
}

IPAddress WiFiClass::softAPIP() {
    esp_netif_ip_info_t ip;
    esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap && esp_netif_get_ip_info(ap, &ip) == ESP_OK) {
        return IPAddress(ip.ip.addr);
    }
    return IPAddress(192, 168, 4, 1);
}

void WiFiClass::softAPdisconnect(bool wifioff) {
    if (wifioff) { esp_wifi_stop(); g_apRunning = g_staRunning = false; }
    else esp_wifi_stop();
}

int WiFiClass::scanNetworks() {
    init();
    if (g_apRunning) esp_wifi_set_mode(WIFI_MODE_APSTA);
    else esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    wifi_scan_config_t sc = {};
    esp_wifi_scan_start(&sc, true);
    uint16_t count = 0;
    if (esp_wifi_scan_get_ap_records(&count, g_aps) == ESP_OK) {
        g_scanCount = count;
    } else {
        g_scanCount = 0;
    }
    return g_scanCount;
}

String WiFiClass::SSID(int i) {
    if (i < 0 || i >= g_scanCount) return String();
    return String((const char *)g_aps[i].ssid);
}

int32_t WiFiClass::RSSI(int i) {
    if (i < 0 || i >= g_scanCount) return 0;
    return g_aps[i].rssi;
}