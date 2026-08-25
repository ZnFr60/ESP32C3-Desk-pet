# ESP32C3 Desk-pet（重构迭代版）

一款基于 ESP32-C3 的桌面电子宠物机器人。通过 OLED 屏幕显示灵动大眼睛，搭载 MPU6050 实现运动感知，具备表情系统、活动动画、心情系统与多层省电策略，支持 WiFi 校时与 Web 配置。

> **当前版本为「重构迭代版」**：已将原 Arduino 框架工程完整迁移到 **ESP-IDF 原生框架**（禁用 Arduino-ESP32 组件），并在实机上验证运行正常（OLED 显示、MPU6050 检测、I2C 扫描、系统稳定运行）。

---

## 目录

```
ESP32C3-Desk-pet/
├── CMakeLists.txt          # ESP-IDF 顶层工程文件
├── sdkconfig.defaults      # 关键配置（esp32c3 / 4MB Flash / USB 控制台）
├── main/
│   ├── app_main.cpp        # IDF 入口（NVS 初始化 + setup()/loop()）
│   ├── esp32-pet-robot-arduino.cpp  # 业务主程序（屏幕/运动/按键/时间/WiFi 逻辑）
│   ├── Config.h            # 引脚、屏幕、运动、心情等全部宏配置
│   ├── RoboEyesManager.h   # 表情/活动状态机
│   ├── WiFiManager.h       # WiFi 自动连接 + AP 配网 Web 服务器
│   ├── TimeManager.h       # RTC 时间管理 + NTP 同步
│   ├── FluxGarage_RoboEyes.h      # 大眼睛动画绘制（模板）
│   ├── compat/             # Arduino API → ESP-IDF 原生接口适配层（自研）
│   └── lib/                # Adafruit GFX/SSD1306/MPU6050/Sensor/BusIO 驱动
└── legacy_broken_code/     # 早期旧代码归档（仅软件测试、实物不可运行）
```

## 硬件清单

| 器件 | 型号/规格 |
|---|---|
| 主控 | ESP32-C3（4MB Flash，原生 USB-Serial-JTAG） |
| 屏幕 | SSD1306 OLED，128×64，I2C 接口 |
| 传感器 | MPU6050 六轴（加速度+陀螺仪），I2C 接口 |
| 按键 | 板载 BOOT 按键（GPIO9，低电平有效） |

## 引脚接线表

| 功能 | 引脚 | 说明 |
|---|---|---|
| I2C 数据线 SDA | GPIO3 | SSD1306 与 MPU6050 共用 |
| I2C 时钟线 SCL | GPIO4 | 同上 |
| OLED 复位 RST | GPIO5 | 低电平有效，主动复位（修复黑屏关键） |
| BOOT 按键 | GPIO9 | 单/双/三连/长按交互 |
| 板载 LED | GPIO8 | OLED 异常时闪烁提示 |

> 引脚均可在 `main/Config.h` 中调整。I2C 总线启用内部上拉。

## ESP-IDF 环境版本

- **ESP-IDF v5.3.1**（本工程基于 5.3 验证编译）
- 目标芯片：`esp32c3`

## 编译与烧录

```bash
# 1. 进入工程并加载 IDF 环境
cd ESP32C3-Desk-pet

# 2. 设定目标芯片（首次）
idf.py set-target esp32c3

# 3. 编译
idf.py build

# 4. 烧录 + 串口监视（COMx 为实际端口）
idf.py -p COMx flash monitor
```

- 本机采用 **原生 USB-Serial-JTAG** 控制台，`Serial` 日志直接输出到 USB 虚拟串口（115200）。
- 上电后串口会打印 I2C 总线扫描、OLED/MPU 检测、系统就绪等日志，用于定位硬件问题。

## 下载与编译（ZIP 包）

提供可直接用 ESP-IDF 编译的源码包：

- **`release/ESP32C3-Desk-pet.zip`** — 完整 ESP-IDF 工程（标准结构）。下载解压后即可编译：

```bash
# 1. 解压 ESP32C3-Desk-pet.zip
# 2. 进入解压目录（先加载 ESP-IDF 环境，Windows 下如：./export.ps1）
cd ESP32C3-Desk-pet

# 3. 设定目标芯片（首次）
idf.py set-target esp32c3

# 4. 编译
idf.py build

# 5. 烧录 + 监视（COMx 为实际端口）
idf.py -p COMx flash monitor
```

- **`release/firmware.bin`** — 预编译的应用固件（ESP32-C3，4MB Flash）。可用 esptool 直接烧录：

```bash
python -m esptool --chip esp32c3 write_flash 0x0 release/bootloader.bin 0x8000 release/partition-table.bin 0x10000 release/firmware.bin
```

## 功能特性

- **表情系统**：23+ 种表情（开心、惊讶、困倦、生气、疼痛、眩晕、思考、发呆等）。
- **活动动画**：9+ 种活动（喝水、阅读、吃饭、跳舞、绘画、玩游戏、运动等）。
- **运动感知**：跌落（疼痛）、摇晃（眩晕）、弹击（生气）、左右倾（眼睛看方向）、计步、单击/双击敲击。
- **心情系统**：随交互增加、随时间衰减（0–100）。
- **交互方式**：单击=抚摸、双击=自定义动作、三连按=AP 配网、长按 3s=关机（RTC 保持）。
- **省电策略**：自动变暗、空闲休眠、深睡（BOOT 唤醒）。
- **WiFi 校时**：首次上电/午夜自动联网 NTP 校时，校时后断开 WiFi 省电，由 RTC 维持时间。
- **Web 配网**：三连按进入 AP 模式（`PetRobot-Setup`，`192.168.4.1`），可配置 WiFi 与显示/运动参数（NVS 持久化）。

## 模块划分（重构说明）

- `main/compat/`：**Arduino API 适配层**，用 ESP-IDF 原生接口封装 `digitalWrite/pinMode/Serial/Wire/WiFi/Preferences/WebServer/String/PROGMEM` 等（`driver/gpio`、`driver/i2c`、UART 控制台、`esp_wifi`、`esp_http_server`、`nvs_flash`、`esp_timer`、`esp_sntp`）。
- `main/lib/`：Adafruit 第三方驱动（GFX/SSD1306/MPU6050/Sensor/BusIO），已内置于工程本地。
- `main/*.h` + `*.cpp`：业务逻辑（驱动、屏幕 UI、业务逻辑分离）。

## legacy_broken_code 说明

`legacy_broken_code/` 存放**早期旧代码归档**（原 Arduino 框架、试用版等）。这些代码**仅做过软件层面的编译测试，未在实物硬件上验证运行，无法在实机正常启动**（存在屏幕不显示等硬件故障），仅作为历史存档保留，不属于当前可运行的工程。当前可运行版本为仓库根目录的 ESP-IDF 工程。

## 开源协议与第三方依赖许可

- **本项目**：遵循 **GPL v3**（GNU General Public License v3.0），详见 `LICENSE`。
- **Adafruit GFX / SSD1306 / MPU6050 / Sensor / BusIO**：BSD 许可证（Adafruit Industries）。
- **FluxGarage RoboEyes**：GPL v3（www.fluxgarage.com）。
- `main/compat/` 自研适配层与业务源码：GPL v3。

## 项目已知限制

- `setCpuFrequencyMhz()` 为**空实现**（未启用 ESP-PM 动态调频），功能不受影响，仅略去激进省电档。
- I2C 驱动使用 legacy `driver/i2c.h`（已在 5.x 标记为 deprecated，建议后续迁移 `i2c_master` 新驱动）。
- WiFi/NTP 校时需联网且首次需经 AP 配网；校时失败则从 00:00 开始计数（RTC 维持）。
- 运动/敲击/按键类交互需**实物操作**触发，无法纯软件模拟验证。
- `Config.h` 中引脚为默认接线，若实际硬件接线不同请按表调整。

## 许可与说明

本项目仅供学习交流。使用、修改、再分发请遵守 GPL v3 与各第三方库的许可条款。
