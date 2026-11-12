# CamOS Plugin Guide (for AI assistants)

> **Für Menschen:** Gib diese Datei zusammen mit deiner Plugin-Idee an eine KI. Speichere das Ergebnis als `src/plugins/<name>.cpp` und baue mit `pio run`.
>
> **For the AI:** This document is the complete contract for writing a CamOS plugin. Follow it exactly. Output **one single C++ file** unless the user asks for something else. Write user-facing strings (descriptions, log messages) in the user's language.

---

## 1. Platform facts

- Hardware: AI-Thinker **ESP32-CAM** (ESP32, 4 MB flash, 4 MB PSRAM, OV2640 camera).
- Framework: **Arduino-ESP32 core 2.0.17** via PlatformIO (`platform = espressif32@6.9.0`). Use the Arduino 2.x APIs, e.g. `ledcSetup`/`ledcAttachPin`, not the 3.x `ledcAttach`.
- Built-in libraries you may include: `WiFi.h`, `HTTPClient.h`, `WiFiClientSecure.h`, `WiFiUdp.h`, `SD_MMC.h`, `FS.h`, `Preferences.h`, `ESPmDNS.h`, `Wire.h`, `SPI.h`, `Update.h`, `esp_camera.h`, `img_converters.h`, `mbedtls/*`.
- **No external libraries** are installed. If one is truly needed, tell the user to add it under `lib_deps =` in `platformio.ini` (for example `lib_deps = knolleary/PubSubClient@^2.8`) and mention this clearly.
- Free app flash is about 700 KB. RAM is limited: keep large buffers in PSRAM (`ps_malloc`) and free them again.

### Pins

| GPIO | Use | Free for plugins? |
|---|---|---|
| 4 | white flash LED (LEDC channel 2) | no, use `api::flash()` |
| 33 | red status LED, **active LOW** (used by the `heartbeat` plugin) | only if heartbeat is disabled |
| 12, 13 | free (12 must be LOW at boot!) | **yes** |
| 2, 14, 15 | SD card (1-bit mode) | only if no SD card is used |
| 16 | PSRAM | **never** |
| 0 | camera XCLK | **never** |
| 1, 3 | Serial TX/RX | avoid |
| all others | camera | **never** |

LEDC channels 0/1 (timer 0) belong to the camera and channel 2 belongs to the flash. Plugins should use channels **4–7**.

---

## 2. File template (copy this)

```cpp
// src/plugins/example.cpp
#include "../plugin_api.h"
#include "../config.h"      // optional: pins, FW_VERSION

class ExamplePlugin : public Plugin {
  // state
  uint32_t lastRun = 0;
  int interval = 10;              // seconds

public:
  const char* name() override { return "example"; }   // unique, lowercase, <= 12 chars, no spaces
  const char* description() override { return "Kurze Beschreibung (Befehl: Example <s>)"; }
  bool defaultEnabled() override { return true; }     // false if it needs hardware/config first

  void setup() override {
    interval = getInt("interval", 10);                // persistent setting
    api::log("example: gestartet, Intervall %d s", interval);
  }

  void loop() override {                              // called ~every 2-5 ms - NEVER block
    if (millis() - lastRun < (uint32_t)interval * 1000UL) return;
    lastRun = millis();
    api::event("Example#Tick", String(millis() / 1000));
  }

  void stop() override { /* release pins/buffers */ }

  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd != "example") return false;              // cmd is already lowercase
    if (args.length()) { interval = max(1, (int)args.toInt()); setInt("interval", interval); }
    out = "{\"Example\":" + String(interval) + "}";   // always return JSON
    return true;
  }

  void onEvent(const String& event, const String& value) override {
    if (event == "Motion#Detected") { /* react */ }
  }

  String status() override {                          // JSON object or ""
    return "{\"interval\":" + String(interval) + "}";
  }

  // GET/POST /p/example/<path>?<query>
  bool http(const String& path, const String& query, const String& body,
            String& contentType, String& out) override {
    if (path == "" || path == "info") { out = status(); return true; }   // contentType defaults to application/json
    return false;                                                         // -> 404
  }
};
REGISTER_PLUGIN(ExamplePlugin)
```

The class name must be unique and must not contain `::` because the macro pastes it into an identifier.

---

## 3. Plugin base class (from `src/plugin_api.h`)

| Method | When it is called | Notes |
|---|---|---|
| `const char* name()` | **required** | Identifier for `Plugin <name> 0/1`, `/p/<name>/`, NVS namespace (`p_<name>`). |
| `description()` | UI / `Plugins` | Short text that names the plugin's commands. |
| `defaultEnabled()` | first boot | User toggles later with `Plugin <name> 0/1` (saved). |
| `setup()` | on boot if enabled, or when enabled at runtime | Read settings, configure pins. |
| `loop()` | main loop, very often | **Non-blocking.** Use `millis()` timers. No `delay()` > 5 ms. |
| `stop()` | when disabled at runtime | Undo `setup()`. |
| `command(cmd, args, out)` | for every console command the core does not know | `cmd` = full command word in lowercase (digits included, e.g. `"relay2"`). Return `true` only if handled. `out` must be JSON. |
| `onEvent(event, value)` | for every event, before rules run | Runs in the main loop task. Do not block. |
| `status()` | `/api/plugins`, Plugins tab, `Plugins` command | Return a JSON **object** string or `""`. |
| `http(path, query, body, contentType, out)` | `/p/<name>/<path>` | `query` is the raw query string (`a=1&b=2`, not URL-decoded). Runs in the HTTP server task. |

Persistent settings helpers (NVS; **key ≤ 15 chars**):
`getStr(key, def)`, `getInt(key, def)`, `setStr(key, v)`, `setInt(key, v)`.

The `active` flag is managed by the system. Do not change it.

---

## 4. System API (`namespace api`)

```cpp
void   api::log(const char* fmt, ...);            // printf-style -> web console + Serial
void   api::event(const String& name, const String& value = "");   // trigger rules ("Name#Sub")
String api::exec(const String& command);          // run any console command now, returns JSON
void   api::queue(const String& command, uint32_t delayMs = 0);    // run later in main loop (non-blocking)
void   api::flash(int level);  int api::flash();  // flash LED 0..255
bool   api::motionActive();  float api::motionLevel();  uint32_t api::motionCount();
camera_fb_t* api::grab(bool useFlash = false);    // JPEG frame - MUST call api::release(fb)!
void   api::release(camera_fb_t* fb);
bool   api::sdReady();
String api::saveSnapshot(const char* prefix);     // saves /cam/<prefix>_<time>.jpg, returns name or ""
bool   api::wifiConnected();  String api::ip();
bool   api::timeValid();  String api::time(const char* fmt = "%Y-%m-%d %H:%M:%S");
String api::var(int n);  void api::setVar(int n, const String& v);  // rule variables Var1..Var8
String api::jsonEscape(const String& s);  String api::jstr(const String& s); // jstr adds quotes
String api::argWord(const String& args, int index);  // split args by spaces, 0-based
bool   api::parseBool(const String& s, bool current);  // on/off/1/0/true/toggle
```

### Useful console commands to call via `api::exec`/`api::queue`
`Flash <n>`, `FlashPulse <ms> [lvl]`, `Snapshot [prefix]`, `Motion 0|1`, `WebSend <url>`, `Event <name>=<val>`, `RuleTimer<n> <s>`, `Var<n> <v>`, `Backlog a; Delay 10; b`, `FrameSize VGA`, `Gpio12 1`. Run `Help` on the device for the full list.

### Core events plugins can react to
`System#Boot`, `Motion#Detected` (value = % changed), `Motion#End`, `Wifi#Connected` (IP), `Wifi#Disconnected`, `Wifi#AP` (1/0), `Time#Initialized`, `Time#Minute` (minute of day 0..1439), `Rules#Timer` (n), `Flash#State`, `SD#Saved` (filename), `WebSend#Code`, `Event#<name>`, `Gpio<n>#State`.

Name your own events `PluginName#Something`, for example `Telegram#Sent`, so users can write `ON Telegram#Sent DO ... ENDON`.

---

## 5. Rules for correct, safe plugins

1. **Never block `loop()` or `onEvent()`.** For slow work (HTTP, uploads > 100 ms) prefer `api::queue("WebSend ...")`, or do the work in your own FreeRTOS task:
   ```cpp
   xTaskCreatePinnedToCore([](void* p){ /* work */ vTaskDelete(nullptr); }, "myjob", 8192, this, 1, nullptr, 0);
   ```
   Do not run more than one such task at a time (use a `volatile bool busy` flag).
2. **Always release camera frames** with `api::release(fb)` on every code path. Holding a frame stalls the stream and motion detection.
3. `api::grab()` returns a **JPEG** (`fb->buf`, `fb->len`, `fb->width`, `fb->height`). For raw pixels use `jpg2rgb565(fb->buf, fb->len, out, JPG_SCALE_8X)` from `img_converters.h` into a `ps_malloc` buffer.
4. Thread safety: `command()` and `onEvent()` run serialized with other commands. `http()` runs in the HTTP task. Protect shared state that `http()` changes with a `portMUX`/mutex, or keep `http()` read-only and use `api::queue()` for actions.
5. Return **valid JSON** from `command()` and `status()`. Use `api::jstr()` for strings.
6. Keep `name()` short (≤ 12 chars) and NVS keys ≤ 15 chars.
7. HTTPS: use `WiFiClientSecure client; client.setInsecure();` (no CA bundle available) and tell the user that the certificate is not verified.
8. Check `api::wifiConnected()` before any internet access. The camera may run in AP-only mode.
9. Do not use `Serial.read()`, which belongs to the console. `Serial.print` is fine, but `api::log` is preferred.
10. Do not call `WiFi.mode()`, `esp_camera_init()`/`deinit()`, `ESP.restart()` or change GPIO 0/4/16/camera pins.
11. Set `defaultEnabled()` to `false` if the plugin needs wiring or credentials, so a fresh device does not misbehave.
12. Never hard-code the user's secrets. Store tokens and URLs via a command plus `setStr()`.

---

## 6. Example: sending a JPEG via HTTP POST (multipart-free)

```cpp
#include "../plugin_api.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class UploadPlugin : public Plugin {
  String url;
  volatile bool busy = false;

  static void job(void* p) {
    auto self = (UploadPlugin*)p;
    camera_fb_t* fb = api::grab(false);
    if (fb) {
      WiFiClientSecure sc; WiFiClient pc; HTTPClient http;
      bool https = self->url.startsWith("https://");
      if (https) sc.setInsecure();
      if (https ? http.begin(sc, self->url) : http.begin(pc, self->url)) {
        http.addHeader("Content-Type", "image/jpeg");
        int code = http.POST(fb->buf, fb->len);
        http.end();
        api::event("Upload#Code", String(code));
      }
      api::release(fb);
    }
    self->busy = false;
    vTaskDelete(nullptr);
  }

public:
  const char* name() override { return "upload"; }
  const char* description() override { return "Sendet bei Bewegung ein JPEG per HTTP POST (UploadUrl <url>)"; }
  bool defaultEnabled() override { return false; }
  void setup() override { url = getStr("url"); }

  void onEvent(const String& e, const String& v) override {
    if (e != "Motion#Detected" || busy || !url.length() || !api::wifiConnected()) return;
    busy = true;
    xTaskCreatePinnedToCore(job, "upload", 8192, this, 1, nullptr, 0);
  }

  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd != "uploadurl") return false;
    if (args.length()) { url = args == "\"\"" ? String("") : args; setStr("url", url); }
    out = "{\"UploadUrl\":" + api::jstr(url) + "}";
    return true;
  }
  String status() override { return "{\"busy\":" + String(busy ? "true" : "false") + "}"; }
};
REGISTER_PLUGIN(UploadPlugin)
```

More examples in the repo: `src/plugins/heartbeat.cpp` (LED / loop timing), `timelapse.cpp` (settings, SD, http), `pir.cpp` (GPIO input, events, `defaultEnabled false`), `webhook.cpp` (events + queue).

---

## 7. What to give back to the user

1. The complete file `src/plugins/<name>.cpp`.
2. Any `platformio.ini` changes (only if an external library is needed).
3. A short usage block: console commands, the events it fires, and example rules, e.g.
   ```
   UploadUrl https://example.com/upload
   Plugin upload 1
   Rule1 ON Upload#Code!=200 DO Echo Upload fehlgeschlagen: %value% ENDON
   ```
4. Wiring notes if hardware is involved (which GPIO, pull-ups, 3.3 V logic!).

Build & install: `pio run`, then upload `bin/CamOS-esp32cam-ota.bin` under **System → Firmware-Update**, or run `pio run -t upload` over USB.
