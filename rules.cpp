// Regel-Engine im Tasmota-Stil:
//   ON <Trigger>[<op><Wert>] DO <Befehl> ENDON  ON ... ENDON
// Events werden ueber eine Queue an die Hauptschleife geliefert.
#include "core.h"
#include <Preferences.h>
#include <WiFi.h>
#include <vector>

static String rules[NUM_RULES];
static bool rulesOn[NUM_RULES];
static String vars[NUM_VARS], mems[NUM_VARS];
static uint32_t timerDue[NUM_TIMERS];
static bool timerOn[NUM_TIMERS];

struct QItem { String cmd; uint32_t due; };
static std::vector<QItem> cmdq;
static SemaphoreHandle_t qMtx;

struct Ev { char name[40]; char value[80]; };
static QueueHandle_t evq;

void rulesInit() {
  qMtx = xSemaphoreCreateMutex();
  evq = xQueueCreate(24, sizeof(Ev));
  Preferences p;
  p.begin("rules", false);
  for (int i = 0; i < NUM_RULES; i++) {
    rules[i] = p.getString(("r" + String(i + 1)).c_str(), "");
    rulesOn[i] = p.getBool(("e" + String(i + 1)).c_str(), false);
  }
  for (int i = 0; i < NUM_VARS; i++) mems[i] = p.getString(("m" + String(i + 1)).c_str(), "");
  p.end();
}

static void saveRule(int i) {
  Preferences p;
  p.begin("rules", false);
  p.putString(("r" + String(i + 1)).c_str(), rules[i]);
  p.putBool(("e" + String(i + 1)).c_str(), rulesOn[i]);
  p.end();
}

String ruleGet(int n) { return (n >= 1 && n <= NUM_RULES) ? rules[n - 1] : String(""); }
void ruleSet(int n, const String& t) {
  if (n < 1 || n > NUM_RULES) return;
  rules[n - 1] = t.length() > 2000 ? t.substring(0, 2000) : t;
  saveRule(n - 1);
}
bool ruleEnabled(int n) { return (n >= 1 && n <= NUM_RULES) ? rulesOn[n - 1] : false; }
void ruleEnable(int n, bool on) {
  if (n < 1 || n > NUM_RULES) return;
  rulesOn[n - 1] = on;
  saveRule(n - 1);
}
String varGet(int n) { return (n >= 1 && n <= NUM_VARS) ? vars[n - 1] : String(""); }
void varSet(int n, const String& v) { if (n >= 1 && n <= NUM_VARS) vars[n - 1] = v; }
String memGet(int n) { return (n >= 1 && n <= NUM_VARS) ? mems[n - 1] : String(""); }
void memSet(int n, const String& v) {
  if (n < 1 || n > NUM_VARS) return;
  mems[n - 1] = v;
  Preferences p;
  p.begin("rules", false);
  p.putString(("m" + String(n)).c_str(), v);
  p.end();
}

void ruleTimerSet(int n, uint32_t sec) {
  if (n < 1 || n > NUM_TIMERS) return;
  timerOn[n - 1] = sec > 0;
  timerDue[n - 1] = millis() + sec * 1000UL;
}
uint32_t ruleTimerLeft(int n) {
  if (n < 1 || n > NUM_TIMERS || !timerOn[n - 1]) return 0;
  int32_t left = (int32_t)(timerDue[n - 1] - millis());
  return left > 0 ? (left + 999) / 1000 : 0;
}

void fireEvent(const String& name, const String& value) {
  if (!evq) return;
  Ev e;
  strlcpy(e.name, name.c_str(), sizeof e.name);
  strlcpy(e.value, value.c_str(), sizeof e.value);
  if (xQueueSend(evq, &e, 0) != pdTRUE) logf("Event-Queue voll, verworfen: %s", e.name);
}

void queueCommand(const String& cmd, uint32_t delayMs) {
  xSemaphoreTake(qMtx, portMAX_DELAY);
  if (cmdq.size() < 64) cmdq.push_back({cmd, millis() + delayMs});
  xSemaphoreGive(qMtx);
}

// ---------------- Variablen-Ersetzung ----------------
static void replaceCI(String& s, const char* key, const String& v) {
  String k = key;
  int from = 0;
  while (true) {
    String low = s;
    low.toLowerCase();
    int p = low.indexOf(k, from);
    if (p < 0) break;
    s = s.substring(0, p) + v + s.substring(p + k.length());
    from = p + v.length();
  }
}

String substVars(const String& in, const String& value) {
  if (in.indexOf('%') < 0) return in;
  String s = in;
  replaceCI(s, "%value%", value);
  for (int i = 1; i <= NUM_VARS; i++) {
    replaceCI(s, ("%var" + String(i) + "%").c_str(), vars[i - 1]);
    replaceCI(s, ("%mem" + String(i) + "%").c_str(), mems[i - 1]);
  }
  if (s.indexOf('%') < 0) return s;
  replaceCI(s, "%time%", timeValid() ? timeStr("%H:%M:%S") : String(""));
  replaceCI(s, "%date%", timeValid() ? timeStr("%Y-%m-%d") : String(""));
  replaceCI(s, "%timestamp%", timeValid() ? timeStr("%Y%m%d_%H%M%S") : String(millis()));
  replaceCI(s, "%uptime%", String(millis() / 1000));
  replaceCI(s, "%motioncount%", String(motionCount));
  replaceCI(s, "%motion%", String(motionLevel / 10.0f, 1));
  replaceCI(s, "%flash%", String(flashGet()));
  replaceCI(s, "%ip%", wifiConnected() ? WiFi.localIP().toString() : String(AP_IP_STR));
  replaceCI(s, "%device%", deviceId);
  replaceCI(s, "%hostname%", cfg.hostname);
  return s;
}

// ---------------- Trigger-Auswertung ----------------
static bool isNum(const String& s) {
  if (!s.length()) return false;
  char* end;
  strtod(s.c_str(), &end);
  while (*end == ' ') end++;
  return *end == 0;
}

static bool matchTrigger(String trig, const String& name, const String& value) {
  trig.trim();
  int opPos = -1;
  for (int i = 0; i < (int)trig.length(); i++) {
    char c = trig[i];
    if (c == '=' || c == '<' || c == '>' || c == '!') { opPos = i; break; }
  }
  String tname = opPos < 0 ? trig : trig.substring(0, opPos);
  tname.trim();
  if (!tname.equalsIgnoreCase(name)) {
    if (!tname.endsWith("*")) return false;            // Wildcard: Event#*
    String pre = tname.substring(0, tname.length() - 1);
    if (!name.substring(0, pre.length()).equalsIgnoreCase(pre)) return false;
  }
  if (opPos < 0) return true;
  String op = trig.substring(opPos, opPos + 1);
  if (opPos + 1 < (int)trig.length() && trig[opPos + 1] == '=') op += '=';
  String rhs = substVars(trig.substring(opPos + op.length()), value);
  rhs.trim();
  if (isNum(rhs) && isNum(value)) {
    double a = value.toDouble(), b = rhs.toDouble();
    if (op == "=" || op == "==") return a == b;
    if (op == "!=") return a != b;
    if (op == ">")  return a > b;
    if (op == "<")  return a < b;
    if (op == ">=") return a >= b;
    if (op == "<=") return a <= b;
    return false;
  }
  if (op == "=" || op == "==") return value.equalsIgnoreCase(rhs);
  if (op == "!=") return !value.equalsIgnoreCase(rhs);
  return false;
}

static void runRuleCommand(String c, const String& value) {
  c = substVars(c, value);
  c.trim();
  if (!c.length()) return;
  String low = c;
  low.toLowerCase();
  if (c.indexOf(';') >= 0 && !low.startsWith("backlog")) c = "Backlog " + c;
  execCommand(c);
}

static void evalRule(const String& text, const String& name, const String& value) {
  String orig = " " + text + " ";
  orig.replace('\n', ' ');
  orig.replace('\r', ' ');
  orig.replace('\t', ' ');
  String up = orig;
  up.toUpperCase();
  int pos = 0;
  while (true) {
    int on = up.indexOf(" ON ", pos);
    if (on < 0) break;
    int d = up.indexOf(" DO ", on + 3);
    if (d < 0) break;
    int e = up.indexOf(" ENDON", d + 3);
    int end = e < 0 ? up.length() : e;
    String trig = orig.substring(on + 4, d);
    String cmds = orig.substring(d + 4, end);
    pos = e < 0 ? up.length() : e + 6;
    if (matchTrigger(trig, name, value)) runRuleCommand(cmds, value);
  }
}

static void handleEvent(const String& name, const String& value) {
  if (!name.equalsIgnoreCase("Time#Minute")) logf("EVENT %s=%s", name.c_str(), value.c_str());
  if (name == "Motion#Detected" && cfg.motionSave && sdOk) sdSaveSnapshot("motion");
  pluginsEvent(name, value);
  for (int i = 0; i < NUM_RULES; i++)
    if (rulesOn[i] && rules[i].length()) evalRule(rules[i], name, value);
}

void rulesLoop() {
  Ev ev;
  int n = 0;
  while (n++ < 10 && xQueueReceive(evq, &ev, 0) == pdTRUE) handleEvent(String(ev.name), String(ev.value));

  uint32_t now = millis();
  for (int i = 0; i < NUM_TIMERS; i++) {
    if (timerOn[i] && (int32_t)(now - timerDue[i]) >= 0) {
      timerOn[i] = false;
      fireEvent("Rules#Timer", String(i + 1));
    }
  }

  std::vector<String> due;
  xSemaphoreTake(qMtx, portMAX_DELAY);
  for (auto it = cmdq.begin(); it != cmdq.end();) {
    if ((int32_t)(now - it->due) >= 0) { due.push_back(it->cmd); it = cmdq.erase(it); }
    else ++it;
  }
  xSemaphoreGive(qMtx);
  for (auto& c : due) execCommand(c);
}
