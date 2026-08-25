#ifndef TRIAL_MANAGER_H
#define TRIAL_MANAGER_H

#include <Preferences.h>
#include <Arduino.h>

// ============================================================
// Trial Manager - 2-hour trial period with flash persistence
// ============================================================
// Tracks accumulated usage time in real-time using millis().
// Saves usage time to flash every 10 minutes to survive power cycles.
// When 2 hours (7200 seconds) total usage is reached, the device is
// permanently locked and displays "Locked, please make a decision".
// Lock state is also persisted to flash to prevent reset by power cycle.
//
// Flash storage (Preferences, namespace "trial-state"):
//   Key "usage_secs" → uint32_t (accumulated usage seconds)
//   Key "locked"     → uint8_t  (0 = trial active, 1 = locked)
//
// Notes:
//   - millis() overflow (~49.7 days) is handled correctly by unsigned
//     arithmetic since sessionStartMillis is reset every 600 seconds.
//   - Each saveToFlash() opens and closes the NVS namespace to avoid
//     handle leaks, since this is a long-lived global object.
//   - Deep sleep saves the exact current usage before powering down.
// ============================================================

class TrialManager {
private:
    unsigned long totalUsageSeconds;    // Total accumulated usage seconds
    unsigned long sessionStartMillis;   // millis() value at session start
    unsigned long lastSavedSeconds;     // Last usage value saved to flash
    bool locked;
    
    static const unsigned long TRIAL_LIMIT_SECONDS = 7200;   // 2 hours = 7200 seconds
    static const unsigned long FLASH_SAVE_INTERVAL = 600;    // Save to flash every 10 minutes
    static const char* NAMESPACE;
    
public:
    TrialManager() 
        : totalUsageSeconds(0), sessionStartMillis(0), lastSavedSeconds(0), locked(false) {
    }
    
    // Initialize: load state from flash, check if already locked
    void begin() {
        // Load state from flash (open/close NVS to avoid handle leak)
        {
            Preferences prefs;
            prefs.begin(NAMESPACE, false);
            totalUsageSeconds = prefs.getULong("usage_secs", 0);
            locked = (prefs.getUChar("locked", 0) == 1);
            prefs.end();
        }
        
        sessionStartMillis = millis();
        lastSavedSeconds = totalUsageSeconds;
        
        Serial.println("=== Trial Manager ===");
        Serial.printf("  Total usage: %lu seconds (%.1f minutes / %.1f hours)\n", 
            totalUsageSeconds, totalUsageSeconds / 60.0, totalUsageSeconds / 3600.0);
        Serial.printf("  Remaining: %lu seconds (%.1f minutes / %.1f hours)\n", 
            getRemainingSeconds(), getRemainingSeconds() / 60.0, getRemainingSeconds() / 3600.0);
        Serial.printf("  Locked: %s\n", locked ? "YES" : "NO");
        
        if (locked) {
            Serial.println("*** DEVICE IS LOCKED - TRIAL EXPIRED ***");
        }
    }
    
    // Call every loop iteration to update usage timer
    void update() {
        if (locked) return;
        
        unsigned long currentMillis = millis();
        // Unsigned arithmetic handles millis() overflow correctly
        // since sessionStartMillis is reset every 600 seconds
        unsigned long sessionElapsed = (currentMillis - sessionStartMillis) / 1000;
        totalUsageSeconds = lastSavedSeconds + sessionElapsed;
        
        // Save to flash every 10 minutes (600 seconds)
        if (totalUsageSeconds - lastSavedSeconds >= FLASH_SAVE_INTERVAL) {
            lastSavedSeconds = totalUsageSeconds;
            sessionStartMillis = currentMillis;
            saveToFlash();
        }
        
        // Check if trial has expired
        if (totalUsageSeconds >= TRIAL_LIMIT_SECONDS) {
            lockDevice();
        }
    }
    
    unsigned long getRemainingSeconds() {
        if (locked) return 0;
        if (totalUsageSeconds >= TRIAL_LIMIT_SECONDS) return 0;
        return TRIAL_LIMIT_SECONDS - totalUsageSeconds;
    }
    
    unsigned long getTotalUsageSeconds() {
        return totalUsageSeconds;
    }
    
    bool isLocked() {
        return locked;
    }
    
    // Save current state to flash (open/close NVS each time to avoid handle leak)
    void saveToFlash() {
        Preferences prefs;
        prefs.begin(NAMESPACE, false);
        prefs.putULong("usage_secs", totalUsageSeconds);
        prefs.putUChar("locked", locked ? 1 : 0);
        prefs.end();
        Serial.printf("Trial: saved to flash (usage=%lu sec, locked=%d)\n", 
            totalUsageSeconds, locked);
    }
    
    // Lock the device permanently
    void lockDevice() {
        locked = true;
        totalUsageSeconds = TRIAL_LIMIT_SECONDS;
        saveToFlash();
        Serial.println("========================================");
        Serial.println("*** TRIAL EXPIRED - DEVICE LOCKED ***");
        Serial.println("========================================");
    }
};

// Out-of-line definition for static const char* member
const char* TrialManager::NAMESPACE = "trial-state";

#endif // TRIAL_MANAGER_H