// Plugin-Verwaltung + Implementierung der api:: Funktionen
#include "core.h"
#include "plugin_api.h"
#include <Preferences.h>
#include <WiFi.h>
#include <vector>

static std::vector<Plugin*>& plist() {
  static std::vector<Plugin*> v;
  return v;
}

PluginRegistrar::PluginRegistrar(Plugin* p) { plist().push_back(p); }

static String key15(const String& s) { return s.length() > 15 ? s.substring(0, 15) : s; }
static String nsOf(Plugin* p) { return key15(String("p_") + p->name()); }

static bool enabledPref(Plugin* p) {
  Preferences pr;
  pr.begin("plugins", false);
  bool e = pr.getBool(key15(p->name()).c_str(), p->defaultEnabled());
  pr.end();
  return e;
}

static void activate(Plugin* p) {
  if (p->active) return;
  p->setup();
  p->active = true;
  logf("Plugin aktiv: %s", p->name());
}

static void deactivate(Plugin* p) {
  if (!p->active) return;
  p->stop();
  p->active = false;
  logf("Plugin gestoppt: %s", p->name());
}

void pluginsSetup() {
  for (auto p : plist()) if (enabledPref(p)) activate(p);
  logf("Plugins: %u registriert", (unsigned)plist().size());
}

void pluginsLoop() {
  for (auto p : plist()) if (p->active) p->loop();
}

bool pluginsCommand(const String& cmd, const String& args, String& out) {
  for (auto p : plist())
    if (p->active && p->command(cmd, args, out)) return true;
  return false;
}

void pluginsEvent(const String& name, const String& value) {
  for (auto p : plist()) if (p->active) p->onEvent(name, value);
}

String pluginsJson() {
  String j = "[";
  bool first = true;
  for (auto p : plist()) {
    if (!first) j += ",";
    first = false;
    String st = p->active ? p->status() : String("");
    j += "{\"name\":" + jstr(p->name()) + ",\"desc\":" + jstr(p->description()) +
         ",\"enabled\":" + (enabledPref(p) ? "true" : "false") +
         ",\"active\":" + (p->active ? "true" : "false") +
         ",\"status\":" + (st.length() ? st : String("null")) + "}";
  }
  return j + "]";
}

bool pluginsHttp(const String& path, const String& query, const String& body, String& ctype, String& out) {
  int sl = path.indexOf('/');
  String n = sl < 0 ? path : path.substring(0, sl);
  String sub = sl < 0 ? String("") : path.substring(sl + 1);
  for (auto p : plist())
    if (p->active && n.equalsIgnoreCase(p->name())) return p->http(sub, query, body, ctype, out);
  return false;
}

bool pluginSetEnabled(const String& name, bool on) {
  for (auto p : plist()) {
    if (!name.equalsIgnoreCase(p->name())) continue;
    Preferences pr;
    pr.begin("plugins", false);
    pr.putBool(key15(p->name()).c_str(), on);
    pr.end();
    if (on) activate(p); else deactivate(p);
    return true;
  }
  return false;
}

// ---------------- Plugin Settings ----------------
String Plugin::getStr(const char* key, const String& def) {
  Preferences pr;
  pr.begin(nsOf(this).c_str(), false);
  String v = pr.getString(key, def);
  pr.end();
  return v;
}
int32_t Plugin::getInt(const char* key, int32_t def) {
  Preferences pr;
  pr.begin(nsOf(this).c_str(), false);
  int32_t v = pr.getInt(key, def);
  pr.end();
  return v;
}
void Plugin::setStr(const char* key, const String& v) {
  Preferences pr;
  pr.begin(nsOf(this).c_str(), false);
  pr.putString(key, v);
  pr.end();
}
void Plugin::setInt(const char* key, int32_t v) {
  Preferences pr;
  pr.begin(nsOf(this).c_str(), false);
  pr.putInt(key, v);
  pr.end();
}

// ---------------- api:: ----------------
namespace api {
void log(const char* fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  logLine(String(buf));
}
void event(const String& name, const String& value) { fireEvent(name, value); }
String exec(const String& command) { return execCommand(command); }
void queue(const String& command, uint32_t delayMs) { queueCommand(command, delayMs); }
void flash(int level) { flashSet(level); }
int flash() { return flashGet(); }
bool motionActive() { return ::motionActive; }
float motionLevel() { return ::motionLevel / 10.0f; }
uint32_t motionCount() { return ::motionCount; }
camera_fb_t* grab(bool useFlash) { return cameraCapture(useFlash); }
void release(camera_fb_t* fb) { if (fb) esp_camera_fb_return(fb); }
bool sdReady() { return sdOk; }
String saveSnapshot(const char* prefix) { return sdSaveSnapshot(prefix); }
bool wifiConnected() { return ::wifiConnected(); }
String ip() { return ::wifiConnected() ? WiFi.localIP().toString() : String(AP_IP_STR); }
bool timeValid() { return ::timeValid(); }
String time(const char* fmt) { return timeStr(fmt); }
String var(int n) { return varGet(n); }
void setVar(int n, const String& v) { varSet(n, v); }
String jsonEscape(const String& s) { return ::jsonEscape(s); }
String jstr(const String& s) { return ::jstr(s); }
String argWord(const String& args, int index) { return ::argWord(args, index); }
bool parseBool(const String& s, bool current) { return parseBoolArg(s, current); }
}  // namespace api
