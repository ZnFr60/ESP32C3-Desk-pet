#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <time.h>
#include <Preferences.h>
#include <Config.h>

// ============================================================
// TimeManager - RTC-backed time keeping
//
// Power-on strategy:
//   1. Fresh power-on: RTC = 0, WiFi search 60s for NTP sync
//      If no WiFi → RTC starts from 0:00, increments normally
//   2. Daily midnight (00:00): WiFi recalibrate 60s, once per day
//   3. Deep sleep wake: RTC preserved, skip WiFi
// ============================================================

class TimeManager {
private:
    bool isSynced = false;
    unsigned long lastSyncAttempt = 0;
    int lastMidnightSyncDay = -1;  // Day-of-year of last midnight sync

    Preferences timePrefs;

    // Load persisted sync data
    void loadSyncState() {
        timePrefs.begin("time-state", false);
        lastMidnightSyncDay = timePrefs.getInt("syncDay", -1);
        timePrefs.end();
    }

    // Save sync state
    void saveSyncState() {
        timePrefs.begin("time-state", false);
        timePrefs.putInt("syncDay", lastMidnightSyncDay);
        timePrefs.end();
    }

public:
    void begin() {
        loadSyncState();

        // Check if RTC already has valid time (from previous NTP sync or deep sleep wake)
        time_t now = time(nullptr);
        if (now > 100000) {  // Valid time check (after 1970-01-02)
            isSynced = true;
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                Serial.printf("RTC time valid: %04d-%02d-%02d %02d:%02d:%02d\n",
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
            // Only set timezone, do NOT start SNTP (WiFi is off, would waste resources)
            setenv("TZ", "CST-8", 1);
            tzset();
        } else {
            Serial.println("RTC time not set (epoch ~0) - fresh power-on detected");
            // Set timezone config so NTP sync will work correctly
            configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
        }
    }

    // One-time NTP sync - called when WiFi is connected
    // Returns true if sync succeeded
    bool syncFromNTP() {
        if (lastSyncAttempt > 0 && (millis() - lastSyncAttempt < 5000)) {
            return isSynced;  // Don't spam NTP requests
        }
        lastSyncAttempt = millis();

        Serial.println("Syncing time from NTP...");
        configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);

        // Wait for NTP sync (non-blocking with timeout)
        unsigned long startTime = millis();
        while (millis() - startTime < NTP_SYNC_TIMEOUT_MS) {
            time_t now = time(nullptr);
            if (now > 100000) {
                isSynced = true;
                struct tm timeinfo;
                if (getLocalTime(&timeinfo)) {
                    Serial.printf("NTP sync success: %04d-%02d-%02d %02d:%02d:%02d\n",
                        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                    // Mark today as synced
                    lastMidnightSyncDay = timeinfo.tm_yday;
                    saveSyncState();
                }
                return true;
            }
            delay(100);
        }

        Serial.println("NTP sync timeout!");
        return false;
    }

    // Check if we already have valid time (from RTC or NTP)
    bool hasValidTime() {
        return (time(nullptr) > 100000);
    }

    // Check if we are in the midnight sync window (00:00 ~ 00:0N)
    // and haven't already synced today
    bool needsMidnightSync() {
        if (!hasValidTime()) return false;

        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) return false;

        int todayDay = timeinfo.tm_yday;
        int hour = timeinfo.tm_hour;
        int min = timeinfo.tm_min;

        // In midnight window (00:00 ~ 00:05) and not synced today
        if (hour == 0 && min < MIDNIGHT_SYNC_WINDOW_MIN && todayDay != lastMidnightSyncDay) {
            Serial.printf("Midnight sync needed: day=%d, lastSyncDay=%d\n", todayDay, lastMidnightSyncDay);
            return true;
        }
        return false;
    }

    // Mark today as midnight-synced (called after successful midnight sync)
    void markMidnightSynced() {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            lastMidnightSyncDay = timeinfo.tm_yday;
            saveSyncState();
            Serial.printf("Midnight sync marked for day %d\n", lastMidnightSyncDay);
        }
    }

    bool getIsSynced() { return hasValidTime(); }

    String getTimeString() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return "--:--";
        }
        char buffer[6];
        sprintf(buffer, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        return String(buffer);
    }

    String getDateString() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return "----/--/--";
        }
        char buffer[11];
        sprintf(buffer, "%04d/%02d/%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
        return String(buffer);
    }

    String getWeekdayString() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            return "";
        }
        const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        return String(weekdays[timeinfo.tm_wday]);
    }

    int getHour() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) return -1;
        return timeinfo.tm_hour;
    }

    int getMinute() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) return -1;
        return timeinfo.tm_min;
    }

    int getSecond() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) return -1;
        return timeinfo.tm_sec;
    }

    time_t getCurrentTime() {
        return time(nullptr);
    }

    // Get uptime in seconds (resets after deep sleep, use getCurrentTime() for absolute time)
    unsigned long getUptimeSeconds() {
        return millis() / 1000;
    }
};

#endif // TIME_MANAGER_H