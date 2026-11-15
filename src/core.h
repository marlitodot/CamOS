#pragma once
// Interne Kern-API von CamOS. Plugins sollten stattdessen plugin_api.h benutzen.
#include <Arduino.h>
#include "config.h"
#include "esp_camera.h"

struct Settings {
  // Netzwerk
  String wifiSsid, wifiPass;     // optionales Heim-WLAN (leer = nur Access Point)
  String apSsid, apPass;         // eigener Access Point
  uint8_t apMode;                // 0 = AP immer an, 1 = AP nur wenn kein WLAN verbunden
  String hostname, webPass, tz, ntp;
  // Kamera
  int framesize, quality, brightness, contrast, saturation, effect;
  bool vflip, hmirror, awb;
  // Bewegungserkennung
  bool motionEnabled;
  int motionThreshold;    // Helligkeitsdifferenz pro Pixel (1..255)
  int motionSensitivity;  // Anteil geaenderter Pixel in 0.1% (30 = 3.0%)
  int motionInterval;     // ms zwischen Pruefungen
  int motionCooldown;     // s ohne Bewegung bis Motion#End
  bool motionSave;        // Foto bei Bewegung auf SD speichern
  // Blitz
  bool snapFlash;         // Blitz bei Fotos
  int  flashSnapLevel;    // Helligkeit Foto-Blitz 0..255
  // Taste
  int  buttonPin;         // -1 = aus, 0 = IO0-Taste, 12/13/14/15 = Taster gegen GND
  bool buttonPhoto;       // kurz druecken = Foto
};
extern Settings cfg;
extern String deviceId;
extern String streamToken;
extern volatile uint32_t restartAt;

// settings.cpp
void settingsLoad();
void settingsSave();
void settingsFactoryReset();

// logger.cpp
void logInit();
void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logLine(const String& s);
String logGet(uint32_t from, uint32_t& next);

// camera.cpp
extern bool cameraOk;
bool cameraInit();
void cameraApply();
camera_fb_t* cameraCapture(bool useFlash);
const char* framesizeName(int fs);
int framesizeFromName(const String& s);
const char* cameraSensorName();   // "OV2640", "OV3660", ...
int cameraMaxFramesize();         // groesste Aufloesung fuer Sensor + Speicher
void flashInit();
void flashSet(int level);
int  flashGet();

// motion.cpp
void motionStart();
extern volatile int  motionLevel;      // zuletzt gemessene Aenderung in 0.1%
extern volatile bool motionActive;
extern volatile uint32_t motionCount;
extern volatile uint32_t motionLastMs;

// rules.cpp
void rulesInit();
void rulesLoop();
void fireEvent(const String& name, const String& value = "");
void queueCommand(const String& cmd, uint32_t delayMs = 0);
String ruleGet(int n);
void ruleSet(int n, const String& t);
bool ruleEnabled(int n);
void ruleEnable(int n, bool on);
String varGet(int n);
void varSet(int n, const String& v);
String memGet(int n);
void memSet(int n, const String& v);
void ruleTimerSet(int n, uint32_t sec);
uint32_t ruleTimerLeft(int n);
String substVars(const String& s, const String& value);

// commands.cpp
void commandsInit();
String execCommand(const String& line, bool logResult = true);
String statusJson();
String jsonEscape(const String& s);
String jstr(const String& s);
bool parseBoolArg(const String& a, bool current);
String argWord(const String& args, int index);

// storage.cpp
extern bool sdOk;
bool sdInit();
String sdSaveSnapshot(const char* prefix);
bool sdSafePath(const String& p);
String sdDirJson(const String& dir);
bool sdRemove(const String& path);
bool sdMkdir(const String& path);

// button.cpp
extern volatile bool buttonHeld;      // IO0 gedrueckt (Kamera-Takt gestoert)
extern volatile uint32_t photoSeq;    // zaehlt Fotos im RAM (fuer PC-Download)
void buttonInit();
void buttonLoop();
String takePhoto(const char* prefix); // "sd:<datei>" | "ram:<name>" | ""
bool photoRamLock(const uint8_t** buf, size_t* len, String* name);
void photoRamUnlock();
String photoRamName();

// webserver.cpp
void webStart();
extern volatile int streamClients;

// main.cpp
void wifiApply();
void wifiLoop();
bool wifiConnected();
bool apActive();
void timeApply();
bool timeValid();
String timeStr(const char* fmt = "%Y-%m-%d %H:%M:%S");

// plugins.cpp
void pluginsSetup();
void pluginsLoop();
bool pluginsCommand(const String& cmd, const String& args, String& out);
void pluginsEvent(const String& name, const String& value);
String pluginsJson();
bool pluginsHttp(const String& path, const String& query, const String& body, String& ctype, String& out);
bool pluginSetEnabled(const String& name, bool on);
