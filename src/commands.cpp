// Konsolen-Befehle (Serial, Web-Konsole, Regeln, Plugins)
#include "core.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SD_MMC.h>

static SemaphoreHandle_t cmdMtx;
void commandsInit() { cmdMtx = xSemaphoreCreateRecursiveMutex(); }

// ---------------- Helfer ----------------
String jsonEscape(const String& s) {
  String o;
  o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '"':  o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
        else o += c;
    }
  }
  return o;
}
String jstr(const String& s) { return "\"" + jsonEscape(s) + "\""; }

bool parseBoolArg(const String& a, bool cur) {
  String s = a;
  s.trim();
  s.toLowerCase();
  if (s == "toggle" || s == "2") return !cur;
  return s == "1" || s == "on" || s == "true" || s == "yes" || s == "ein";
}

String argWord(const String& args, int index) {
  int i = 0, start = 0;
  String a = args;
  a.trim();
  while (start < (int)a.length()) {
    while (start < (int)a.length() && a[start] == ' ') start++;
    int sp = a.indexOf(' ', start);
    if (sp < 0) sp = a.length();
    if (i == index) return a.substring(start, sp);
    i++;
    start = sp + 1;
  }
  return "";
}

static String res(const String& k, const String& v) { return "{\"" + k + "\":" + jstr(v) + "}"; }
static String resN(const String& k, long v) { return "{\"" + k + "\":" + String(v) + "}"; }
static String resB(const String& k, bool v) { return "{\"" + k + "\":\"" + (v ? "ON" : "OFF") + "\"}"; }
static bool emptyArg(const String& a) { return a == "\"\""; }
static String numStr(double v) {
  if (v == (long)v) return String((long)v);
  return String(v, 3);
}

static const char HELP[] =
  "Befehle (Gross/Klein egal):\n"
  " System : Status | Restart 1 | Reset 1 | Time | Echo <txt> | Help\n"
  " Blitz  : Flash <0-255|on|off|toggle> | FlashPulse <ms> [level] | SnapFlash <0|1> | SnapLevel <0-255>\n"
  " Foto   : Photo [prefix] (SD, ohne SD -> PC-Download) | Snapshot [prefix] (nur SD) | SDInit\n"
  " Taste  : ButtonPin <-1|0|12|13|14|15> | ButtonPhoto <0|1>  (Events Button#Short / Button#Long)\n"
  " Motion : Motion <0|1> | MotionThreshold <1-255> | MotionSensitivity <0.1-100 %>\n"
  "          MotionInterval <ms> | MotionCooldown <s> | MotionSave <0|1>\n"
  " Kamera : FrameSize <QQVGA|QVGA|CIF|HVGA|VGA|SVGA|XGA|HD|SXGA|UXGA|FHD|QXGA|QSXGA> | Quality <4-63>\n"
  "          (OV2640 max. UXGA, OV3660 max. QXGA, OV5640 max. QSXGA)\n"
  "          Brightness|Contrast|Saturation <-2..2> | Effect <0-6> | VFlip|HMirror|AWB <0|1>\n"
  " WLAN   : WifiSSID <ssid> | WifiPass <pw> | WifiConnect | WifiScan\n"
  "          APSSID <ssid> | APPass <pw> | APMode <0=immer|1=nur ohne WLAN>\n"
  "          Hostname <name> | WebPassword <pw> | Timezone <POSIX-TZ> | NtpServer <host>\n"
  " Regeln : Rule<1-3> [text|+text|0|1|toggle|\"\"] | RuleTimer<1-8> <s>\n"
  "          Var<1-8> [wert] | Mem<1-8> [wert] | Add<n>/Sub<n>/Mult<n> <zahl>\n"
  "          Event <name>[=wert] | Backlog cmd1; Delay 10; cmd2 (Delay in 0.1s)\n"
  " Netz   : WebSend <url>  (HTTP GET, z.B. Benachrichtigung)\n"
  " GPIO   : Gpio<2|12|13|14|15> [0|1]\n"
  " Plugins: Plugins | Plugin <name> <0|1>\n"
  " Leeres Argument = \"\" ; Befehl ohne Argument zeigt aktuellen Wert.";

// ---------------- Status ----------------
String statusJson() {
  auto b = [](bool v) { return v ? "true" : "false"; };
  String j;
  j.reserve(1600);
  j += "{\"device\":"; j += jstr(deviceId);
  j += ",\"name\":"; j += jstr(cfg.hostname);
  j += ",\"version\":\"" FW_VERSION "\"";
  j += ",\"uptime\":"; j += String(millis() / 1000);
  j += ",\"heap\":"; j += String(ESP.getFreeHeap());
  j += ",\"psram\":"; j += String(ESP.getFreePsram());
  j += ",\"token\":"; j += jstr(streamToken);
  j += ",\"streams\":"; j += String(streamClients);
  j += ",\"time\":"; j += jstr(timeValid() ? timeStr() : String(""));

  bool wc = wifiConnected();
  j += ",\"wifi\":{\"ssid\":"; j += jstr(cfg.wifiSsid);
  j += ",\"connected\":"; j += b(wc);
  j += ",\"ip\":"; j += jstr(wc ? WiFi.localIP().toString() : String(""));
  j += ",\"rssi\":"; j += String(wc ? WiFi.RSSI() : 0);
  j += ",\"ap\":"; j += b(apActive());
  j += ",\"apssid\":"; j += jstr(cfg.apSsid);
  j += ",\"apip\":\"" AP_IP_STR "\"";
  j += ",\"clients\":"; j += String(apActive() ? WiFi.softAPgetStationNum() : 0);
  j += ",\"apmode\":"; j += String(cfg.apMode);
  j += ",\"hostname\":"; j += jstr(cfg.hostname);
  j += "}";

  j += ",\"cam\":{\"ok\":"; j += b(cameraOk);
  j += ",\"sensor\":"; j += jstr(cameraSensorName());
  j += ",\"maxfs\":\""; j += framesizeName(cameraMaxFramesize()); j += "\"";
  j += ",\"framesize\":\""; j += framesizeName(cfg.framesize);
  j += "\",\"quality\":"; j += String(cfg.quality);
  j += ",\"brightness\":"; j += String(cfg.brightness);
  j += ",\"contrast\":"; j += String(cfg.contrast);
  j += ",\"saturation\":"; j += String(cfg.saturation);
  j += ",\"effect\":"; j += String(cfg.effect);
  j += ",\"vflip\":"; j += b(cfg.vflip);
  j += ",\"hmirror\":"; j += b(cfg.hmirror);
  j += ",\"awb\":"; j += b(cfg.awb);
  j += ",\"psram\":"; j += b(psramFound());
  j += "}";

  j += ",\"motion\":{\"enabled\":"; j += b(cfg.motionEnabled);
  j += ",\"active\":"; j += b(motionActive);
  j += ",\"level\":"; j += String(motionLevel / 10.0f, 1);
  j += ",\"count\":"; j += String(motionCount);
  j += ",\"threshold\":"; j += String(cfg.motionThreshold);
  j += ",\"sensitivity\":"; j += String(cfg.motionSensitivity / 10.0f, 1);
  j += ",\"interval\":"; j += String(cfg.motionInterval);
  j += ",\"cooldown\":"; j += String(cfg.motionCooldown);
  j += ",\"save\":"; j += b(cfg.motionSave);
  j += "}";

  j += ",\"flash\":{\"level\":"; j += String(flashGet());
  j += ",\"snap\":"; j += b(cfg.snapFlash);
  j += ",\"snaplevel\":"; j += String(cfg.flashSnapLevel);
  j += "}";

  j += ",\"sd\":{\"ok\":"; j += b(sdOk);
  j += ",\"total\":"; j += String(sdOk ? (uint32_t)(SD_MMC.totalBytes() / (1024 * 1024)) : 0);
  j += "}";

  j += ",\"photo\":{\"seq\":"; j += String(photoSeq);
  j += ",\"name\":"; j += jstr(photoRamName());
  j += "}";

  j += ",\"button\":{\"pin\":"; j += String(cfg.buttonPin);
  j += ",\"photo\":"; j += b(cfg.buttonPhoto);
  j += "}";

  j += ",\"sys\":{\"tz\":"; j += jstr(cfg.tz);
  j += ",\"ntp\":"; j += jstr(cfg.ntp);
  j += ",\"webpass\":"; j += b(cfg.webPass.length() > 0);
  j += "}}";
  return j;
}

// ---------------- Dispatcher ----------------
#define CAM_INT(NM, KEY, FIELD, LO, HI) \
  if (base == NM) { if (has) { FIELD = constrain((int)args.toInt(), LO, HI); settingsSave(); cameraApply(); } return resN(KEY, FIELD); }
#define CAM_BOOL(NM, KEY, FIELD) \
  if (base == NM) { if (has) { FIELD = parseBoolArg(args, FIELD); settingsSave(); cameraApply(); } return resB(KEY, FIELD); }
#define SET_INT(NM, KEY, FIELD, LO, HI) \
  if (base == NM) { if (has) { FIELD = constrain((int)args.toInt(), LO, HI); settingsSave(); } return resN(KEY, FIELD); }
#define SET_BOOL(NM, KEY, FIELD) \
  if (base == NM) { if (has) { FIELD = parseBoolArg(args, FIELD); settingsSave(); } return resB(KEY, FIELD); }

static String webSend(const String& url) {
  HTTPClient http;
  WiFiClientSecure secure;
  WiFiClient plain;
  http.setTimeout(6000);
  bool ok;
  if (url.startsWith("https://")) { secure.setInsecure(); ok = http.begin(secure, url); }
  else ok = http.begin(plain, url);
  if (!ok) return res("WebSend", "invalid URL");
  int code = http.GET();
  String body = code > 0 ? http.getString() : HTTPClient::errorToString(code);
  http.end();
  if (body.length() > 300) body = body.substring(0, 300);
  fireEvent("WebSend#Code", String(code));
  return "{\"WebSend\":{\"Code\":" + String(code) + ",\"Body\":" + jstr(body) + "}}";
}

static String dispatch(const String& base, int idx, const String& full, const String& args) {
  bool has = args.length() > 0;

  // --- System ---
  if (base == "help") return res("Help", HELP);
  if (base == "status") return statusJson();
  if (base == "restart") {
    if (args == "1") { restartAt = millis() + 800; return res("Restart", "Restarting"); }
    return res("Restart", "Benutze 'Restart 1'");
  }
  if (base == "reset") {
    if (args == "1") { settingsFactoryReset(); restartAt = millis() + 800; return res("Reset", "Werkseinstellungen, Neustart"); }
    return res("Reset", "Benutze 'Reset 1' um ALLE Einstellungen zu loeschen");
  }
  if (base == "echo") { logLine(args); return res("Echo", args); }
  if (base == "time") return res("Time", timeValid() ? timeStr() : String("nicht synchronisiert"));

  // --- Blitz ---
  if (base == "flash") {
    if (has) {
      String a = args; a.toLowerCase();
      if (a == "on") flashSet(255);
      else if (a == "off") flashSet(0);
      else if (a == "toggle") flashSet(flashGet() ? 0 : 255);
      else flashSet(a.toInt());
      fireEvent("Flash#State", String(flashGet()));
    }
    return resN("Flash", flashGet());
  }
  if (base == "flashpulse") {
    int ms = has ? constrain((int)argWord(args, 0).toInt(), 10, 60000) : 200;
    String l = argWord(args, 1);
    flashSet(l.length() ? l.toInt() : 255);
    queueCommand("Flash 0", ms);
    return resN("FlashPulse", ms);
  }
  SET_BOOL("snapflash", "SnapFlash", cfg.snapFlash)
  SET_INT("snaplevel", "SnapLevel", cfg.flashSnapLevel, 0, 255)

  // --- Foto ---
  if (base == "snapshot" || base == "sdsave") {
    if (!sdOk) return res("Snapshot", "keine SD-Karte");
    String f = sdSaveSnapshot(has ? args.c_str() : "img");
    return res("Snapshot", f.length() ? f : String("fehlgeschlagen"));
  }

  if (base == "photo") {
    String r = takePhoto(has ? args.c_str() : "img");
    if (!r.length()) return res("Photo", "fehlgeschlagen");
    if (r.startsWith("sd:")) return "{\"Photo\":{\"Target\":\"SD\",\"File\":" + jstr(r.substring(3)) + "}}";
    return "{\"Photo\":{\"Target\":\"PC\",\"File\":" + jstr(r.substring(4)) + ",\"Url\":\"/api/lastphoto\"}}";
  }
  if (base == "sdinit") {
    if (!sdOk) { sdInit(); flashInit(); flashSet(flashGet()); }
    return resB("SD", sdOk);
  }

  // --- Taste ---
  if (base == "buttonpin") {
    if (has) {
      int p = args.toInt();
      if (!(p == -1 || p == 0 || p == 12 || p == 13 || p == 14 || p == 15))
        return res("ButtonPin", "erlaubt: -1 (aus), 0 (IO0-Taste), 12, 13, 14, 15");
      cfg.buttonPin = p;
      settingsSave();
      buttonInit();
    }
    return resN("ButtonPin", cfg.buttonPin);
  }
  SET_BOOL("buttonphoto", "ButtonPhoto", cfg.buttonPhoto)

  // --- Bewegung ---
  SET_BOOL("motion", "Motion", cfg.motionEnabled)
  SET_INT("motionthreshold", "MotionThreshold", cfg.motionThreshold, 1, 255)
  if (base == "motionsensitivity") {
    if (has) { cfg.motionSensitivity = constrain((int)(args.toFloat() * 10 + 0.5f), 1, 1000); settingsSave(); }
    return "{\"MotionSensitivity\":" + String(cfg.motionSensitivity / 10.0f, 1) + "}";
  }
  SET_INT("motioninterval", "MotionInterval", cfg.motionInterval, 100, 10000)
  SET_INT("motioncooldown", "MotionCooldown", cfg.motionCooldown, 1, 3600)
  SET_BOOL("motionsave", "MotionSave", cfg.motionSave)

  // --- Kamera ---
  if (base == "framesize") {
    if (has) {
      int fs = isDigit(args[0]) ? args.toInt() : framesizeFromName(args);
      if (fs < 0 || !strcmp(framesizeName(fs), "?")) return res("FrameSize", "unbekannt");
      if (fs > cameraMaxFramesize()) fs = cameraMaxFramesize();   // OV2640: UXGA, OV3660: QXGA
      cfg.framesize = fs;
      settingsSave();
      cameraApply();
    }
    return res("FrameSize", framesizeName(cfg.framesize));
  }
  CAM_INT("quality", "Quality", cfg.quality, 4, 63)
  CAM_INT("brightness", "Brightness", cfg.brightness, -2, 2)
  CAM_INT("contrast", "Contrast", cfg.contrast, -2, 2)
  CAM_INT("saturation", "Saturation", cfg.saturation, -2, 2)
  CAM_INT("effect", "Effect", cfg.effect, 0, 6)
  CAM_BOOL("vflip", "VFlip", cfg.vflip)
  CAM_BOOL("hmirror", "HMirror", cfg.hmirror)
  CAM_BOOL("awb", "AWB", cfg.awb)

  // --- WLAN ---
  if (base == "wifissid") { if (has) { cfg.wifiSsid = emptyArg(args) ? "" : args; settingsSave(); } return res("WifiSSID", cfg.wifiSsid); }
  if (base == "wifipass") { if (has) { cfg.wifiPass = emptyArg(args) ? "" : args; settingsSave(); } return res("WifiPass", cfg.wifiPass.length() ? "****" : ""); }
  if (base == "wificonnect") { queueCommand("_wifiapply", 300); return res("WifiConnect", cfg.wifiSsid.length() ? "verbinde mit " + cfg.wifiSsid : String("nur Access Point")); }
  if (base == "_wifiapply") { wifiApply(); return res("WifiConnect", "OK"); }
  if (base == "wifiscan") {
    int n = WiFi.scanNetworks();
    String o = "{\"WifiScan\":[";
    for (int i = 0; i < n && i < 30; i++) {
      if (i) o += ",";
      o += "{\"ssid\":" + jstr(WiFi.SSID(i)) + ",\"rssi\":" + String(WiFi.RSSI(i)) +
           ",\"ch\":" + String(WiFi.channel(i)) + ",\"enc\":" + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
    WiFi.scanDelete();
    return o + "]}";
  }
  if (base == "apssid") { if (has && !emptyArg(args)) { cfg.apSsid = args; settingsSave(); } return res("APSSID", cfg.apSsid); }
  if (base == "appass") {
    if (has) {
      String p = emptyArg(args) ? "" : args;
      if (p.length() && p.length() < 8) return res("APPass", "mind. 8 Zeichen oder \"\" fuer offen");
      cfg.apPass = p; settingsSave();
    }
    return res("APPass", cfg.apPass.length() ? "****" : "(offen)");
  }
  SET_INT("apmode", "APMode", cfg.apMode, 0, 1)
  if (base == "hostname") { if (has && !emptyArg(args)) { cfg.hostname = args; settingsSave(); } return res("Hostname", cfg.hostname); }
  if (base == "webpassword") { if (has) { cfg.webPass = emptyArg(args) ? "" : args; settingsSave(); } return res("WebPassword", cfg.webPass.length() ? "****" : "(aus)"); }
  if (base == "timezone") { if (has) { cfg.tz = args; settingsSave(); timeApply(); } return res("Timezone", cfg.tz); }
  if (base == "ntpserver") { if (has) { cfg.ntp = args; settingsSave(); timeApply(); } return res("NtpServer", cfg.ntp); }

  // --- Regeln ---
  if (base == "rule") {
    if (idx < 1 || idx > NUM_RULES) return res("Error", "Rule1..Rule3");
    if (has) {
      String al = args; al.toLowerCase();
      if (al == "0" || al == "off") ruleEnable(idx, false);
      else if (al == "1" || al == "on") ruleEnable(idx, true);
      else if (al == "toggle") ruleEnable(idx, !ruleEnabled(idx));
      else if (emptyArg(args)) ruleSet(idx, "");
      else if (args[0] == '+') ruleSet(idx, ruleGet(idx) + " " + args.substring(1));
      else ruleSet(idx, args);
    }
    return "{\"Rule" + String(idx) + "\":{\"State\":\"" + (ruleEnabled(idx) ? "ON" : "OFF") +
           "\",\"Rules\":" + jstr(ruleGet(idx)) + "}}";
  }
  if (base == "ruletimer") {
    if (idx < 1 || idx > NUM_TIMERS) return res("Error", "RuleTimer1..8");
    if (has) ruleTimerSet(idx, args.toInt());
    return resN("RuleTimer" + String(idx), ruleTimerLeft(idx));
  }
  if (base == "var" || base == "mem") {
    if (idx < 1 || idx > NUM_VARS) return res("Error", "1..8");
    bool mem = base == "mem";
    if (has) {
      String v = emptyArg(args) ? String("") : substVars(args, "");
      if (mem) memSet(idx, v); else varSet(idx, v);
    }
    return res((mem ? "Mem" : "Var") + String(idx), mem ? memGet(idx) : varGet(idx));
  }
  if (base == "add" || base == "sub" || base == "mult") {
    if (idx < 1 || idx > NUM_VARS) return res("Error", "1..8");
    double a = varGet(idx).toDouble(), b = substVars(args, "").toDouble();
    double r = base == "add" ? a + b : base == "sub" ? a - b : a * b;
    varSet(idx, numStr(r));
    return res("Var" + String(idx), varGet(idx));
  }
  if (base == "event") {
    int eq = args.indexOf('=');
    String n = eq < 0 ? args : args.substring(0, eq);
    String v = eq < 0 ? String("") : args.substring(eq + 1);
    n.trim();
    fireEvent("Event#" + n, v);
    return res("Event", "Done");
  }
  if (base == "backlog") {
    uint32_t off = 0;
    int start = 0, cnt = 0;
    while (start <= (int)args.length()) {
      int semi = args.indexOf(';', start);
      if (semi < 0) semi = args.length();
      String item = args.substring(start, semi);
      item.trim();
      start = semi + 1;
      if (!item.length()) continue;
      String il = item; il.toLowerCase();
      if (il.startsWith("delay")) off += item.substring(5).toInt() * 100;
      else { queueCommand(item, off); cnt++; }
    }
    return resN("Backlog", cnt);
  }
  if (base == "delay") return res("Delay", "nur in Backlog");

  // --- Netz ---
  if (base == "websend") {
    if (!has) return res("WebSend", "URL fehlt");
    if (!wifiConnected()) return res("WebSend", "kein WLAN");
    return webSend(substVars(args, ""));
  }

  // --- GPIO ---
  if (base == "gpio") {
    if (!(idx == 2 || idx == 12 || idx == 13 || idx == 14 || idx == 15))
      return res("Gpio", "erlaubt: 2,12,13,14,15 (2/14/15 von SD-Karte belegt)");
    if (has) {
      pinMode(idx, OUTPUT);
      digitalWrite(idx, parseBoolArg(args, digitalRead(idx)) ? HIGH : LOW);
      fireEvent("Gpio" + String(idx) + "#State", String(digitalRead(idx)));
    }
    return resN("Gpio" + String(idx), digitalRead(idx));
  }

  // --- Plugins ---
  if (base == "plugins") return "{\"Plugins\":" + pluginsJson() + "}";
  if (base == "plugin") {
    String n = argWord(args, 0), v = argWord(args, 1);
    if (!n.length()) return "{\"Plugins\":" + pluginsJson() + "}";
    if (!v.length()) return res("Plugin", "Benutze: Plugin <name> <0|1>");
    if (!pluginSetEnabled(n, parseBoolArg(v, false))) return res("Plugin", "unbekannt: " + n);
    return resB("Plugin " + n, parseBoolArg(v, false));
  }

  String out;
  if (pluginsCommand(full, args, out)) return out;
  return res("Command", "Unbekannt. 'Help' zeigt alle Befehle");
}

String execCommand(const String& lineIn, bool logResult) {
  String line = lineIn;
  line.trim();
  if (!line.length()) return "{}";
  int sp = line.indexOf(' ');
  String name = sp < 0 ? line : line.substring(0, sp);
  String args = sp < 0 ? String("") : line.substring(sp + 1);
  args.trim();
  String lname = name;
  lname.toLowerCase();
  int i = lname.length();
  while (i > 0 && isDigit(lname[i - 1])) i--;
  String base = lname.substring(0, i);
  int idx = i < (int)lname.length() ? lname.substring(i).toInt() : 0;

  if (!lname.startsWith("_")) logf("> %s", line.c_str());
  xSemaphoreTakeRecursive(cmdMtx, portMAX_DELAY);
  String out = dispatch(base, idx, lname, args);
  xSemaphoreGiveRecursive(cmdMtx);
  if (logResult && !lname.startsWith("_")) logLine(out.length() > 400 ? out.substring(0, 400) + "..." : out);
  return out;
}
