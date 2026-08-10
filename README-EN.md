## About Me

Hi! I'm a 16-year-old high school student from China. I'm passionate about embedded systems and hardware development. This is my open-source project — welcome to star or fork!

## About This Project

Pet Robot is an ESP32-C3-based desktop electronic pet. It displays expressive large eyes on an OLED screen, features an MPU6050 gyroscope for motion sensing, and offers 23 expressions, 19 activity animations, a mood system, and multi-layer power-saving strategies. It supports WiFi time sync and web-based configuration.

## Project Structure

```
ESP32C3-Desk-pet/
├── README.md                    # Project documentation (Chinese)
├── README-EN.md                 # Project documentation (English)
├── pet-robot-manual.html        # HTML user manual (Chinese)
├── pet-robot-manual-EN.html     # HTML user manual (English)
├── LICENSE                      # GPL v3 open source license
├── release/                     # Firmware releases
│   └── firmware.bin
├── ESP32C3-Desk-pet/            # Standard edition (no trial restriction)
│   ├── Arduino专用/
│   ├── 标准源码/
│   └── 固件/
└── ESP32C3-Desk-pet-Trial/      # Trial edition (with trial restriction)
    ├── Arduino专用/
    ├── 标准源码/
    └── 固件/
```

## Important Notes

> **⚠️ Testing Status**: This project has only undergone logic-level software testing so far and has not been fully verified or hardware-debugged on real hardware. If you encounter issues during flashing or operation, feel free to submit an Issue. Developers with hardware experience are welcome to assist with testing and improvements.

> **💡 Feedback & Remixes Welcome**: If you find bugs, have improvement suggestions, or have created derivative works based on this project, feel free to share your results via Issue or Pull Request. This is an open-source project — looking forward to your participation!

## Open Source License

This project is open-sourced under the **GNU General Public License v3 (GPL v3)**. Since this project depends on GPL-licensed open source libraries, under the GPL's "copyleft" clause, derivative works must also be released under the GPL license.

This means:
- You are free to use, modify, and distribute the code
- Any modified versions based on this project must also be open-sourced under GPL v3
- You must retain the original author's copyright notice and provide complete source code when distributing
- If you plan to make substantial modifications or re-release, it is recommended to communicate with me via Issue first

---

## Version Info

| Version | Folder | Description |
|------|--------|------|
| Standard | `ESP32C3-Desk-pet/` | Full features, no trial restriction |
| Trial | `ESP32C3-Desk-pet-Trial/` | Includes `TrialManager.h`, with trial restriction mechanism |

---

# Pet Robot — ESP32-C3 Desktop Electronic Pet

> **Firmware Version 1.0 · Low Power Edition**  
> Based on ESP32-C3 + SSD1306 OLED + MPU6050 Desktop Electronic Pet

---

## Table of Contents

1. [Product Overview](#product-overview)
2. [Hardware Specifications](#hardware-specifications)
3. [Wiring Guide](#wiring-guide)
4. [Getting Started & Flashing](#getting-started--flashing)
5. [WiFi & Time Sync](#wifi--time-sync)
6. [Interaction Guide](#interaction-guide)
7. [AP Configuration Mode](#ap-configuration-mode)
8. [Power Management](#power-management)
9. [Code Architecture](#code-architecture)
10. [Feature Reference](#feature-reference)
11. [Expression & Activity Catalog](#expression--activity-catalog)
12. [Troubleshooting](#troubleshooting)

---

## Product Overview

Pet Robot is a desktop electronic pet based on the **ESP32-C3** microcontroller. It displays a pair of expressive large eyes on a **SSD1306 OLED (128x64)** screen, uses an **MPU6050 6-axis gyroscope** for motion sensing, and can perform 23 expressions and 19 daily activity animations. It is a desktop companion that combines fun with low-power operation.

### Core Features

| Feature | Description |
|------|------|
| **23 Dynamic Expressions** | Happy, Surprised, Confused, Sleepy, Angry, Pain, Dizzy, Curious, Looking Around, Thinking, Sad, Cool, etc. |
| **19 Daily Activities** | Drinking, Reading, Eating, Dancing, Drawing, Gaming, Ice cream, Photo, Writing, Workout, Brushing, Cooking, Bathing, Calling, Noodles, Mirror, Stretching, etc. |
| **RTC Timekeeping** | After one WiFi NTP sync, WiFi disconnects while internal RTC keeps time, preserved through deep sleep |
| **Smart Power Saving** | Auto screen dimming, dynamic CPU frequency scaling, WiFi disconnect after use, auto deep sleep when idle |
| **Motion Sensing** | Step counter, tap detection, tilt detection, drop/shake/flick action recognition |
| **Mood System** | 100-point scale, decays without interaction, increases with interaction, affects expression tendencies |

---

## Hardware Specifications

### Core Components

| Component | Model | Specification | Purchase Reference |
|------|------|------|----------|
| MCU | ESP32-C3 | RISC-V 32-bit, 160 MHz, 400 KB SRAM, 4 MB Flash | ESP32-C3 Dev Board (SuperMini / Xiao) |
| Display | SSD1306 OLED | 128x64 pixels, I2C address 0x3C, monochrome | 0.96" OLED Blue/White |
| Gyroscope | MPU6050 | 3-axis accelerometer + 3-axis gyroscope, I2C address 0x68 | GY-521 Module |
| Button | BOOT Button | GPIO9, built-in pull-up, active low | Built-in on ESP32-C3 dev board |

### Electrical Characteristics

| Parameter | Value | Notes |
|------|------|------|
| Supply Voltage | 3.3V | Powered via USB or battery |
| I2C Bus | 400 kHz Fast Mode | GPIO3=SDA, GPIO4=SCL |
| Normal Operating Current | ~40-60 mA | 160 MHz CPU + full brightness |
| Power Saving Current | ~20-30 mA | 80 MHz CPU + low brightness |
| Deep Sleep Current | ~5 uA | RTC keeps running |
| Flash Usage | ~865 KB / 1.25 MB (66%) | Including all libraries |
| RAM Usage | ~40 KB / 320 KB (12.4%) | At runtime |

### Pin Assignment

| ESP32-C3 Pin | Function | Connected To | Description |
|---------------|------|--------|------|
| GPIO3 | I2C SDA | SSD1306 SDA + MPU6050 SDA | Data Line, shared by both devices |
| GPIO4 | I2C SCL | SSD1306 SCL + MPU6050 SCL | Clock Line, shared by both devices |
| GPIO9 | BOOT Button | Onboard button (active low) | Built-in on dev board |
| 3.3V | Power Positive | SSD1306 VCC + MPU6050 VCC | **Must use 3.3V, do NOT use 5V** |
| GND | Power Negative | SSD1306 GND + MPU6050 GND | Common Ground |

---

## Wiring Guide

### Shared I2C Bus

SSD1306 and MPU6050 share the same I2C bus (GPIO3=SDA, GPIO4=SCL). The two devices have different I2C addresses (0x3C and 0x68), so there is no conflict. Only **4 data wires** are needed for all connections.

### Wiring Table

| ESP32-C3 | SSD1306 OLED | MPU6050 | Wire Color | Description |
|----------|-------------|---------|----------|------|
| 3.3V | VCC | VCC | Red | Power Positive (**Do NOT connect to 5V**) |
| GND | GND | GND | Black | Power Negative / Common Ground |
| GPIO3 | SDA | SDA | Yellow | I2C Data Line |
| GPIO4 | SCL | SCL | Green | I2C Clock Line |
| -- | -- | AD0 -> GND | -- | Set I2C address to 0x68 |

### Wiring Diagram

```
ESP32-C3              SSD1306 OLED          MPU6050
┌─────────┐           ┌──────────┐          ┌──────────┐
│  3.3V   ├───────────┤   VCC    │──────────┤   VCC    │
│   GND   ├───────────┤   GND    │──────────┤   GND    │
│  GPIO3  ├───────────┤   SDA    │──────────┤   SDA    │
│  GPIO4  ├───────────┤   SCL    │──────────┤   SCL    │
│  GPIO9  │(onboard   │          │  AD0 ────┤   GND    │
│         │ button)    │          │          │          │
└─────────┘           └──────────┘          └──────────┘
```

### Physical Wiring Steps

1. Place the ESP32-C3, SSD1306 OLED, and MPU6050 (GY-521) modules on the desk.
2. Use Dupont wires to connect the ESP32-C3 **3.3V** pin to the SSD1306 **VCC** and MPU6050 **VCC** (can be wired in parallel, i.e., one wire from ESP32 3.3V splits to both modules' VCC).
3. Use Dupont wires to connect the ESP32-C3 **GND** pin to the SSD1306 **GND** and MPU6050 **GND**.
4. Use Dupont wires to connect the ESP32-C3 **GPIO3** to the SSD1306 **SDA** and MPU6050 **SDA**.
5. Use Dupont wires to connect the ESP32-C3 **GPIO4** to the SSD1306 **SCL** and MPU6050 **SCL**.
6. Connect the MPU6050 **AD0** pin to **GND** (sets I2C address to 0x68).
7. Check all connections are secure and ensure no short circuits.
8. Power the ESP32-C3 via USB Type-C cable.

### MPU6050 Orientation Notes

The MPU6050 module (GY-521) has chip silkscreen markings indicating the X/Y axis directions. The tilt detection mapping in the code is:

- **accelX increases** (positive direction) -> device tilts left -> pet eyes look **left**
- **accelX decreases** (negative direction) -> device tilts right -> pet eyes look **right**
- Tilt threshold: > 5 m/s^2 triggers, auto-resets when returning to upright

### Wiring Precautions

> **⚠️ Voltage Warning**: SSD1306 and MPU6050 are both 3.3V devices. **Do NOT connect to 5V**, or the modules may be damaged.

- Some SSD1306 modules may have an I2C address of 0x3D. If the display is abnormal, check the resistor soldering position on the back of the module (typically 0x3C corresponds to R1 soldered, 0x3D corresponds to R2 soldered).
- The MPU6050 AD0 pin **must be grounded** (GND) for the address to be 0x68. If AD0 is floating or connected to 3.3V, the address becomes 0x69 and the program will not find the device.
- Most SSD1306 and MPU6050 modules already have built-in 4.7kohm I2C pull-up resistors; additional ones are generally not needed.
- If using longer Dupont wires (>20cm), I2C communication may be unstable. It is recommended to shorten the wire length or reduce the I2C clock frequency.
- It is recommended to use different colored Dupont wires to distinguish VCC/GND/SDA/SCL for easier troubleshooting.

### Breadboard vs Soldering

| Method | Pros | Cons | Use Case |
|------|------|------|----------|
| Breadboard | No soldering needed, quick setup, adjustable | Risk of poor contact, bulky | Prototyping, learning & debugging |
| Soldering (perfboard) | Reliable connections, compact, suitable for enclosure | Irreversible, requires soldering skill | Final product, long-term use |
| Custom PCB | Most reliable, smallest size, mass-producible | High cost, long lead time | Mass production |

---

## Getting Started & Flashing

### Environment Setup

This firmware is developed based on the **Arduino framework**. It is recommended to use Arduino IDE or PlatformIO for compilation and flashing.

### Method 1: Arduino IDE

1. Install **ESP32 board support**: In Arduino IDE, go to "Preferences" -> "Additional Board Manager URLs" and add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. In "Tools" -> "Board" -> "Board Manager", search for **ESP32** and install.
3. Install the required libraries (via "Tools" -> "Manage Libraries"):
   - `Adafruit SSD1306`
   - `Adafruit MPU6050`
   - `Adafruit GFX`
   - `Adafruit Unified Sensor`
   - `Adafruit BusIO`
   - `ArduinoJson`
4. Open `esp32-pet-robot-arduino.ino`.
5. Select board: **"ESP32C3 Dev Module"**.
6. Configuration: Flash Size = 4MB, Partition Scheme = Default, USB CDC On Boot = Enabled.
7. Click the "Upload" button to flash the firmware.

### Method 2: PlatformIO (Recommended)

```bash
# Build
pio run

# Build and flash
pio run --target upload

# Monitor serial output
pio device monitor --baud 115200
```

### Boot Sequence

| Step | Stage | Description |
|------|------|------|
| 1 | Initialize I2C | Scan I2C bus, detect SSD1306 (0x3C) and MPU6050 (0x68) |
| 2 | Gyroscope Calibration | Collect 50 samples, calculate zero offset, **keep the device stationary** |
| 3 | Check RTC | First power-on: RTC = 0:00; Deep sleep wake: RTC valid, skip WiFi |
| 4 | WiFi Search | First power-on: search WiFi for 60s; found -> NTP sync; not found -> start from 0:00 |
| 5 | Startup Phase | 15-second startup phase, Surprised and Stretching each at 50% probability |
| 6 | Normal Operation | Normal expression cycle, Looking Around 60%, other expressions 40% |

> **First Power-On vs Deep Sleep Wake**: On first power-on (reconnecting power), RTC starts from 0:00 and will attempt WiFi search for 60 seconds. If previously configured, it will auto-connect using saved WiFi credentials. When waking from deep sleep, RTC retains the previous time, skipping WiFi and the startup phase, going directly into normal interaction.

---

## WiFi & Time Sync

### Time Sync Strategy

Pet Robot uses an **RTC-first** power-saving time strategy. WiFi is only briefly connected when necessary:

| Scenario | Behavior | WiFi Search Duration |
|------|------|---------------|
| First power-on (RTC = 0:00) | Search saved WiFi -> NTP sync -> disconnect WiFi | Up to 60 seconds |
| Deep sleep wake | Skip WiFi, RTC keeps time | 0 seconds |
| Daily midnight (00:00~00:05) | Connect WiFi to calibrate time, once per day | Up to 60 seconds |
| Triple-click to enter AP mode | Enable AP hotspot + Web configuration interface | Until user action |

### WiFi Search Timeout Flow

1. **Start Search**: Use saved SSID + password
2. **Wait for Connection**: Check every 500ms, up to 60 seconds
3. **Connected**: NTP sync time -> disconnect WiFi immediately
4. **Timeout**: Turn off WiFi, RTC continues running (starting from 0:00)

> **⚠️ When No Saved WiFi**: If never configured, the device will not search for WiFi on power-up and will run directly with RTC starting from 0:00. Triple-click the button to enter AP mode for configuration; credentials will then be saved for future automatic connections.

### NTP Server

Default uses `pool.ntp.org`, timezone is UTC+8 (Beijing Time). NTP sync timeout is 15 seconds. After successful sync, WiFi is immediately disconnected to save power.

---

## Interaction Guide

### Quick Reference

| Action | Function |
|------|------|
| Single-click | Pet the pet |
| Double-click | Random action |
| Triple-click | AP configuration mode |
| Long press 3s | Deep sleep |
| GPIO3 | I2C SDA |
| GPIO4 | I2C SCL |
| GPIO9 | BOOT Button |
| 0x3C | OLED Address |
| 0x68 | MPU6050 Address |
| 192.168.4.1 | AP Configuration Page |

### Button Operations

| Action | Trigger Condition | Pet Reaction |
|------|----------|----------|
| **Single-click (Pet)** | Press < 500ms, once | Mood > 70 -> Comfortable; Mood > 40 -> Happy; else -> Sad |
| **Double-click (Custom Action)** | 2 presses within 500ms | Random trigger: Heart / Cool / Excited / Wink / Dance / Gaming / Ice cream / Photo / Listening / Drawing / Noodles / Mirror |
| **Triple-click (AP Mode)** | 3 presses within 500ms | Screen shows "AP MODE" -> Enter Web configuration mode |
| **Long press (Shutdown)** | Hold for 3+ seconds | Enter deep sleep (RTC keeps running, press BOOT to wake) |

> Button uses non-blocking detection, determining single-click, double-click, or long press only after button release. Triple-click triggers immediately on the third press. Button debounce time is 50ms.

### Motion Sensing

| Action | Detection Threshold | Pet Reaction |
|------|----------|----------|
| Single tap | Acceleration > 15 m/s^2 | 50% Surprised / 50% Wink |
| Double tap | Two taps within 500ms | Excited (Mood +15) |
| Drop | Acceleration > 18 m/s^2 + pattern matching | Pain expression -> Scared expression |
| Shake | Direction change >= 4 times + high amplitude | Dizzy expression -> Confused expression |
| Flick | Acceleration > 25 m/s^2 spike | Angry expression (Mood -10) |
| Tilt left | accelX increases > 5 m/s^2 | Eyes look left |
| Tilt right | accelX decreases < -5 m/s^2 | Eyes look right |
| Walk/Run | Dynamic acceleration > 12 m/s^2 | Step counter +1 |

### Mood System

| Event | Mood Change | Description |
|------|----------|------|
| No interaction | -5 / 60s | Mood continuously decays, minimum 0 |
| Button click / Tap / Tilt | +10 | Interaction increases mood |
| Double-click (button or tap) | +15 | Double-click is especially joyful |
| Flick | -10 | Being flicked is upsetting |
| Initial value | 70 | Boot-up mood |
| Range | 0 ~ 100 | Will not go below 0 or above 100 |

### Action Cooldown Mechanism

To prevent gyroscope noise from causing frequent expression switching, all motion-triggered events (tilt, drop, shake, flick, tap) have a **5-second cooldown** (configurable via AP mode to 0.5s ~ 3s). On startup, the gyroscope undergoes 50-sample calibration to eliminate zero offset, with a 0.5 m/s^2 deadband filter.

---

## AP Configuration Mode

### Entering AP Mode

**Triple-click the BOOT button** (3 presses within 500ms) to enter AP configuration mode. The screen will display large "AP" and "MODE" text, along with the "PetRobot-Setup" hotspot name.

The ESP32-C3 will create an open WiFi hotspot named `PetRobot-Setup` (no password).

### Configuration Process

1. **Triple-click button**: Screen shows AP MODE, ESP32-C3 creates hotspot
2. **Connect WiFi**: Phone/computer connects to `PetRobot-Setup`
3. **Open Browser**: Visit `http://192.168.4.1` to enter the configuration page
4. **Configuration Complete**: Select WiFi and enter password, or skip directly. WiFi auto-disconnects to save power

### Configurable Parameters

| Parameter | Options | Default | Description |
|------|------|--------|------|
| Screen Refresh Rate | 10 / 20 / 30 / 40 / 50 FPS | 30 FPS | Lower = more power saving, higher = smoother |
| Gyroscope Sample Rate | 10 / 20 / 50 / 100 Hz | 50 Hz | Lower = more power saving, slower response |
| Action Cooldown | 0.5s / 1s / 2s / 3s | 1s | Minimum interval between action events |
| Idle Timeout | 15s / 30s / 60s / 120s | 30s | No interaction before entering sleepy/sleep |
| Auto Dim | On / Off | On | Reduce screen brightness when idle |
| Power Saving Mode | On / Off | Off | CPU downclock to 80 MHz, forced low brightness |

> All settings are saved in Flash (via ESP32 Preferences library) and persist across power loss.

---

## Power Management

### Multi-Layer Power Saving Strategy

| Layer | Trigger Condition | Measure |
|------|----------|------|
| 1. WiFi Disconnect | After NTP sync completes | Immediately turn off WiFi module |
| 2. Screen Dim | No interaction for 10s | Brightness reduced to 80/255 |
| 3. Screen Very Dim | No interaction for 20s | Brightness reduced to 20/255 |
| 4. Power Saving Mode | User manually enables | CPU 80 MHz + forced dim brightness |
| 5. Sleepy | Idle timeout reached | 5-second eye-close animation -> Enter sleep |
| 6. Sleep | Sleepy animation ends | Eyes closed, brightness stays at DIM level |
| 7. Deep Sleep | Long press button 3s | ESP32 deep sleep, RTC maintained, BOOT to wake |

### Deep Sleep Details

Long press BOOT button for 3 seconds to enter deep sleep. At this point:

- ESP32-C3 enters deep sleep state, power consumption drops to approximately **5 uA**
- Internal RTC continues running, time is not lost
- Press BOOT button to wake, shows surprised expression then returns to normal
- After waking, skips the 15-second startup phase and goes directly into normal interaction

> **Battery Power Recommendations**: It is recommended to use a 200-400 mAh small lithium battery. In power saving mode + 30 FPS configuration, estimated battery life is about 8-12 hours. Deep sleep mode can standby for several days. To extend battery life, reduce FPS to 10 and enable power saving mode.

---

## Code Architecture

### File Structure

| File | Size | Responsibility |
|------|------|------|
| `Config.h` | ~200 lines | Global configuration: pin definitions, thresholds, durations, power saving parameters |
| `esp32-pet-robot-arduino.ino` | ~800 lines | Main program: initialization, sensor reading, button handling, main loop |
| `RoboEyesManager.h` | ~1200 lines | Expression manager: state machine, 23 expressions, 19 activities, brightness control, mood system |
| `WiFiManager.h` | ~400 lines | WiFi manager: AP mode, Web configuration interface, settings persistence, auto-connect |
| `TimeManager.h` | ~190 lines | Time manager: NTP sync, RTC maintenance, midnight detection, daily calibration |
| `FluxGarage_RoboEyes.h` | ~1500 lines | Eye animation library: eye geometry calculations, animation interpolation, blinking, curiosity mode, shaking |

### Data Flow

```
┌──────────────┐    ┌──────────────────┐
│   MPU6050    │───▶│  main.cpp        │
│  Accel+Gyro  │    │  sampleMotion()  │
└──────────────┘    │  · Step counter  │
                    │  · Tap detection │
┌──────────────┐    │  · Action analysis│
│  BOOT Button │───▶│  · Tilt detection│
│  GPIO9       │    └────────┬─────────┘
└──────────────┘             │
                    ┌────────▼─────────┐    ┌──────────────────┐
┌──────────────┐    │ RoboEyesManager  │───▶│ SSD1306 OLED     │
│  TimeManager │───▶│ · State machine  │    │ 128x64 Display   │
│  RTC + NTP   │    │ · Expression anim│    │ · Eye animation  │
└──────────────┘    │ · Activity anim  │    │ · Time overlay   │
                    │ · Brightness ctrl│    │ · Step overlay   │
┌──────────────┐    │ · Mood system    │    │ · Activity label │
│  WiFiManager │    └──────────────────┘    └──────────────────┘
│  AP + Web    │
└──────────────┘
```

### State Machine

- **Default** -- Default state, randomly switches expressions or activities when idle
- **Startup Phase** -- First 15 seconds, only switches between Surprised and Stretching (50% each)
- **Normal Phase** -- Looking Around 60%, other 10 expressions each 4%
- **Interaction Trigger** -- Button / Tap / Tilt / Action -> Corresponding expression (5s cooldown)
- **Idle Timeout** -- Reaches idleTimeout -> Sleepy -> Sleep
- **Sleep Wake** -- Any interaction -> Surprised expression -> Normal state

### Build Environment

| Item | Configuration |
|------|------|
| Framework | Arduino (ESP32-C3) |
| Build Tool | Arduino IDE / PlatformIO |
| Board | ESP32C3 Dev Module |
| Dependencies | Adafruit SSD1306, Adafruit MPU6050, Adafruit GFX, Adafruit Unified Sensor, Adafruit BusIO, ArduinoJson, WiFi, Wire, Preferences, WebServer, ESP Sleep |

---

## Feature Reference

### Screen Overlay Priority

| Priority | Content | Display Condition |
|--------|------|----------|
| 1 (Highest) | Activity Label | Displayed during active activities (English labels, e.g., "Drinking...") |
| 2 | Time | Displayed when RTC time is valid (format HH:MM) |
| 3 | Steps | Displayed when steps > 0 and no activity label |
| 4 (Bottom) | Eye Animation | Always displayed |

### Screen Brightness Levels

| Level | Contrast Value | Trigger Condition |
|------|----------|----------|
| Normal | 255 | Interaction present or just powered on |
| Dim | 80 | No interaction for 10s |
| Very Dim | 20 | No interaction for 20s |
| Sleep | 80 | Entered sleep state (remains visible) |
| Power Saving | 80 | Power saving mode enabled (forced) |

### Default Configuration Values

| Parameter | Default | Range |
|------|--------|------|
| Screen FPS | 30 | 10 ~ 50 (step 10) |
| Gyroscope Sample Rate | 50 Hz | 10 / 20 / 50 / 100 Hz |
| Action Cooldown | 1000 ms | 500 ~ 3000 ms |
| Idle Timeout | 30000 ms | 15000 ~ 120000 ms |
| Auto Dim | On | On / Off |
| Power Saving Mode | Off | On / Off |

---

## Expression & Activity Catalog

### Expression List (23)

| Expression | Duration | Dynamic Behavior | Trigger |
|------|----------|----------|----------|
| Default | -- | Default state, waiting to switch | Initial state / Expression ends |
| Looking Around | 8s | Random eye movement + curiosity mode | 60% probability (idle) |
| Happy | 2s | Gentle height adjustment breathing | 4% probability / Button |
| Comfortable | 3s | Breathing eye movement | Button (Mood > 70) |
| Heart | 3s | Pulsing eye movement | 4% probability / Double-click |
| Excited | 2.5s | Gentle height bouncy adjustment | Double-click trigger |
| Bored | 5s | Half-closed eyes + slow blink | 4% probability |
| Surprised | 2s | Wide eyes + looking around | Tap / Startup phase / Wake |
| Confused | 3s | Confused animation | 4% probability |
| Satisfied | 4s | Gentle blink | 4% probability (Mood >= 30) |
| Angry | 3s | Horizontal shake | Flick trigger |
| Pain | 2.5s | Eyes closed + sweat + high-frequency shake | Drop trigger |
| Dizzy | 3s | Rapid left-right rotating eye movement | Shake trigger |
| Scared | 2s | Wide eyes + slight tremble (low frequency) | Pain ends / 4% probability |
| Startling | 4s | Sudden eye opening | 4% probability |
| Sleepy | 5s | 4-stage eye-close animation | Idle timeout |
| Sleep | -- | Eyes closed, waiting for wake | Sleepy ends |
| Curious | 8s | Rapid blink + eye movement | 4% probability |
| Stretching | 2s | Eye stretch | 4% probability / Startup phase |
| Wink | 0.5s | Single eye wink | Tap 50% / Double-click |
| Thinking | 4s | Eyes tilted to one side + frown | 4% probability |
| Sad | 3s | Droopy eyes + occasional tears | 4% probability (Mood < 30) |
| Cool | 3s | Half-closed eyes + occasional eyebrow raise | 4% probability / Double-click |

### Activity List (19)

| Activity | Duration | Dynamic Behavior | Screen Label |
|------|----------|----------|----------|
| Drinking | 5s | Look down -> swallow action -> blink | Drinking... |
| Reading | 8s | Small eyes scanning left-right | Reading... |
| Eating | 6s | Happy chewing action | Eating... |
| Listening | 7s | Blink to the beat | Listening... |
| Yawning | 3s | Eyes close stretch + big->close->big->normal | Yawning... |
| Dancing | 6s | Bounce + sway (three directions) | Dancing~~ |
| Drawing | 8s | Focused gaze | Drawing... |
| Gaming | 10s | Rapid eye movement | Gaming... |
| Ice cream | 5s | Happy licking action | Ice cream! |
| Photo | 3s | Blink shutter | Photo! :D |
| Writing | 7s | Line-by-line scanning | Writing... |
| Workout | 6s | Up-down bounce | Workout... |
| Brushing | 5s | Left-right swing | Brushing... |
| Cooking | 8s | Looking around | Cooking... |
| Bathing | 6s | Comfortable eyes closed | Bathing... |
| Calling | 5s | Tilting to listen | Calling... |
| Noodles | 6s | Noodle slurping action | Noodles~~ |
| Mirror | 5s | Gazing forward | Mirror... |
| Stretching | 4s | Eye stretch | Stretching... |

> **Activity Label Notes**: Since the SSD1306 OLED with the Adafruit_GFX library does not support Chinese character rendering, all activity labels are displayed in English. Activity trigger probability: when idle for more than 8 seconds, there is a 30% probability of triggering a random activity. Activities last 3-10 seconds, then return to the default state.

---

## Troubleshooting

| Problem | Possible Cause | Solution |
|------|----------|----------|
| Screen not displaying | I2C address error or loose wiring | Check wiring, verify I2C address is 0x3C (not 0x3D) |
| Screen shows "No I2C devices" | I2C bus not connected | Check SDA(GPIO3)/SCL(GPIO4) wiring and power |
| MPU6050 not detected | AD0 pin level incorrect | AD0 to GND -> address 0x68; AD0 to 3.3V -> address 0x69 |
| Time not displayed (--:--) | RTC not synced | Triple-click button to enter AP mode, connect WiFi to complete NTP sync |
| WiFi connection failed | Wrong password or weak signal | Re-enter AP mode, select correct WiFi and enter password |
| Expressions not switching | Insufficient idle time | Need idle time of 6+ seconds for expression switching |
| Screen shows garbled text | Chinese not supported by OLED library | Change labels to English, upgrade firmware |
| Pet always sleeping | Idle timeout set too short | Enter AP mode, increase idle timeout (e.g., 120s) |
| Eyes frequently shaking erratically | Gyroscope drift / threshold too low | Deadband filter and calibration already added, upgrade firmware |
| Twitching when scared | Shake frequency too high | VFlicker/HFlicker already reduced from 3 to 1 |
| Excessive up-down swing when laughing | anim_laugh animation too aggressive | Already changed to gentle height adjustment |
| Tap not responding | Insufficient tap force | Needs acceleration > 15 m/s^2, gently tap the desk |
| Button single-click no response | GPIO pin conflict or detection failure | Verify GPIO9 (BOOT button), wait for 500ms window then confirm |
| Double-click not triggering | Interval between two presses > 500ms | Press faster, two presses within 500ms |
| Deep sleep cannot wake | Wake source not configured | Press BOOT button to wake (GPIO9 active low trigger) |
| Compilation error | Missing dependency libraries | Install all dependencies: Adafruit SSD1306, MPU6050, GFX, Unified Sensor, BusIO, ArduinoJson |

### Serial Debugging

Connect ESP32-C3 via USB, use Serial Monitor (115200 bps) to view runtime logs:

```bash
# Arduino IDE: Tools -> Serial Monitor, Baud Rate 115200

# PlatformIO:
pio device monitor --baud 115200
```

Logs will display I2C scan results, gyroscope calibration values, RTC status, WiFi connection process, button detection, action detection results, and other detailed information.

### Quick Diagnosis

After power-on, immediately check the serial output to confirm the following:

- I2C Scan: Should show `Found device at 0x3C (SSD1306)` and `Found device at 0x68 (MPU6050)`
- Gyroscope Calibration: Should show `Gyro calibration complete` and offset values
- RTC Status: Should show current time or `RTC starts from 0:00`
- WiFi Status: Should show connection success or `No WiFi found`

---

*Pet Robot -- ESP32-C3 Desktop Electronic Pet . Low Power Edition*  
*Arduino IDE / PlatformIO . SSD1306 + MPU6050 . RTC Timekeeping*