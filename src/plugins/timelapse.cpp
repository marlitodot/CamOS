// Beispiel-Plugin: Zeitraffer - speichert alle N Sekunden ein Foto auf der SD-Karte.
#include "../plugin_api.h"

class TimelapsePlugin : public Plugin {
  int interval = 0;      // Sekunden, 0 = aus
  uint32_t last = 0;
  uint32_t count = 0;

public:
  const char* name() override { return "timelapse"; }
  const char* description() override { return "Zeitraffer: Foto alle N Sekunden auf SD (Befehl: Timelapse <s>)"; }

  void setup() override { interval = getInt("interval", 0); last = millis(); }

  void loop() override {
    if (interval <= 0 || !api::sdReady()) return;
    if (millis() - last < (uint32_t)interval * 1000UL) return;
    last = millis();
    String f = api::saveSnapshot("tl");
    if (f.length()) { count++; api::event("Timelapse#Saved", f); }
  }

  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd != "timelapse") return false;
    if (args.length()) { interval = max(0, (int)args.toInt()); setInt("interval", interval); last = millis(); }
    out = "{\"Timelapse\":" + String(interval) + "}";
    return true;
  }

  String status() override {
    return "{\"interval\":" + String(interval) + ",\"saved\":" + String(count) + "}";
  }

  // GET /p/timelapse/info
  bool http(const String& path, const String& query, const String& body, String& ctype, String& out) override {
    if (path != "info") return false;
    out = status();
    return true;
  }
};
REGISTER_PLUGIN(TimelapsePlugin)
