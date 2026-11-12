// Beispiel-Plugin: PIR-Bewegungsmelder an einem GPIO -> Event PIR#State=1/0
// Standard: GPIO13, standardmaessig AUS (sonst Fehlalarme bei offenem Pin).
#include "../plugin_api.h"

class PirPlugin : public Plugin {
  int pin = 13;
  int state = -1;
  uint32_t changedAt = 0;

public:
  const char* name() override { return "pir"; }
  const char* description() override { return "PIR-Sensor an GPIO (Standard 13) -> Event PIR#State (Befehl: PirPin <n>)"; }
  bool defaultEnabled() override { return false; }

  void setup() override {
    pin = getInt("pin", 13);
    pinMode(pin, INPUT_PULLDOWN);
    state = -1;
  }

  void loop() override {
    int v = digitalRead(pin);
    if (v == state) return;
    if (millis() - changedAt < 50) return;   // Entprellen
    changedAt = millis();
    state = v;
    api::event("PIR#State", String(v));
  }

  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd != "pirpin") return false;
    if (args.length()) {
      int p = args.toInt();
      if (p == 12 || p == 13 || p == 2 || p == 14 || p == 15) { pin = p; setInt("pin", pin); setup(); }
    }
    out = "{\"PirPin\":" + String(pin) + "}";
    return true;
  }

  String status() override { return "{\"pin\":" + String(pin) + ",\"state\":" + String(state) + "}"; }
};
REGISTER_PLUGIN(PirPlugin)
