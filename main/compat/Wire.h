#pragma once
// Wire.h - Arduino TwoWire over ESP-IDF native I2C driver (legacy driver API).
#include <stdint.h>
#include <stddef.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_BUFFER_LENGTH 256

class TwoWire {
public:
    TwoWire(int port = 0)
        : m_port(port), m_installed(false), m_sda(-1), m_scl(-1), m_freq(100000),
          m_txlen(0), m_rxlen(0), m_rxidx(0), m_addr(0) {}

    void begin(int sda = -1, int scl = -1, uint32_t freq = 100000) {
        if (sda >= 0) m_sda = sda;
        if (scl >= 0) m_scl = scl;
        if (freq) m_freq = freq;
        if (m_sda < 0 || m_scl < 0) return;
        if (!m_installed) {
            i2c_config_t conf = {};
            conf.mode = I2C_MODE_MASTER;
            conf.sda_io_num = m_sda;
            conf.scl_io_num = m_scl;
            conf.sda_pullup_en = GPIO_PULLUP_ENABLE; // enables internal pull-ups (helps OLED if no external pull-ups)
            conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
            conf.master.clk_speed = m_freq;
            esp_err_t e = i2c_param_config((i2c_port_t)m_port, &conf);
            if (e == ESP_OK) e = i2c_driver_install((i2c_port_t)m_port, I2C_MODE_MASTER, 0, 0, 0);
            m_installed = (e == ESP_OK);
        } else {
            setClock(m_freq);
        }
    }
    void end() {
        if (m_installed) { i2c_driver_delete((i2c_port_t)m_port); m_installed = false; }
    }
    bool isInstalled() const { return m_installed; }
    int sdaPin() const { return m_sda; }
    int sclPin() const { return m_scl; }
    void setClock(uint32_t freq) {
        m_freq = freq;
        if (m_installed) {
            i2c_config_t conf = {};
            conf.mode = I2C_MODE_MASTER;
            conf.sda_io_num = m_sda;
            conf.scl_io_num = m_scl;
            conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
            conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
            conf.master.clk_speed = m_freq;
            i2c_param_config((i2c_port_t)m_port, &conf);
        }
    }
    void beginTransmission(uint8_t addr) { m_addr = addr; m_txlen = 0; }
    uint8_t endTransmission(bool stop = true) {
        (void)stop;
        if (!m_installed) return 4;
        esp_err_t e;
        if (m_txlen == 0) {
            // Empty probe: the legacy i2c_master_write_to_device rejects a NULL
            // zero-length buffer ("i2c null address error"). Send one 0x00 byte
            // instead so the address ACK is actually checked (used by scanI2C).
            uint8_t probe = 0x00;
            e = i2c_master_write_to_device((i2c_port_t)m_port, m_addr, &probe, 1, pdMS_TO_TICKS(50));
        } else {
            e = i2c_master_write_to_device((i2c_port_t)m_port, m_addr, m_txbuf, m_txlen, pdMS_TO_TICKS(50));
            m_txlen = 0;
        }
        return (e == ESP_OK) ? 0 : 2;
    }
    size_t write(uint8_t b) {
        if (m_txlen < I2C_BUFFER_LENGTH) { m_txbuf[m_txlen++] = b; return 1; }
        return 0;
    }
    size_t write(const uint8_t *data, size_t len) {
        size_t n = 0;
        while (n < len && m_txlen < I2C_BUFFER_LENGTH) m_txbuf[m_txlen++] = data[n++];
        return n;
    }
    size_t requestFrom(uint8_t addr, uint8_t len, uint8_t stop = true) {
        (void)stop;
        m_rxlen = 0; m_rxidx = 0;
        if (!m_installed) return 0;
        esp_err_t e = i2c_master_read_from_device((i2c_port_t)m_port, addr, m_rxbuf, len, pdMS_TO_TICKS(50));
        if (e != ESP_OK) return 0;
        m_rxlen = len;
        return m_rxlen;
    }
    size_t requestFrom(uint8_t addr, size_t len) { return requestFrom(addr, (uint8_t)len); }
    uint8_t read() { return (m_rxidx < m_rxlen) ? m_rxbuf[m_rxidx++] : 0xff; }
    int available() { return (int)(m_rxlen - m_rxidx); }
    void setWireTimeout(uint32_t us = 25000, bool reset = true) { (void)us; (void)reset; }

private:
    int m_port;
    bool m_installed;
    int m_sda, m_scl;
    uint32_t m_freq;
    uint8_t m_txbuf[I2C_BUFFER_LENGTH];
    uint8_t m_rxbuf[I2C_BUFFER_LENGTH];
    size_t m_txlen, m_rxlen, m_rxidx;
    uint8_t m_addr;
};

extern TwoWire Wire;