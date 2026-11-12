// Beispiel-Plugin: ruft bei Bewegung eine URL auf (z.B. ntfy.sh, Home Assistant Webhook).
// Platzhalter in der URL werden ersetzt (von WebSend): %device%, %time%, %ip%, %motion% ...
#include "../plugin_api.h"

class WebhookPlugin : public Plugin {
  String url;
  uint32_t sent = 0;

public:
  const char* name() override { return "webhook"; }
  const char* description() override { return "HTTP-Aufruf bei Bewegung (Befehl: WebhookUrl <url>, Webhook test)"; }

  void setup() override { url = getStr("url", ""); }

  void onEvent(const String& event, const String& value) override {
    if (event != "Motion#Detected" || !url.length()) return;
    api::queue("WebSend " + url);   // laeuft asynchron in der Hauptschleife
    sent++;
  }

  bool command(const String& cmd, const String& args, String& out) override {
    if (cmd == "webhookurl") {
      if (args.length()) { url = args == "\"\"" ? String("") : args; setStr("url", url); }
      out = "{\"WebhookUrl\":" + api::jstr(url) + "}";
      return true;
    }
    if (cmd == "webhook") {
      if (!url.length()) { out = "{\"Webhook\":\"keine URL\"}"; return true; }
      out = api::exec("WebSend " + url);
      return true;
    }
    return false;
  }

  String status() override { return "{\"url\":" + api::jstr(url) + ",\"sent\":" + String(sent) + "}"; }
};
REGISTER_PLUGIN(WebhookPlugin)
