// ============================================================
//  CamOS - ESP32-CAM Firmware
//  Webserver, eigener Access Point, optionales WLAN, Bewegungserkennung,
//  Konsole, Regeln (Tasmota-Stil), Blitzsteuerung, SD, OTA, Plugins
// ============================================================
#include "core.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

String deviceId;
String streamToken;
volatile uint32_t restartAt = 0;

static DNSServer dns;
static bool apOn = false;
static bool staWas = false;
static uint32_t staLastTry = 0, staLostAt = 0, staUpAt = 0;

// ---------------- WLAN ----------------
bool wifiConnected() { return WiFi.status() == WL_CONNECTED; }
bool apActive() { return apOn; }

static void startAP() {
  IPAddress ip(192, 168, 4, 1), mask(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, mask);
  const char* pw = cfg.apPass.length() >= 8 ? cfg.apPass.c_str() : nullptr;
  WiFi.softAP(cfg.apSsid.c_str(), pw);
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", ip);
  apOn = true;
  logf("AP: '%s' gestartet (%s), IP " AP_IP_STR, cfg.apSsid.c_str(), pw ? "Passwort" : "offen");
  fireEvent("Wifi#AP", "1");
}

static void stopAP() {
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apOn = false;
  logf("AP: gestoppt (WLAN verbunden)");
  fireEvent("Wifi#AP", "0");
}

void wifiApply() {
  WiFi.persistent(false);
  if (apOn) { dns.stop(); apOn = false; }
  WiFi.disconnect(true);
  delay(100);
  WiFi.setHostname(cfg.hostname.c_str());
  bool sta = cfg.wifiSsid.length() > 0;
  WiFi.mode(sta ? WIFI_AP_STA : WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  startAP();
  staWas = false;
  if (sta) {
    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
    staLastTry = millis();
    logf("WLAN: verbinde mit '%s'...", cfg.wifiSsid.c_str());
  }
}

void wifiLoop() {
  if (!cfg.wifiSsid.length()) return;
  bool c = wifiConnected();
  uint32_t now = millis();
  if (c && !staWas) {
    staWas = true;
    staUpAt = now;
    logf("WLAN: verbunden, IP %s, RSSI %d dBm", WiFi.localIP().toString().c_str(), WiFi.RSSI());
    fireEvent("Wifi#Connected", WiFi.localIP().toString());
  } else if (!c && staWas) {
    staWas = false;
    staLostAt = now;
    logf("WLAN: Verbindung verloren");
    fireEvent("Wifi#Disconnected");
  }
  // AP-Modus 1: AP nur, wenn kein WLAN
  if (c && cfg.apMode == 1 && apOn && now - staUpAt > 15000 && WiFi.softAPgetStationNum() == 0) stopAP();
  if (!c) {
    if (cfg.apMode == 1 && !apOn && now - staLostAt > 20000) { WiFi.mode(WIFI_AP_STA); startAP(); }
    if (now - staLastTry > 30000) {
      staLastTry = now;
      WiFi.disconnect();
      WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
    }
  }
}

// ---------------- Zeit ----------------
static char tzBuf[64], ntpBuf[64];
void timeApply() {
  strlcpy(tzBuf, cfg.tz.c_str(), sizeof tzBuf);
  strlcpy(ntpBuf, cfg.ntp.c_str(), sizeof ntpBuf);
  configTzTime(tzBuf, ntpBuf, "time.google.com");
}
bool timeValid() { return time(nullptr) > 1700000000; }
String timeStr(const char* fmt) {
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  char b[48];
  strftime(b, sizeof b, fmt, &t);
  return String(b);
}

static void timeLoop() {
  static uint32_t last = 0;
  static int lastMin = -1;
  static bool wasValid = false;
  if (millis() - last < 1000) return;
  last = millis();
  if (!timeValid()) return;
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  if (!wasValid) { wasValid = true; logf("Zeit: synchronisiert %s", timeStr().c_str()); fireEvent("Time#Initialized", timeStr()); }
  if (t.tm_min != lastMin) {
    lastMin = t.tm_min;
    fireEvent("Time#Minute", String(t.tm_hour * 60 + t.tm_min));
  }
}

// ---------------- Serielle Konsole ----------------
static void serialLoop() {
  static String line;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      if (line.length()) Serial.println(execCommand(line, false));
      line = "";
    } else if (line.length() < 1024) line += ch;
  }
}

// ---------------- Setup / Loop ----------------
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);   // Brownout aus (ESP32-CAM Spannungseinbrueche)
  Serial.begin(115200);
  delay(200);
  logInit();
  logf("==== %s v%s startet ====", FW_NAME, FW_VERSION);
  commandsInit();
  settingsLoad();
  rulesInit();
  streamToken = String(esp_random(), HEX) + String(esp_random(), HEX);

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, HIGH);
  cameraInit();
  sdInit();
  flashInit();   // nach SD, damit GPIO4 wieder der Blitz gehoert
  buttonInit();  // nach Kamera (GPIO0 = XCLK)

  wifiApply();
  timeApply();
  if (MDNS.begin(cfg.hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    logf("mDNS: http://%s.local", cfg.hostname.c_str());
  }
  webStart();
  motionStart();
  pluginsSetup();
  logf("Bereit. Access Point '%s' -> http://" AP_IP_STR, cfg.apSsid.c_str());
  fireEvent("System#Boot", FW_VERSION);
}

void loop() {
  serialLoop();
  wifiLoop();
  timeLoop();
  buttonLoop();
  rulesLoop();
  pluginsLoop();
  if (apOn) dns.processNextRequest();
  if (restartAt && (int32_t)(millis() - restartAt) >= 0) {
    logf("Neustart...");
    delay(200);
    ESP.restart();
  }
  delay(2);
}
