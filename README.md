# ESP32C3 Desk-pet (Reconstructed Iterative Build)

A desktop virtual-pet robot based on the ESP32-C3. It renders lively robot eyes on an OLED, uses an MPU6050 for motion sensing, and features an expression system, activity animations, a mood system, and multi-layer power saving, plus WiFi time sync and web-based provisioning.

> **This is the "reconstructed iterative build"**: the original Arduino-framework project has been fully migrated to the **native ESP-IDF framework** (no Arduino-ESP32 component), and verified on real hardware (OLED display, MPU6050 sensing, I2C scan, stable system operation).

---

## Layout

```
ESP32C3-Desk-pet/
├── CMakeLists.txt          # ESP-IDF top-level project file
├── sdkconfig.defaults      # Key config (esp32c3 / 4MB Flash / USB console)
├── main/
│   ├── app_main.cpp        # IDF entry (NVS init + setup()/loop())
│   ├── esp32-pet-robot-arduino.cpp  # Main business logic (display/motion/button/time/WiFi)
│   ├── Config.h            # All macros: pins, display, motion, mood, etc.
│   ├── RoboEyesManager.h   # Expression/activity state machine
│   ├── WiFiManager.h       # WiFi auto-connect + AP provisioning web server
│   ├── TimeManager.h       # RTC time keeping + NTP sync
│   ├── FluxGarage_RoboEyes.h      # Animated eyes drawing (template)
│   ├── compat/             # Arduino API → native ESP-IDF adapter layer (own code)
│   └── lib/                # Adafruit GFX/SSD1306/MPU6050/Sensor/BusIO drivers
└── legacy_broken_code/     # Archived early code (software-tested only, not runnable on hardware)
```

## Hardware

| Part | Model/Spec |
|---|---|
| MCU | ESP32-C3 (4MB Flash, native USB-Serial-JTAG) |
| Display | SSD1306 OLED, 128×64, I2C |
| Sensor | MPU6050 6-axis (accel+gyro), I2C |
| Button | On-board BOOT button (GPIO9, active low) |

## Pin Wiring

| Function | Pin | Note |
|---|---|---|
| I2C SDA | GPIO3 | Shared by SSD1306 and MPU6050 |
| I2C SCL | GPIO4 | Same |
| OLED RST | GPIO5 | Active low, driven actively (key fix for black screen) |
| BOOT button | GPIO9 | Single/double/triple/long press |
| On-board LED | GPIO8 | Blinks when OLED fails |

> All pins are adjustable in `main/Config.h`. Internal pull-ups are enabled on the I2C bus.

## ESP-IDF Environment

- **ESP-IDF v5.3.1** (verified build on 5.3)
- Target chip: `esp32c3`

## Build & Flash

```bash
# 1. cd into the project and source the IDF environment
cd ESP32C3-Desk-pet

# 2. Set the target chip (first time)
idf.py set-target esp32c3

# 3. Build
idf.py build

# 4. Flash + monitor (COMx = your port)
idf.py -p COMx flash monitor
```

- This board uses the **native USB-Serial-JTAG** console, so `Serial` logs go straight to the USB virtual COM port (115200).
- On power-up the serial prints the I2C scan, OLED/MPU detection, and "system ready" logs for hardware troubleshooting.

## Download & Build from Source (ZIP)

A ready-to-build source package is provided so you can compile directly with ESP-IDF:

- **`release/ESP32C3-Desk-pet.zip`** — the complete ESP-IDF project (standard structure). Download, extract, then build:

```bash
# 1. Extract ESP32C3-Desk-pet.zip
# 2. cd into the extracted folder (source the ESP-IDF env first, e.g. on Windows: ./export.ps1)
cd ESP32C3-Desk-pet

# 3. Set the target chip (first time)
idf.py set-target esp32c3

# 4. Build
idf.py build

# 5. Flash + monitor (COMx = your port)
idf.py -p COMx flash monitor
```

- **`release/firmware.bin`** — pre-built application firmware (ESP32-C3, 4MB flash). Flash it directly with esptool:

```bash
python -m esptool --chip esp32c3 write_flash 0x0 release/bootloader.bin 0x8000 release/partition-table.bin 0x10000 release/firmware.bin
```

## Features

- **Expressions**: 23+ (happy, surprised, sleepy, angry, pain, dizzy, thinking, bored, ...).
- **Activities**: 9+ (drinking, reading, eating, dancing, drawing, gaming, exercising, ...).
- **Motion sensing**: drop (pain), shake (dizzy), flick (angry), left/right tilt (eyes look that way), step counting, single/double tap.
- **Mood system**: grows with interaction, decays over time (0–100).
- **Interactions**: single press = petting, double press = custom action, triple press = AP provisioning, long press 3s = shutdown (RTC keeps time).
- **Power saving**: auto-dim, idle sleep, deep sleep (wake by BOOT).
- **WiFi time sync**: on first power-up / at midnight it auto-connects and NTP-syncs, then disconnects to save power (RTC keeps time).
- **Web provisioning**: triple-press enters AP mode (`PetRobot-Setup`, `192.168.4.1`) to configure WiFi and display/motion settings (persisted in NVS).

## Module Split (reconstruction notes)

- `main/compat/`: **Arduino API adapter layer** that wraps `digitalWrite/pinMode/Serial/Wire/WiFi/Preferences/WebServer/String/PROGMEM` with native ESP-IDF APIs (`driver/gpio`, `driver/i2c`, UART console, `esp_wifi`, `esp_http_server`, `nvs_flash`, `esp_timer`, `esp_sntp`).
- `main/lib/`: Adafruit third-party drivers (GFX/SSD1306/MPU6050/Sensor/BusIO), bundled locally.
- `main/*.h` + `*.cpp`: business logic (drivers, UI, business logic kept separate).

## legacy_broken_code

`legacy_broken_code/` holds the **archived early code** (original Arduino framework, trial build, etc.). That code was **only software/compile-tested and never validated on real hardware; it cannot boot correctly on the actual device** (e.g. the black-screen display issue). It is kept purely as a historical archive and is not part of the runnable project. The runnable build is the ESP-IDF project at the repository root.

## License & Third-Party Notices

- **This project**: **GPL v3** (GNU General Public License v3.0), see `LICENSE`.
- **Adafruit GFX / SSD1306 / MPU6050 / Sensor / BusIO**: BSD license (Adafruit Industries).
- **FluxGarage RoboEyes**: GPL v3 (www.fluxgarage.com).
- `main/compat/` adapter layer and business source: GPL v3.

## Known Limitations

- `setCpuFrequencyMhz()` is a **no-op** (ESP-PM dynamic frequency scaling not enabled); functionality is unaffected, only the aggressive power-saving tier is skipped.
- I2C uses the legacy `driver/i2c.h` (deprecated in 5.x; migration to the `i2c_master` driver is recommended).
- WiFi/NTP sync requires a network and initial AP provisioning; if sync fails the RTC starts from 00:00.
- Motion/tap/button interactions require **physical operation** and cannot be verified purely in software.
- Pins in `Config.h` are the default wiring; adjust them if your hardware differs.

## License

For learning and exchange only. Use, modification and redistribution must comply with GPL v3 and the licenses of the third-party libraries.
