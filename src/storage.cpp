// SD-Karte (1-Bit Modus, damit GPIO4/12/13 frei bleiben)
#include "core.h"
#include <SD_MMC.h>
#include <vector>

bool sdOk = false;

bool sdInit() {
  if (!SD_MMC.begin("/sdcard", true)) {
    logf("SD: keine Karte");
    return false;
  }
  if (SD_MMC.cardType() == CARD_NONE) {
    logf("SD: keine Karte");
    SD_MMC.end();
    return false;
  }
  if (!SD_MMC.exists("/cam")) SD_MMC.mkdir("/cam");
  sdOk = true;
  logf("SD: OK, %lu MB", (unsigned long)(SD_MMC.totalBytes() / (1024 * 1024)));
  return true;
}

String sdSaveSnapshot(const char* prefix) {
  if (!sdOk) return "";
  camera_fb_t* fb = cameraCapture(cfg.snapFlash);
  if (!fb) { logf("SD: kein Frame"); return ""; }
  String name = "/cam/" + String(prefix) + "_" +
                (timeValid() ? timeStr("%Y%m%d_%H%M%S") : String(millis())) + ".jpg";
  if (SD_MMC.exists(name)) name.replace(".jpg", "_" + String(millis() % 1000) + ".jpg");
  File f = SD_MMC.open(name, FILE_WRITE);
  bool ok = false;
  if (f) { ok = f.write(fb->buf, fb->len) == fb->len; f.close(); }
  esp_camera_fb_return(fb);
  if (!ok) { logf("SD: Schreiben fehlgeschlagen"); return ""; }
  String shortName = name.substring(5);
  logf("SD: gespeichert %s", shortName.c_str());
  fireEvent("SD#Saved", shortName);
  return shortName;
}

// ---------------- Dateimanager ----------------
bool sdSafePath(const String& p) {
  return p.startsWith("/") && p.indexOf("..") < 0 && p.indexOf('\\') < 0 && p.length() < 200;
}

static String baseName(const String& n) {
  int sl = n.lastIndexOf('/');
  return sl >= 0 ? n.substring(sl + 1) : n;
}

static String joinPath(const String& dir, const String& n) {
  return dir.endsWith("/") ? dir + n : dir + "/" + n;
}

String sdDirJson(const String& dirIn) {
  String dir = dirIn.length() ? dirIn : String("/");
  if (dir.length() > 1 && dir.endsWith("/")) dir.remove(dir.length() - 1);
  String j = "{\"ok\":";
  j += sdOk ? "true" : "false";
  if (!sdOk) return j + "}";
  // Groessen in KB, damit es in uint32 passt
  j += ",\"total\":" + String((uint32_t)(SD_MMC.totalBytes() / 1024));
  j += ",\"used\":" + String((uint32_t)(SD_MMC.usedBytes() / 1024));
  j += ",\"dir\":" + jstr(dir);
  if (!sdSafePath(dir)) return j + ",\"error\":\"ungueltiger Pfad\",\"entries\":[]}";
  File d = SD_MMC.open(dir);
  if (!d || !d.isDirectory()) return j + ",\"error\":\"Ordner nicht gefunden\",\"entries\":[]}";
  j += ",\"entries\":[";
  bool first = true;
  int n = 0;
  File f = d.openNextFile();
  while (f && n < 1000) {
    String nm = baseName(f.name());
    if (nm.length() && nm[0] != '.' && nm != "System Volume Information") {
      if (!first) j += ",";
      first = false;
      j += "{\"n\":" + jstr(nm) + ",\"s\":" + String((uint32_t)f.size()) + ",\"d\":" + (f.isDirectory() ? "true" : "false") + "}";
      n++;
    }
    f.close();
    f = d.openNextFile();
  }
  d.close();
  return j + "]}";
}

static bool rmRec(const String& p, int depth) {
  File f = SD_MMC.open(p);
  if (!f) return false;
  if (!f.isDirectory()) { f.close(); return SD_MMC.remove(p); }
  if (depth > 8) { f.close(); return false; }
  std::vector<String> kids;
  File c = f.openNextFile();
  while (c) {
    kids.push_back(joinPath(p, baseName(c.name())));
    c.close();
    c = f.openNextFile();
  }
  f.close();
  for (auto& k : kids) rmRec(k, depth + 1);
  return SD_MMC.rmdir(p);
}

bool sdRemove(const String& path) {
  if (!sdOk || !sdSafePath(path) || path == "/") return false;
  return rmRec(path, 0);
}

bool sdMkdir(const String& path) {
  if (!sdOk || !sdSafePath(path) || path == "/") return false;
  return SD_MMC.mkdir(path);
}
