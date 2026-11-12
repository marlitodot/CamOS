#pragma once
// ============================================================
//  CamOS Plugin-API
//  Siehe PLUGINS_AI.md fuer die vollstaendige Anleitung.
//
//  Ein Plugin ist EINE .cpp Datei in src/plugins/ :
//
//    #include "../plugin_api.h"
//    class MyPlugin : public Plugin {
//    public:
//      const char* name() override { return "myplugin"; }
//      void loop() override { ... }
//    };
//    REGISTER_PLUGIN(MyPlugin)
// ============================================================
#include <Arduino.h>
#include "esp_camera.h"

class Plugin {
public:
  virtual ~Plugin() {}

  // --- Pflicht ---
  virtual const char* name() = 0;                 // eindeutig, klein, max 12 Zeichen, keine Leerzeichen

  // --- Optional ---
  virtual const char* description() { return ""; }
  virtual bool defaultEnabled() { return true; }  // beim ersten Start aktiv?
  virtual void setup() {}                         // einmal beim Aktivieren
  virtual void loop() {}                          // staendig aus loop(), NICHT blockieren!
  virtual void stop() {}                          // beim Deaktivieren
  // Konsolen-Befehl. cmd ist kleingeschrieben (z.B. "timelapse"), args der Rest.
  // true zurueckgeben wenn behandelt; out = JSON-Antwort z.B. {"Timelapse":10}
  virtual bool command(const String& cmd, const String& args, String& out) { return false; }
  // Wird fuer jedes Event aufgerufen, z.B. ("Motion#Detected", "4.2")
  virtual void onEvent(const String& event, const String& value) {}
  // Status als JSON-Objekt-String, z.B. {"count":3}  ("" = kein Status)
  virtual String status() { return ""; }
  // HTTP unter /p/<name>/<path>. true = behandelt. contentType vorbelegt mit application/json
  virtual bool http(const String& path, const String& query, const String& body,
                    String& contentType, String& out) { return false; }

  // --- Persistente Einstellungen (NVS, Key max 15 Zeichen) ---
  String  getStr(const char* key, const String& def = "");
  int32_t getInt(const char* key, int32_t def = 0);
  void    setStr(const char* key, const String& v);
  void    setInt(const char* key, int32_t v);

  bool active = false;   // vom System verwaltet
};

struct PluginRegistrar { PluginRegistrar(Plugin* p); };
#define REGISTER_PLUGIN(CLS) \
  static CLS _plugin_inst_##CLS; \
  static PluginRegistrar _plugin_reg_##CLS(&_plugin_inst_##CLS);

// ------------------------------------------------------------
//  Funktionen des Systems, die Plugins benutzen duerfen
// ------------------------------------------------------------
namespace api {
  void   log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));  // in Konsole/Serial
  void   event(const String& name, const String& value = "");  // Event fuer Regeln ausloesen
  String exec(const String& command);                 // Befehl sofort ausfuehren -> JSON
  void   queue(const String& command, uint32_t delayMs = 0);   // Befehl spaeter ausfuehren
  void   flash(int level);                            // Blitz 0..255
  int    flash();
  bool   motionActive();
  float  motionLevel();                               // % geaenderte Pixel
  uint32_t motionCount();
  camera_fb_t* grab(bool useFlash = false);           // JPEG-Frame holen (IMMER release!)
  void   release(camera_fb_t* fb);
  bool   sdReady();
  String saveSnapshot(const char* prefix);            // -> Dateiname oder ""
  bool   wifiConnected();
  String ip();
  bool   timeValid();
  String time(const char* fmt = "%Y-%m-%d %H:%M:%S");
  String var(int n);                                  // Var1..Var8
  void   setVar(int n, const String& v);
  String jsonEscape(const String& s);
  String jstr(const String& s);                       // "escaped string" mit Anfuehrungszeichen
  String argWord(const String& args, int index);      // n-tes Wort (0-basiert)
  bool   parseBool(const String& s, bool current);    // on/off/1/0/toggle
}
