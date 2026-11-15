<div align="center">

# 📷 CamOS

**A feature-rich firmware for the ESP32-CAM.** It has a web UI, its own access point, motion detection, Tasmota-style rules, a console, an SD file manager and a plugin system built for AI-assisted development.

![Platform](https://img.shields.io/badge/platform-ESP32--CAM-blue)
![Framework](https://img.shields.io/badge/framework-Arduino%20%7C%20PlatformIO-orange)
![Version](https://img.shields.io/badge/version-1.2.0-green)
![Sensors](https://img.shields.io/badge/sensors-OV2640%20%7C%20OV3660%20%7C%20OV5640-blueviolet)
![No Cloud](https://img.shields.io/badge/cloud-not%20required-success)

🇩🇪 **[Deutsche Anleitung → ANLEITUNG.md](ANLEITUNG.md)** · 🤖 **[Plugin guide for AI → PLUGINS_AI.md](PLUGINS_AI.md)**

</div>

---

## ✨ Features

| | |
|---|---|
| 📶 **Own access point** | Works out of the box without a router. Includes a captive portal that opens the UI automatically on phones. |
| 🏠 **Optional home WiFi** | AP stays on always, or only when WiFi is down. mDNS (`camos-xxxxxx.local`). |
| 📸 **Multi-sensor** | Detects **OV2640** (2 MP, up to UXGA), **OV3660** (3 MP, up to QXGA 2048×1536) and OV5640 (5 MP) automatically, with matching default image settings for each. |
| 🎥 **Live stream** | MJPEG on port 81, which works with VLC, Home Assistant and Frigate. Snapshots at `/capture`. |
| 🏃 **Motion detection** | On-device frame diffing with adjustable threshold, sensitivity, interval and cooldown, plus a live graph. |
| ⚙️ **Rules engine** | Tasmota-style `ON Motion#Detected DO Flash 255 ENDON`, with variables, timers, schedules and `Backlog`. |
| 💻 **Console** | In the browser and over serial, with 50+ commands and command history. |
| 💡 **Flash control** | Dimmable LED (0–255), pulse, and flash for photos. |
| 🔘 **Photo button** | Press **IO0** briefly to take a photo. It is saved to SD, or downloaded to the PC if no card is inserted. |
| 💾 **SD file manager** | Storage usage, folders, upload (drag & drop), download, preview and delete. |
| 🔔 **Notifications** | `WebSend` HTTP(S) requests, e.g. to ntfy.sh or Home Assistant webhooks. |
| 🧩 **Plugins** | One `.cpp` file per plugin, with a guide written so an AI can generate plugins. |
| 🔄 **OTA updates** | Upload new firmware from the browser. |
| 🔒 **Password protection** | Optional HTTP Basic Auth. |
| 🕒 **NTP time** | Timezone support and time-based rules. |

---

## 🚀 Quick start

### 1. Flash the prebuilt firmware

Download `CamOS-esp32cam-factory.bin` from the [`bin/`](bin/) folder (or from Releases) and flash it to address **`0x0`**:

**In the browser (no install):** open [espressif.github.io/esptool-js](https://espressif.github.io/esptool-js/) in Chrome or Edge. Put the board into flash mode (hold IO0 and press RST), then connect, set the address to `0x0`, choose the file and click Program.

**With esptool:**
```bash
esptool.py --chip esp32 --port COM5 --baud 460800 write_flash 0x0 bin/CamOS-esp32cam-factory.bin
```

### 2. Connect

1. Join the WiFi **`CamOS-XXXXXX`**. The password is **`camos1234`**.
2. Open **http://192.168.4.1**. On most phones it opens automatically.
3. Optional: add your home WiFi under **System → WLAN**.

> ⚠️ Please change the AP password and set a web password after the first start.

### 3. Build from source (optional)

```bash
pip install platformio
```
```bash
pio run
```
```bash
pio run -t upload
```

Every build writes ready-to-flash files to `bin/`:

| File | Use |
|---|---|
| `CamOS-esp32cam-factory.bin` | First install, flash at `0x0` (bootloader, partitions and app) |
| `CamOS-esp32cam-ota.bin` | Update through the web UI (System → Firmware-Update) |

---

## 🔧 Hardware

- **AI-Thinker ESP32-CAM** (4 MB PSRAM) with an **OV2640** or **OV3660** camera (OV5640 should work too)
- USB-serial adapter or **ESP32-CAM-MB** board for flashing
- Stable **5 V / ≥ 500 mA** power supply. Weak power is the #1 cause of reboots and camera errors.
- Optional: microSD card (FAT32), PIR sensor, push button

| GPIO | Function |
|---|---|
| 4 | Flash LED |
| 33 | Red status LED (heartbeat plugin) |
| 0 | Camera XCLK and IO0 photo button |
| 2, 14, 15 | SD card (1-bit mode) |
| 12, 13 | Free (PIR, button, relays …) |

---

## ⚙️ Rules (Tasmota-style)

Write rules in the web UI or the console:

```text
Rule1 ON Motion#Detected DO Backlog Flash 255; Delay 50; Flash 0 ENDON
Rule1 1
```

```text
ON Motion#Detected DO WebSend https://ntfy.sh/my_topic?m=Motion+%value%25 ENDON
ON Time#Minute=1320 DO Motion 0 ENDON  ON Time#Minute=360 DO Motion 1 ENDON
ON Button#Long DO Flash toggle ENDON
ON System#Boot DO RuleTimer1 60 ENDON  ON Rules#Timer=1 DO Backlog Photo tl; RuleTimer1 60 ENDON
```

**Events:** `System#Boot` `Motion#Detected` `Motion#End` `Button#Short` `Button#Long` `Wifi#Connected` `Wifi#Disconnected` `Time#Minute` `Rules#Timer` `SD#Saved` `Photo#Ready` `Event#<name>` …
**Placeholders:** `%value%` `%var1%` `%mem1%` `%time%` `%date%` `%timestamp%` `%motion%` `%ip%` `%device%` …

Run `Help` in the console for the full command list, or see [ANLEITUNG.md](ANLEITUNG.md).

---

## 🧩 Plugins

A plugin is a single file in `src/plugins/`:

```cpp
#include "../plugin_api.h"

class HelloPlugin : public Plugin {
public:
  const char* name() override { return "hello"; }
  void onEvent(const String& e, const String& v) override {
    if (e == "Motion#Detected") api::log("Hello! Motion: %s%%", v.c_str());
  }
  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd != "hello") return false;
    out = "{\"Hello\":\"World\"}";
    return true;
  }
};
REGISTER_PLUGIN(HelloPlugin)
```

Plugins can add **commands**, react to **events**, fire **events**, expose **status**, serve **HTTP endpoints** (`/p/<name>/…`) and store **settings**.

**Built-in plugins:** `heartbeat` (status LED), `timelapse` (photos to SD), `pir` (motion sensor on GPIO), `webhook` (HTTP call on motion).

### 🤖 Build plugins with AI
[`PLUGINS_AI.md`](PLUGINS_AI.md) is written as a complete spec for LLMs. It covers pins, the API, threading rules and examples. Give it to Claude, ChatGPT or another assistant together with your idea:

> *"Read PLUGINS_AI.md and write a CamOS plugin that sends a photo to my Telegram bot when motion is detected."*

Save the result in `src/plugins/`, run `pio run`, and upload the OTA bin.

---

## 🌐 HTTP API

| Endpoint | Description |
|---|---|
| `GET /` | Web UI |
| `GET :81/stream` | MJPEG stream (`?t=<token>` when a password is set) |
| `GET /capture[?flash=1]` | JPEG snapshot |
| `GET /api/status` | Full device status (JSON) |
| `POST /api/cmd` | Run a console command (command in the request body) |
| `GET /api/log?from=N` | Log lines |
| `GET /api/lastphoto` | Last photo held in RAM (no SD card) |
| `GET /api/fs?dir=/` | List SD directory |
| `GET /api/dl?f=/path[&inline=1]` | Download or view a file |
| `POST /api/upload?f=/path` | Upload a file (raw body) |
| `POST /api/rm?f=/path` · `POST /api/mkdir?d=/path` | Delete / create folder |
| `GET /api/rules` · `POST /api/rule?n=1&on=1` | Read / save rules |
| `POST /api/ota` | Firmware update (raw .bin body) |
| `GET/POST /p/<plugin>/…` | Plugin endpoints |

Example:
```bash
curl -X POST http://192.168.4.1/api/cmd -d "Flash 128"
```

---

## 📁 Project structure

```
├── platformio.ini       # build config
├── tools/merge_bin.py   # creates bin/*.bin after each build
├── bin/                 # prebuilt firmware
├── src/
│   ├── main.cpp         # setup/loop, WiFi/AP, time, serial console
│   ├── camera.cpp       # camera + flash LED
│   ├── motion.cpp       # motion detection task
│   ├── button.cpp       # IO0 button + RAM photo
│   ├── rules.cpp        # rules engine, events, command queue
│   ├── commands.cpp     # console commands + status JSON
│   ├── storage.cpp      # SD card / file manager
│   ├── webserver.cpp    # HTTP API, stream, OTA
│   ├── web_ui.h         # single-page web UI
│   ├── plugin_api.h     # plugin API
│   ├── plugins.cpp      # plugin manager
│   └── plugins/         # plugins (one .cpp each)
├── ANLEITUNG.md         # full manual (German)
└── PLUGINS_AI.md        # plugin spec for AI assistants
```

---

## 🛠️ Troubleshooting

| Problem | Fix |
|---|---|
| Reboot loops / brownout | Use a stronger 5 V supply and a short cable |
| `Camera init failed 0x105` | Reseat the camera ribbon cable |
| Motion triggers constantly | Raise `MotionThreshold` (e.g. 35) or `MotionSensitivity` (e.g. 5) |
| Forgot the password | Send `Reset 1` over serial, or run `esptool.py erase_flash` and reflash |
| Only one stream at a time | This is a hardware limit. Use a proxy (e.g. go2rtc) for several viewers |

---

## 🤝 Contributing

Issues and pull requests are welcome, especially new plugins. Please keep plugins self-contained in `src/plugins/` and non-blocking (see [PLUGINS_AI.md](PLUGINS_AI.md)).

## 📄 License

<!-- Choose a license, e.g. MIT, and add a LICENSE file -->
No license has been chosen yet.
