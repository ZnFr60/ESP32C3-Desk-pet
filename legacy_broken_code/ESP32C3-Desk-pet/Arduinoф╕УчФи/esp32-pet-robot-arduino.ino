#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_wifi.h>

#include <Config.h>
#include <RoboEyesManager.h>
#include <WiFiManager.h>
#include <TimeManager.h>

// ============================================================
// Global Objects
// ============================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;
RoboEyesManager roboEyesManager(display);
PetWiFiManager wifiManager;
TimeManager timeManager;

// ============================================================
// MPU6050 State
// ============================================================
bool mpuConnected = false;
float accelX = 0, accelY = 0, accelZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;

// Motion detection state
bool isTilted = false;
unsigned long lastMotionEventTime = 0;
unsigned long motionCooldown = DEFAULT_MOTION_COOLDOWN;

// Motion sample buffer for pattern analysis
#define MOTION_BUFFER_SIZE 25
float accelBuffer[MOTION_BUFFER_SIZE];
int bufferIndex = 0;
bool bufferFull = false;

// ============================================================
// Step Counter State
// ============================================================
int stepCount = 0;
bool stepActive = false;
unsigned long stepStartTime = 0;
unsigned long lastStepTime = 0;
float lastDynamicAccel = 0;

// ============================================================
// Tap Detection State
// ============================================================
// State machine: 0=IDLE, 1=TAP1_DETECTED, 2=COOLDOWN
int tapState = 0;
bool tapPeakDetected = false;
unsigned long lastTapTime = 0;
unsigned long tapCooldownEnd = 0;
#define TAP_MIN_SEPARATION_MS 100  // Minimum time between first and second tap (avoid same-tap false double)

// ============================================================
// Button State (BOOT button - active LOW)
// ============================================================
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastPressTime = 0;
unsigned long pressReleaseTime = 0;
unsigned long buttonPressStartTime = 0;
int pressCount = 0;
bool triplePressDetected = false;
bool isShuttingDown = false;

// ============================================================
// Timing
// ============================================================
unsigned long lastTimeUpdate = 0;
unsigned long lastMotionSampleTime = 0;
unsigned long lastMidnightCheckTime = 0;
unsigned long motionSampleInterval = 20;  // Calculated from gyro rate setting

// ============================================================
// Settings (loaded from flash via WiFiManager)
// ============================================================
PetSettings currentSettings;

// ============================================================
// System State
// ============================================================
bool wokeFromDeepSleep = false;
bool wifiDisconnectedForPower = false;

// ============================================================
// I2C Scanner
// ============================================================
void scanI2C() {
    byte error, address;
    int nDevices = 0;

    Serial.println("Scanning I2C bus...");
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("  Found device at 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            if (address == 0x3C || address == 0x3D) Serial.print(" (SSD1306 OLED)");
            else if (address == 0x68 || address == 0x69) Serial.print(" (MPU6050)");
            Serial.println();
            nDevices++;
        }
    }

    if (nDevices == 0) Serial.println("  No I2C devices found!");
    else Serial.printf("  Found %d device(s)\n", nDevices);
}

// ============================================================
// Apply Settings to All Subsystems
// ============================================================
void applySettings() {
    PetSettings& s = wifiManager.getSettings();
    currentSettings = s;

    // Calculate motion sample interval from gyro rate
    // e.g., 50 Hz → 20ms interval, 10 Hz → 100ms interval
    motionSampleInterval = (s.gyroRateHz > 0) ? (1000 / s.gyroRateHz) : 20;

    // Apply motion cooldown to both pattern analysis and motion interactions
    motionCooldown = s.motionCooldown;
    roboEyesManager.setMotionCooldown(s.motionCooldown);

    // Apply idle timeout to eye manager
    roboEyesManager.setIdleTimeout(s.idleTimeout);

    // Apply auto-dim setting
    roboEyesManager.setAutoDim(s.autoDim);

    // Apply eco mode
    roboEyesManager.setEcoMode(s.ecoMode);

    // CPU frequency scaling
    if (s.ecoMode) {
        setCpuFrequencyMhz(CPU_FREQ_ECO);  // 80 MHz for power saving
        Serial.printf("CPU frequency set to %d MHz (eco mode)\n", CPU_FREQ_ECO);
    } else {
        setCpuFrequencyMhz(CPU_FREQ_NORMAL);  // 160 MHz normal
        Serial.printf("CPU frequency set to %d MHz (normal)\n", CPU_FREQ_NORMAL);
    }

    Serial.printf("Settings applied: FPS=%d, Gyro=%dHz (interval=%lums), Cooldown=%dms, Idle=%lus, Dim=%d, Eco=%d\n",
        s.screenFPS, s.gyroRateHz, motionSampleInterval,
        s.motionCooldown, s.idleTimeout / 1000, s.autoDim, s.ecoMode);
}

// ============================================================
// Motion Pattern Analysis
// ============================================================
// Returns: 0=none, 1=drop(pain), 2=shake(dizzy), 3=flick(angry)
int analyzeMotionPattern() {
    if (!bufferFull) return 0;

    int third = MOTION_BUFFER_SIZE / 3;

    float firstThirdMax = 0, midThirdMax = 0, lastThirdMax = 0;
    float firstThirdAvg = 0, lastThirdAvg = 0;
    int directionChanges = 0;
    float prevSign = 0;
    int peakIndex = 0;
    float peakVal = 0;
    float totalRange = 0;
    float globalMin = 999;

    for (int i = 0; i < MOTION_BUFFER_SIZE; i++) {
        float val = abs(accelBuffer[i]);

        if (i < third) {
            firstThirdAvg += val;
            if (val > firstThirdMax) firstThirdMax = val;
        } else if (i < 2 * third) {
            if (val > midThirdMax) midThirdMax = val;
        } else {
            lastThirdAvg += val;
            if (val > lastThirdMax) lastThirdMax = val;
        }

        if (val > peakVal) {
            peakVal = val;
            peakIndex = i;
        }
        if (val < globalMin) globalMin = val;

        if (i > 0) {
            float diff = accelBuffer[i] - accelBuffer[i - 1];
            float sign = (diff > 0.5) ? 1.0 : (diff < -0.5) ? -1.0 : 0.0;
            if (sign != 0 && prevSign != 0 && sign != prevSign) {
                directionChanges++;
            }
            if (sign != 0) prevSign = sign;
        }
    }

    firstThirdAvg /= third;
    lastThirdAvg /= (MOTION_BUFFER_SIZE - 2 * third);
    totalRange = peakVal - globalMin;

    // Pattern: FLICK - sudden spike in first third, then decay
    if (peakVal > MOTION_FLICK_THRESHOLD && peakIndex < third) {
        if (lastThirdMax < peakVal * 0.5) {
            Serial.printf("Motion: FLICK (peak=%.1f at %d)\n", peakVal, peakIndex);
            return 3;
        }
    }

    // Pattern: DROP - quiet start, spike in middle, sudden stop
    if (peakVal > MOTION_DROP_THRESHOLD) {
        bool quietStart = (firstThirdAvg < MOTION_DROP_THRESHOLD * 0.4);
        bool peakNotEarly = (peakIndex >= third / 2);
        bool suddenStop = (lastThirdMax < peakVal * 0.3);

        if (quietStart && peakNotEarly && suddenStop) {
            Serial.printf("Motion: DROP (peak=%.1f at %d)\n", peakVal, peakIndex);
            return 1;
        }
    }

    // Pattern: SHAKE - many direction changes with high amplitude
    if (directionChanges >= 4 && totalRange > MOTION_SHAKE_THRESHOLD * 1.5) {
        bool sustainedActivity = (midThirdMax > MOTION_SHAKE_THRESHOLD * 0.8);
        if (sustainedActivity) {
            Serial.printf("Motion: SHAKE (changes=%d, range=%.1f)\n", directionChanges, totalRange);
            return 2;
        }
    }

    // Fallback: high acceleration without clear pattern
    if (peakVal > MOTION_DROP_THRESHOLD) {
        Serial.printf("Motion: DROP-fallback (peak=%.1f)\n", peakVal);
        return 1;
    }

    return 0;
}

// ============================================================
// Step Counter Algorithm
// ============================================================
// Detects walking/running steps based on acceleration peaks.
// Uses hysteresis: step starts when accel > STEP_THRESHOLD,
// step counted when accel drops back below threshold and
// duration > STEP_MIN_DURATION and cooldown elapsed.
void updateStepCounter(float dynamicAccel, unsigned long currentTime) {
    if (!stepActive && dynamicAccel > STEP_THRESHOLD) {
        // Step starts
        stepActive = true;
        stepStartTime = currentTime;
    } else if (stepActive && dynamicAccel < STEP_THRESHOLD * 0.5) {
        // Step ends - check if valid
        unsigned long stepDuration = currentTime - stepStartTime;

        if (stepDuration >= STEP_MIN_DURATION &&
            (currentTime - lastStepTime >= STEP_COOLDOWN)) {
            stepCount++;
            lastStepTime = currentTime;
            roboEyesManager.setStepCount(stepCount);
            Serial.printf("Step #%d detected (duration=%lums)\n", stepCount, stepDuration);
        }
        stepActive = false;
    }
}

// ============================================================
// Tap Detection Algorithm
// ============================================================
// State machine for single tap and double tap detection.
// State 0 (IDLE): waiting for tap
// State 1 (TAP1): first tap detected, waiting for double tap window
// State 2 (COOLDOWN): post-tap cooldown
//
// Single tap → onTap() (playful response)
// Double tap → onDoubleTap() (very happy, mood boost)
void updateTapDetection(float dynamicAccel, unsigned long currentTime) {
    // Don't detect taps during motion events (drop/shake/flick)
    if (currentTime < tapCooldownEnd) return;

    switch (tapState) {
        case 0:  // IDLE - waiting for first tap
            if (dynamicAccel > TAP_THRESHOLD && !tapPeakDetected) {
                tapPeakDetected = true;
                lastTapTime = currentTime;
                tapState = 1;
                Serial.println("Tap: first tap detected, waiting for double...");
            }
            break;

        case 1:  // TAP1 - first tap detected, waiting for double tap window
            // Reset peak detection when acceleration drops
            if (dynamicAccel < TAP_THRESHOLD * 0.3) {
                tapPeakDetected = false;
            }

            // Check for second tap within window (with minimum separation to avoid false double)
            if (dynamicAccel > TAP_THRESHOLD && !tapPeakDetected &&
                (currentTime - lastTapTime >= TAP_MIN_SEPARATION_MS) &&
                (currentTime - lastTapTime < DOUBLE_TAP_WINDOW)) {
                // Double tap confirmed!
                Serial.println("Tap: DOUBLE TAP detected!");
                roboEyesManager.onDoubleTap();
                tapState = 2;
                tapCooldownEnd = currentTime + TAP_COOLDOWN;
                tapPeakDetected = false;
            }
            // If window expired, it was a single tap
            else if (currentTime - lastTapTime >= DOUBLE_TAP_WINDOW) {
                Serial.println("Tap: single tap confirmed");
                roboEyesManager.onTap();
                tapState = 2;
                tapCooldownEnd = currentTime + TAP_COOLDOWN;
            }
            break;

        case 2:  // COOLDOWN
            if (currentTime >= tapCooldownEnd) {
                tapState = 0;
                tapPeakDetected = false;
            }
            break;
    }
}

// ============================================================
// Motion Detection (called at configured gyro rate)
// ============================================================
void sampleMotion() {
    if (!mpuConnected) return;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    accelX = a.acceleration.x;
    accelY = a.acceleration.y;
    accelZ = a.acceleration.z;
    gyroX = g.gyro.x;
    gyroY = g.gyro.y;
    gyroZ = g.gyro.z;

    // Calculate dynamic acceleration (remove gravity)
    float accelMagnitude = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
    float dynamicAccel = abs(accelMagnitude - 9.8);
    
    // Apply deadzone filter to ignore small movements (gyro drift)
    if (dynamicAccel < MOTION_DEADZONE) {
        dynamicAccel = 0;
    }

    // Store in ring buffer for pattern analysis
    accelBuffer[bufferIndex] = dynamicAccel;
    bufferIndex++;
    if (bufferIndex >= MOTION_BUFFER_SIZE) {
        bufferIndex = 0;
        bufferFull = true;
    }

    unsigned long currentTime = millis();

    // Update step counter (independent of pattern analysis)
    updateStepCounter(dynamicAccel, currentTime);

    // Update tap detection (only when no major motion events)
    if (currentTime - lastMotionEventTime > motionCooldown) {
        updateTapDetection(dynamicAccel, currentTime);
    }

    // Tilt detection (steady state) - LEFT/RIGHT tilt detection
    // Only trigger on initial tilt, reset when device returns upright
    // NOTE: tilt should NOT block tap detection, so it uses a separate timer (tiltEventTime)
    // 
    // MPU6050 axis mapping for this device (per hardware silkscreen):
    // - accelX increases → device tilts LEFT → eyes look LEFT
    // - accelX decreases → device tilts RIGHT → eyes look RIGHT
    static unsigned long tiltEventTime = 0;
    if (abs(accelX) > MOTION_TILT_THRESHOLD) {
        if (!isTilted) {
            isTilted = true;
            tiltEventTime = currentTime;
            if (accelX > MOTION_TILT_THRESHOLD) {
                // accelX positive (增大) → left tilt → eyes look left
                roboEyesManager.onTiltLeft();
                Serial.println("Tilt: LEFT detected -> eyes look left");
            } else {
                // accelX negative (减小) → right tilt → eyes look right
                roboEyesManager.onTiltRight();
                Serial.println("Tilt: RIGHT detected -> eyes look right");
            }
        }
    } else {
        // Device returned to upright position - reset tilt state
        if (isTilted) {
            isTilted = false;
            Serial.println("Tilt: device returned to upright position");
        }
    }

    // Check for major motion events (drop/shake/flick) with cooldown
    if (bufferFull && (currentTime - lastMotionEventTime > motionCooldown)) {
        int motionType = analyzeMotionPattern();

        if (motionType > 0) {
            lastMotionEventTime = currentTime;

            // Reset buffer after detection
            bufferFull = false;
            bufferIndex = 0;

            // Reset tap detection state (major motion disrupts taps)
            tapState = 0;
            tapPeakDetected = false;
            tapCooldownEnd = currentTime + TAP_COOLDOWN;

            switch (motionType) {
                case 1: roboEyesManager.onDrop(); break;   // Pain
                case 2: roboEyesManager.onShake(); break;  // Dizzy
                case 3: roboEyesManager.onFlick(); break;  // Angry
            }
        }
    }

    lastDynamicAccel = dynamicAccel;
}

// ============================================================
// Deep Sleep - Power Off
// ============================================================
void enterDeepSleep() {
    Serial.println("Entering deep sleep (power off)...");
    Serial.println("RTC will continue keeping time while sleeping.");

    // Clear display and show shutdown message briefly
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 24);
    display.print("Shutting down...");
    display.setCursor(15, 40);
    display.print("RTC stays alive");
    display.display();
    delay(800);

    // Clear display before sleep
    display.clearDisplay();
    display.display();

    // Turn off WiFi if still on
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // Configure BOOT button as wake source for ESP32-C3
    // NOTE: esp_deep_sleep_enable_gpio_wakeup expects a bitmask, not a GPIO number
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BOOT_BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);

    // Enter deep sleep - RTC keeps running, button wakes the device
    Serial.println("Going to sleep now. Press BOOT to wake.");
    Serial.flush();
    esp_deep_sleep_start();
    // Never reaches here
}

// ============================================================
// Button Handler (BOOT button - active LOW, non-blocking)
// ============================================================
// Single press → petting (抚摸) → mood-based expression
// Double press → custom action (user-defined) → special expression
// Triple press → enter AP mode (显示 AP MODE)
// Long press (3s) → shutdown (deep sleep)
void handleButton() {
    if (isShuttingDown) return;

    bool buttonState = digitalRead(BOOT_BUTTON_PIN);
    unsigned long currentTime = millis();

    // Debounce
    if (buttonState != lastButtonState) {
        lastDebounceTime = currentTime;
        lastButtonState = buttonState;  // Update immediately for debounce
        return;  // Skip this iteration during debounce
    }

    if ((currentTime - lastDebounceTime) < BUTTON_DEBOUNCE_MS) {
        return;  // Still in debounce period
    }

    // Detect button press (HIGH → LOW transition, active LOW)
    if (buttonState == LOW && buttonPressStartTime == 0) {
        buttonPressStartTime = currentTime;
        pressReleaseTime = 0;
        
        // Count presses for multi-press detection
        if (currentTime - lastPressTime < BUTTON_TRIPLE_PRESS_WINDOW) {
            pressCount++;
        } else {
            pressCount = 1;
        }
        lastPressTime = currentTime;
        
        Serial.printf("Button pressed (count=%d)\n", pressCount);

        // Triple press detected immediately (no need to wait for release)
        if (pressCount >= AP_TRIGGER_TRIPLE_PRESS) {
            triplePressDetected = true;
            pressCount = 0;
            pressReleaseTime = 0;
            buttonPressStartTime = 0;
            Serial.println("Triple press detected! Entering AP mode...");

            // Show AP MODE screen clearly
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(SSD1306_WHITE);
            // Center "AP" text
            display.setCursor(35, 8);
            display.print("AP");
            display.setTextSize(1);
            display.setCursor(30, 32);
            display.print("MODE");
            display.drawLine(10, 48, 118, 48, SSD1306_WHITE);
            display.setCursor(10, 52);
            display.print("PetRobot-Setup");
            display.display();
            delay(1000);

            wifiManager.enterAPMode();
            return;
        }
    }

    // Detect button release (LOW → HIGH transition)
    if (buttonState == HIGH && buttonPressStartTime > 0) {
        unsigned long pressDuration = currentTime - buttonPressStartTime;
        pressReleaseTime = currentTime;
        
        Serial.printf("Button released (duration=%lums, pressCount=%d)\n", pressDuration, pressCount);
        
        // Check for long press (3 seconds) on release
        if (pressDuration >= BUTTON_LONG_PRESS_MS) {
            isShuttingDown = true;
            Serial.println("Long press detected! Shutting down...");
            enterDeepSleep();
            // Never reaches here after deep sleep
        }
        
        buttonPressStartTime = 0;
    }

    // After release, wait for multi-press window to expire before confirming action
    if (pressReleaseTime > 0 && !triplePressDetected && buttonPressStartTime == 0) {
        if (currentTime - pressReleaseTime > BUTTON_TRIPLE_PRESS_WINDOW) {
            if (pressCount == 1) {
                // Single press = petting (抚摸)
                roboEyesManager.onButtonPress();
                Serial.println("Button: single press -> petting");
            } else if (pressCount == 2) {
                // Double press = custom action (user-defined)
                roboEyesManager.onDoublePress();
                Serial.println("Button: double press -> custom action");
            }
            pressCount = 0;
            pressReleaseTime = 0;
        }
    }
}

// ============================================================
// Perform NTP sync and then disconnect WiFi
// ============================================================
void syncNTPAndDisconnect() {
    Serial.println("Performing one-time NTP sync...");
    bool synced = timeManager.syncFromNTP();

    if (synced) {
        Serial.println("NTP sync successful! Time is now stored in RTC.");
    } else {
        Serial.println("NTP sync failed - will use whatever time is available.");
    }

    // Disconnect WiFi to save power - RTC keeps time from here
    Serial.println("Disconnecting WiFi to save power (RTC will keep time)...");
    wifiManager.disconnectWiFi();
    wifiDisconnectedForPower = true;
    Serial.println("WiFi turned off. Time will be maintained by RTC.");
}

// ============================================================
// MidNight Sync - daily time recalibration at 00:00
// ============================================================
void handleMidnightSync() {
    if (!timeManager.needsMidnightSync()) return;

    Serial.println("\n=== Midnight Sync: day ended, recalibrating time ===");

    bool connected = wifiManager.tryAutoConnect(WIFI_SEARCH_TIMEOUT_MS);

    if (connected) {
        bool synced = timeManager.syncFromNTP();
        if (synced) {
            Serial.println("Midnight sync: NTP sync successful!");
            timeManager.markMidnightSynced();
        }
        // Disconnect WiFi after sync
        wifiManager.disconnectWiFi();
        wifiDisconnectedForPower = true;
    } else {
        // No WiFi found within 60s → mark today as synced anyway
        // (to avoid retrying every 30s all night)
        Serial.println("Midnight sync: no WiFi, marking skipped for today.");
        timeManager.markMidnightSynced();
    }
}

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n========================================");
    Serial.println("  Pet Robot v1.0 - Low Power Edition");
    Serial.println("========================================\n");

    // Check if waking from deep sleep
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    wokeFromDeepSleep = (wakeupReason == ESP_SLEEP_WAKEUP_GPIO);
    if (wokeFromDeepSleep) {
        Serial.println("Waking up from deep sleep (RTC time should be valid)");
    }

    // Seed random
    randomSeed(esp_random() + millis());

    // Initialize I2C (SDA=3, SCL=4) at 400kHz fast mode
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(400000);
    Serial.printf("I2C initialized (SDA=%d, SCL=%d) @ 400kHz\n", I2C_SDA_PIN, I2C_SCL_PIN);

    scanI2C();

    // Initialize OLED
    Serial.println("Initializing OLED...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed!");
        for (;;);
    }
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    Serial.println("OLED ready!");

    // Initialize BOOT button (GPIO9, active LOW, internal pull-up)
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    Serial.printf("BOOT button initialized on pin %d\n", BOOT_BUTTON_PIN);

    // Initialize MPU6050
    Serial.println("Initializing MPU6050...");
    if (mpu.begin(0x68, &Wire)) {
        Serial.println("MPU6050 initialized!");
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        mpuConnected = true;
        
        // Calibrate gyroscope at startup (device should be stationary)
        Serial.println("Calibrating gyroscope (keep device still)...");
        float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;
        for (int i = 0; i < MOTION_CALIBRATION_SAMPLES; i++) {
            sensors_event_t a, g, temp;
            mpu.getEvent(&a, &g, &temp);
            gyroOffsetX += g.gyro.x;
            gyroOffsetY += g.gyro.y;
            gyroOffsetZ += g.gyro.z;
            delay(20);
        }
        gyroOffsetX /= MOTION_CALIBRATION_SAMPLES;
        gyroOffsetY /= MOTION_CALIBRATION_SAMPLES;
        gyroOffsetZ /= MOTION_CALIBRATION_SAMPLES;
        Serial.printf("Gyro calibration complete: offsets=(%.2f, %.2f, %.2f)\n", 
                      gyroOffsetX, gyroOffsetY, gyroOffsetZ);
    } else {
        Serial.println("MPU6050 not found - continuing without motion sensing");
        mpuConnected = false;
    }

    // Initialize motion buffer
    for (int i = 0; i < MOTION_BUFFER_SIZE; i++) {
        accelBuffer[i] = 0;
    }

    // Initialize TimeManager (check RTC)
    Serial.println("\nChecking RTC time...");
    timeManager.begin();

    // Load settings from flash (needed regardless of WiFi strategy)
    wifiManager.loadConfig();

    // ============================================================
    // WiFi Strategy (power-optimized):
    //   Fresh power-on: RTC=0, WiFi search 60s → NTP sync or RTC from 0:00
    //   Deep sleep wake: skip WiFi, RTC preserved
    //   RTC valid: skip WiFi entirely
    //   Daily midnight: WiFi recalibrate 60s, once per day (in loop)
    // ============================================================
    bool isFreshBoot = (wakeupReason == ESP_SLEEP_WAKEUP_UNDEFINED);

    if (isFreshBoot && !timeManager.hasValidTime()) {
        // Fresh power-on: RTC is 0:00, attempt WiFi for 60 seconds
        Serial.println("Fresh power-on detected. RTC starts from 0:00.");
        Serial.println("Searching for saved WiFi (60s timeout)...");

        // No splash screen - WiFi search in background, eyes start immediately
        bool connected = wifiManager.tryAutoConnect(WIFI_SEARCH_TIMEOUT_MS);

        if (connected) {
            syncNTPAndDisconnect();
        } else {
            Serial.println("No WiFi found. Starting from 00:00 (RTC only).");
            WiFi.mode(WIFI_OFF);
            wifiDisconnectedForPower = true;
        }
    } else if (wokeFromDeepSleep) {
        // Deep sleep wake: skip WiFi, RTC should be valid
        Serial.println("Waking from deep sleep - skipping WiFi for power saving.");
        WiFi.mode(WIFI_OFF);
        wifiDisconnectedForPower = true;
    } else if (timeManager.hasValidTime()) {
        // RTC has valid time - no WiFi needed!
        Serial.println("RTC time is valid - skipping WiFi for power saving.");
        WiFi.mode(WIFI_OFF);
        wifiDisconnectedForPower = true;
    } else {
        // Fallback: no RTC time but not fresh boot (shouldn't normally happen)
        Serial.println("RTC time not set - attempting WiFi...");
        bool connected = wifiManager.tryAutoConnect(WIFI_SEARCH_TIMEOUT_MS);
        if (connected) {
            syncNTPAndDisconnect();
        } else {
            WiFi.mode(WIFI_OFF);
            wifiDisconnectedForPower = true;
        }
    }

    // Initialize RoboEyes with configured FPS
    Serial.println("Initializing eye animation system...");
    PetSettings& s = wifiManager.getSettings();
    int initialFPS = s.ecoMode ? ECO_FPS : s.screenFPS;
    roboEyesManager.begin(SCREEN_WIDTH, SCREEN_HEIGHT, initialFPS);

    // Apply all settings to subsystems
    applySettings();

    // Show greeting if waking from deep sleep
    if (wokeFromDeepSleep) {
        Serial.println("Showing wakeup greeting...");
        roboEyesManager.skipBootPhase();  // Don't show 15s boot phase on wake
        roboEyesManager.onWakeup();
    }

    // ============================================================
    // System Ready
    // ============================================================
    Serial.println("\n=== System Ready ===");
    if (wifiManager.getIsAPMode()) {
        Serial.printf("AP Mode active. Connect to '%s' and visit http://%s\n", AP_SSID, AP_IP);
        Serial.println("Triple-press BOOT button anytime to re-enter AP mode.");
    } else if (timeManager.hasValidTime()) {
        Serial.printf("Time: %s %s\n",
            timeManager.getDateString().c_str(),
            timeManager.getTimeString().c_str());
        Serial.println("Running on RTC time (WiFi off for power saving).");
    } else {
        Serial.println("Running without time. Triple-press BOOT to configure WiFi.");
    }
    Serial.println("Single-press BOOT = petting (抚摸)");
    Serial.println("Double-press BOOT = custom action (自定义)");
    Serial.println("Triple-press BOOT = AP mode (AP MODE)");
    Serial.println("Long-press BOOT 3s = shutdown (RTC stays alive)");
    Serial.printf("Mood: %d/100, Steps: %d\n", roboEyesManager.getMood(), stepCount);
    Serial.println();
}

// ============================================================
// Main Loop
// ============================================================
void loop() {
    // ============================================================
    // Handle AP mode (web server for provisioning + settings)
    // ============================================================
    if (wifiManager.getIsAPMode()) {
        wifiManager.handleClient();

        // Still handle button in AP mode
        handleButton();

        // If just connected via AP provisioning, sync NTP then disconnect
        if (wifiManager.getIsConnected() && !wifiDisconnectedForPower) {
            Serial.println("WiFi connected via AP! Starting NTP sync...");
            syncNTPAndDisconnect();

            // Show connected message
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(5, 15);
            display.print("WiFi Connected!");
            display.setCursor(5, 30);
            display.print("Time synced via NTP");
            display.setCursor(5, 45);
            display.print("WiFi now OFF");
            display.display();
            delay(2000);
        }

        // If user skipped WiFi setup (not connected, not synced)
        if (!wifiManager.getIsAPMode() && !wifiManager.getIsConnected() && !wifiDisconnectedForPower) {
            Serial.println("WiFi setup skipped. Turning off WiFi for power saving.");
            WiFi.mode(WIFI_OFF);
            wifiDisconnectedForPower = true;
            display.clearDisplay();
            display.display();
        }

        // Reset triple press flag
        triplePressDetected = false;
        return;  // Don't run pet logic during AP provisioning
    }

    // ============================================================
    // Button handling
    // ============================================================
    handleButton();

    // If button triggered AP mode, return to handle it
    if (wifiManager.getIsAPMode()) return;

    // ============================================================
    // Motion sampling at configured gyro rate
    // ============================================================
    if (millis() - lastMotionSampleTime >= motionSampleInterval) {
        sampleMotion();
        lastMotionSampleTime = millis();
    }

    // ============================================================
    // Update time display from RTC (every second, no WiFi needed)
    // ============================================================
    if (millis() - lastTimeUpdate > 1000) {
        if (timeManager.hasValidTime()) {
            String timeStr = timeManager.getTimeString();
            roboEyesManager.setTimeDisplay(timeStr, true);
        }
        lastTimeUpdate = millis();
    }

    // ============================================================
    // Midnight sync check (every 30 seconds, once per day)
    // ============================================================
    if (millis() - lastMidnightCheckTime > 30000) {
        lastMidnightCheckTime = millis();
        handleMidnightSync();
    }

    // ============================================================
    // Update eye animation
    // ============================================================
    roboEyesManager.update();

    // ============================================================
    // Draw overlays (time + step count) when eyes were redrawn
    // ============================================================
    if (roboEyesManager.wasFrameDrawn()) {
        bool showTimeOverlay = timeManager.hasValidTime();
        roboEyesManager.drawOverlays(display, showTimeOverlay);
    }
}
