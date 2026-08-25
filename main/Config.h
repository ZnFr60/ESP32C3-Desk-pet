#pragma once
#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// Pin Configuration (ESP32-C3)
// ============================================================
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 4
#define BOOT_BUTTON_PIN 9  // ESP32-C3 BOOT button is usually on GPIO9
#define BATTERY_ADC_PIN 0   // ADC0 for battery voltage reading (if available)

// ============================================================
// Display Configuration
// ============================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// SSD1306 RESET pin (active-low). MUST be a real GPIO, not -1: the library only
// drives the hardware reset sequence when rstPin >= 0. With -1 the panel
// controller can stay latched after a cold flash -> totally black screen.
// Wire the OLED RST line to this GPIO (ESP32-C3 GPIO5 by default).
#define OLED_RESET_PIN 5
#define OLED_RESET OLED_RESET_PIN

#define SCREEN_ADDRESS 0x3C

// Display layout zones
#define TIME_ZONE_X 90
#define TIME_ZONE_Y 2
#define EYES_ZONE_Y 20  // Eyes start from y=20, leaving top 18px for time/info
#define EYES_ZONE_HEIGHT 44  // Available height for eyes (64 - 20)
#define INFO_ZONE_X 2
#define INFO_ZONE_Y 2

// ============================================================
// WiFi Configuration
// ============================================================
#define AP_SSID "PetRobot-Setup"
#define AP_PASSWORD ""  // No password for setup AP
#define AP_IP "192.168.4.1"
#define AP_TRIGGER_TRIPLE_PRESS 3  // Triple press to enter AP mode

// ============================================================
// NTP Configuration (one-time sync only, then WiFi disconnects)
// ============================================================
#define NTP_SERVER "pool.ntp.org"
#define NTP_GMT_OFFSET_SEC 28800  // UTC+8 (Beijing Time)
#define NTP_DAYLIGHT_OFFSET_SEC 0
#define NTP_SYNC_TIMEOUT_MS 15000  // Wait max 15s for NTP sync

// ============================================================
// WiFi Auto-Connect Strategy
// ============================================================
// Power-on (fresh boot): RTC starts at 0:00, WiFi search for 60s
// Midnight (00:00 daily): WiFi recalibrate, 60s timeout
// Deep sleep wake: skip WiFi, use RTC
#define WIFI_SEARCH_TIMEOUT_MS 60000   // Max WiFi scan duration (1 minute)
#define MIDNIGHT_SYNC_WINDOW_MIN 5     // Midnight sync window (00:00 ~ 00:05)

// ============================================================
// Default Power & Performance Settings
// ============================================================
#define DEFAULT_SCREEN_FPS 30        // Default screen refresh rate (lower = more power saving)
#define DEFAULT_GYRO_RATE_HZ 50       // Default gyro sampling rate (lower = more power saving)
#define DEFAULT_MOTION_COOLDOWN 1000  // Default cooldown between motion events (ms)
#define DEFAULT_IDLE_TIMEOUT 30000    // Default idle timeout before sleep (ms)
#define DEFAULT_AUTO_DIM true         // Auto-dim screen when idle
#define DEFAULT_ECO_MODE false        // Eco mode (aggressive power saving)

// FPS options for AP configuration (every 10 FPS)
#define FPS_OPTION_MIN 10
#define FPS_OPTION_MAX 50
#define FPS_OPTION_STEP 10

// Gyro rate options (Hz)
#define GYRO_RATE_10HZ   10
#define GYRO_RATE_20HZ   20
#define GYRO_RATE_50HZ   50
#define GYRO_RATE_100HZ  100

// Motion cooldown options (ms)
#define COOLDOWN_500MS   500
#define COOLDOWN_1000MS  1000
#define COOLDOWN_2000MS  2000
#define COOLDOWN_3000MS  3000

// Idle timeout options (seconds)
#define IDLE_TIMEOUT_15S   15000
#define IDLE_TIMEOUT_30S   30000
#define IDLE_TIMEOUT_60S   60000
#define IDLE_TIMEOUT_120S  120000

// ============================================================
// Power Saving
// ============================================================
#define DIM_LEVEL_NORMAL 255    // Normal OLED brightness (0-255)
#define DIM_LEVEL_DIM 80        // Dimmed brightness for idle
#define DIM_LEVEL_VERY_DIM 20   // Very dim for deep idle
#define DIM_AFTER_MS 10000      // Dim after 10s of no interaction
#define ECO_FPS 10              // FPS in eco mode
#define CPU_FREQ_ECO 80         // CPU frequency in eco mode (MHz)
#define CPU_FREQ_NORMAL 160     // CPU frequency in normal mode (MHz)

// ============================================================
// Button Timing
// ============================================================
#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_TRIPLE_PRESS_WINDOW 500  // 500ms window for triple press
#define BUTTON_LONG_PRESS_MS 3000       // 3 seconds for long press (shutdown)

// ============================================================
// Expression Durations (milliseconds)
// ============================================================
#define EXPR_DEFAULT_DURATION 0       // Stays until timeout
#define EXPR_HAPPY_DURATION 2000
#define EXPR_COMFORTABLE_DURATION 3000
#define EXPR_LOVE_DURATION 3000
#define EXPR_EXCITED_DURATION 2500
#define EXPR_BORED_DURATION 5000
#define EXPR_SURPRISED_DURATION 2000
#define EXPR_CONFUSED_DURATION 3000
#define EXPR_CONTENT_DURATION 4000
#define EXPR_ANGRY_DURATION 3000      // From being flicked
#define EXPR_PAIN_DURATION 2500       // From dropping
#define EXPR_DIZZY_DURATION 3000      // From shaking
#define EXPR_SCARED_DURATION 2000
#define EXPR_SCARE_DURATION 4000
#define EXPR_SLEEPY_DURATION 5000
#define EXPR_CURIOUS_DURATION 8000
#define EXPR_STRETCH_DURATION 2000
#define EXPR_BLINK_DURATION 150       // Quick blink
#define EXPR_WINK_DURATION 500        // Wink
#define EXPR_THINKING_DURATION 4000   // Thinking expression
#define EXPR_SAD_DURATION 3000        // Sad expression
#define EXPR_COOL_DURATION 3000       // Cool/sunglasses

// ============================================================
// Activity Durations (milliseconds) - "what is he doing"
// ============================================================
#define ACTIVITY_DRINKING_DURATION 5000       // Drinking water
#define ACTIVITY_READING_DURATION 8000        // Reading a book
#define ACTIVITY_EATING_DURATION 6000         // Eating food
#define ACTIVITY_LISTENING_DURATION 7000      // Listening to music
#define ACTIVITY_YAWNING_DURATION 3000        // Yawning
#define ACTIVITY_DANCING_DURATION 6000        // Dancing
#define ACTIVITY_DRAWING_DURATION 8000        // Drawing/painting
#define ACTIVITY_GAMING_DURATION 10000        // Playing games
#define ACTIVITY_EATING_ICE_CREAM_DURATION 5000  // Eating ice cream
#define ACTIVITY_TAKING_PHOTO_DURATION 3000   // Taking a photo
#define ACTIVITY_WRITING_DURATION 7000        // Writing

// ============================================================
// Activity Durations
// ============================================================
#define ACTIVITY_EXERCISING_DURATION 6000     // Exercising
#define ACTIVITY_BRUSHING_TEETH_DURATION 5000 // Brushing teeth
#define ACTIVITY_COOKING_DURATION 8000        // Cooking
#define ACTIVITY_BATHING_DURATION 6000        // Bathing
#define ACTIVITY_CALLING_DURATION 5000        // Calling
#define ACTIVITY_EATING_NOODLES_DURATION 6000 // Eating noodles
#define ACTIVITY_LOOKING_MIRROR_DURATION 5000 // Looking in mirror
#define ACTIVITY_STRETCHING_BODY_DURATION 4000 // Stretching body

// Activity display interval - how often activities appear when idle (ms)
#define ACTIVITY_MIN_IDLE_TIME 8000   // Min idle time before activity starts
#define ACTIVITY_CHANCE_PERCENT 30    // 30% chance to trigger an activity when idle

// Boot phase - first few seconds after power on
#define BOOT_PHASE_DURATION_MS 15000  // First 15 seconds after boot: only surprised/stretch

// ============================================================
// Motion Detection Thresholds
// ============================================================
#define MOTION_DROP_THRESHOLD 18.0      // Sudden acceleration > 18 m/s^2 (increased to reduce false triggers)
#define MOTION_SHAKE_THRESHOLD 12.0     // Rapid changes > 12 m/s^2 (increased)
#define MOTION_FLICK_THRESHOLD 25.0     // Very sudden spike > 25 m/s^2 (increased)
#define MOTION_TILT_THRESHOLD 5.0       // Tilt angle threshold (increased)
#define MOTION_SAMPLE_WINDOW 500        // 500ms window for motion analysis
#define MOTION_DEADZONE 0.5             // Deadzone for small movements (gyro drift filter)
#define MOTION_CALIBRATION_SAMPLES 50   // Number of samples for gyro calibration at startup

// ============================================================
// Step Counter
// ============================================================
#define STEP_THRESHOLD 12.0     // Minimum dynamic acceleration for a step
#define STEP_COOLDOWN 300       // Min ms between steps
#define STEP_MIN_DURATION 50   // Min step duration (filter noise)

// ============================================================
// Tap Detection
// ============================================================
// NOTE: TAP_THRESHOLD must be HIGHER than STEP_THRESHOLD to avoid
// false tap triggers while walking (step peaks are ~12 m/s²).
// A genuine finger tap on the desk/case produces a sharp spike > 15 m/s².
#define TAP_THRESHOLD 15.0      // Acceleration threshold for tap (must be > STEP_THRESHOLD)
#define TAP_COOLDOWN 400        // Cooldown between taps
#define DOUBLE_TAP_WINDOW 500   // Window for double tap detection

// ============================================================
// Mood System (happiness tracking)
// ============================================================
#define MOOD_MAX 100
#define MOOD_MIN 0
#define MOOD_INITIAL 70
#define MOOD_DECAY_INTERVAL 60000  // Mood decreases every 60s without interaction
#define MOOD_DECAY_AMOUNT 5        // How much mood decreases per interval
#define MOOD_INTERACTION_GAIN 10   // How much mood increases per interaction

#endif // CONFIG_H