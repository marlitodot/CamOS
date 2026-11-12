// Beispiel-Plugin: rote Status-LED als Lebenszeichen, blinkt schnell bei Bewegung.
#include "../plugin_api.h"
#include "../config.h"

class HeartbeatPlugin : public Plugin {
  int period = 2000;
  bool ledOn = false;

public:
  const char* name() override { return "heartbeat"; }
  const char* description() override { return "Rote LED (GPIO33) blinkt als Lebenszeichen, schnell bei Bewegung"; }

  void setup() override {
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);
    period = getInt("period", 2000);
  }
  void stop() override { digitalWrite(STATUS_LED_PIN, HIGH); }

  void loop() override {
    if (period <= 0) return;
    uint32_t p = api::motionActive() ? 200 : (uint32_t)period;
    bool want = (millis() % p) < 50;
    if (want != ledOn) {
      ledOn = want;
      digitalWrite(STATUS_LED_PIN, ledOn ? LOW : HIGH);
    }
  }

  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd != "heartbeat") return false;
    if (args.length()) {
      period = args.toInt();
      setInt("period", period);
      if (period <= 0) digitalWrite(STATUS_LED_PIN, HIGH);
    }
    out = "{\"Heartbeat\":" + String(period) + "}";
    return true;
  }

  String status() override { return "{\"period\":" + String(period) + "}"; }
};
REGISTER_PLUGIN(HeartbeatPlugin)
