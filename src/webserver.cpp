// Webserver: Port 80 = UI + API, Port 81 = MJPEG-Stream
#include "core.h"
#include "esp_http_server.h"
#include "mbedtls/base64.h"
#include <Update.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <memory>
#include "web_ui.h"

static httpd_handle_t web = nullptr, strm = nullptr;
volatile int streamClients = 0;

// ---------------- Helfer ----------------
static bool authOk(httpd_req_t* req) {
  if (!cfg.webPass.length()) return true;
  char buf[200];
  size_t len = httpd_req_get_hdr_value_len(req, "Authorization");
  if (len > 0 && len < sizeof buf &&
      httpd_req_get_hdr_value_str(req, "Authorization", buf, sizeof buf) == ESP_OK &&
      strncmp(buf, "Basic ", 6) == 0) {
    unsigned char dec[160];
    size_t olen = 0;
    if (mbedtls_base64_decode(dec, sizeof(dec) - 1, &olen, (const unsigned char*)buf + 6, strlen(buf + 6)) == 0) {
      dec[olen] = 0;
      String expect = "admin:" + cfg.webPass;
      if (expect == (const char*)dec) return true;
    }
  }
  httpd_resp_set_status(req, "401 Unauthorized");
  httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"CamOS\"");
  httpd_resp_send(req, "Login erforderlich (Benutzer: admin)", HTTPD_RESP_USE_STRLEN);
  return false;
}
#define AUTH if (!authOk(req)) return ESP_OK;

static String urlDecode(const char* s) {
  String o;
  while (*s) {
    if (*s == '+') o += ' ';
    else if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) {
      char h[3] = {s[1], s[2], 0};
      o += (char)strtol(h, nullptr, 16);
      s += 2;
    } else o += *s;
    s++;
  }
  return o;
}

static String rawQuery(httpd_req_t* req) {
  size_t l = httpd_req_get_url_query_len(req);
  if (!l) return "";
  std::unique_ptr<char[]> q(new char[l + 1]);
  if (httpd_req_get_url_query_str(req, q.get(), l + 1) != ESP_OK) return "";
  return String(q.get());
}

static String qparam(httpd_req_t* req, const char* key) {
  String q = rawQuery(req);
  if (!q.length()) return "";
  char v[512];
  if (httpd_query_key_value(q.c_str(), key, v, sizeof v) == ESP_OK) return urlDecode(v);
  return "";
}

static String readBody(httpd_req_t* req, size_t maxLen = 8192) {
  size_t total = req->content_len;
  if (total > maxLen) total = maxLen;
  String s;
  s.reserve(total + 1);
  char buf[512];
  size_t got = 0;
  while (got < total) {
    size_t want = total - got;
    if (want > sizeof buf) want = sizeof buf;
    int r = httpd_req_recv(req, buf, want);
    if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
    if (r <= 0) break;
    for (int i = 0; i < r; i++) s += buf[i];
    got += r;
  }
  return s;
}

static esp_err_t sendStr(httpd_req_t* req, const String& s, const char* type) {
  httpd_resp_set_type(req, type);
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, s.c_str(), s.length());
}
static esp_err_t sendJson(httpd_req_t* req, const String& s) { return sendStr(req, s, "application/json"); }

// ---------------- Handler ----------------
static esp_err_t h_index(httpd_req_t* req) {
  AUTH;
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t h_status(httpd_req_t* req) { AUTH; return sendJson(req, statusJson()); }

static esp_err_t h_cmd(httpd_req_t* req) {
  AUTH;
  String c = req->method == HTTP_POST ? readBody(req, 4096) : String("");
  if (!c.length()) c = qparam(req, "c");
  return sendJson(req, execCommand(c, false));
}

static esp_err_t h_log(httpd_req_t* req) {
  AUTH;
  uint32_t from = strtoul(qparam(req, "from").c_str(), nullptr, 10), next = 0;
  String body = logGet(from, next);
  return sendStr(req, String(next) + "\n" + body, "text/plain; charset=utf-8");
}

static esp_err_t h_capture(httpd_req_t* req) {
  AUTH;
  bool fl = qparam(req, "flash") == "1";
  camera_fb_t* fb = cameraCapture(fl || cfg.snapFlash);
  if (!fb) { httpd_resp_send_500(req); return ESP_FAIL; }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t r = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return r;
}

// ---------------- SD-Dateimanager ----------------
static const char* mimeOf(const String& n) {
  String l = n;
  l.toLowerCase();
  if (l.endsWith(".jpg") || l.endsWith(".jpeg")) return "image/jpeg";
  if (l.endsWith(".png")) return "image/png";
  if (l.endsWith(".gif")) return "image/gif";
  if (l.endsWith(".bmp")) return "image/bmp";
  if (l.endsWith(".txt") || l.endsWith(".log") || l.endsWith(".csv")) return "text/plain; charset=utf-8";
  if (l.endsWith(".htm") || l.endsWith(".html")) return "text/html; charset=utf-8";
  if (l.endsWith(".json")) return "application/json";
  if (l.endsWith(".mp4")) return "video/mp4";
  if (l.endsWith(".avi")) return "video/x-msvideo";
  if (l.endsWith(".pdf")) return "application/pdf";
  return "application/octet-stream";
}

static esp_err_t sendFile(httpd_req_t* req, const String& path, bool inlineView) {
  File f = SD_MMC.open(path);
  if (!f || f.isDirectory()) { if (f) f.close(); httpd_resp_send_404(req); return ESP_OK; }
  String base = path.substring(path.lastIndexOf('/') + 1);
  base.replace("\"", "_");
  String disp = String(inlineView ? "inline" : "attachment") + "; filename=\"" + base + "\"";
  httpd_resp_set_type(req, mimeOf(base));
  httpd_resp_set_hdr(req, "Content-Disposition", disp.c_str());
  const size_t sz = 8192;
  std::unique_ptr<uint8_t[]> buf(new uint8_t[sz]);
  size_t n;
  while ((n = f.read(buf.get(), sz)) > 0) {
    if (httpd_resp_send_chunk(req, (const char*)buf.get(), n) != ESP_OK) { f.close(); return ESP_FAIL; }
  }
  f.close();
  return httpd_resp_send_chunk(req, nullptr, 0);
}

static esp_err_t h_fs(httpd_req_t* req) { AUTH; return sendJson(req, sdDirJson(qparam(req, "dir"))); }

static esp_err_t h_dl(httpd_req_t* req) {
  AUTH;
  String p = qparam(req, "f");
  if (!sdOk || !sdSafePath(p)) { httpd_resp_send_404(req); return ESP_OK; }
  return sendFile(req, p, qparam(req, "inline") == "1");
}

static esp_err_t h_upload(httpd_req_t* req) {
  AUTH;
  String p = qparam(req, "f");
  if (!sdOk) return sendJson(req, "{\"Upload\":\"keine SD-Karte\"}");
  if (!sdSafePath(p) || p.endsWith("/")) return sendJson(req, "{\"Upload\":\"ungueltiger Pfad\"}");
  size_t total = req->content_len;
  uint64_t freeB = SD_MMC.totalBytes() - SD_MMC.usedBytes();
  if (total + 32768 > freeB) return sendJson(req, "{\"Upload\":\"nicht genug Speicher\"}");
  File f = SD_MMC.open(p, FILE_WRITE);
  if (!f) return sendJson(req, "{\"Upload\":\"Datei kann nicht erstellt werden\"}");
  const size_t sz = 8192;
  std::unique_ptr<uint8_t[]> buf(new uint8_t[sz]);
  size_t got = 0;
  int timeouts = 0;
  bool ok = true;
  while (got < total) {
    size_t want = total - got;
    if (want > sz) want = sz;
    int r = httpd_req_recv(req, (char*)buf.get(), want);
    if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 20) continue;
    if (r <= 0 || f.write(buf.get(), r) != (size_t)r) { ok = false; break; }
    got += r;
  }
  f.close();
  if (!ok) { SD_MMC.remove(p); logf("SD: Upload abgebrochen %s", p.c_str()); return sendJson(req, "{\"Upload\":\"Fehler\"}"); }
  logf("SD: hochgeladen %s (%u KB)", p.c_str(), (unsigned)(total / 1024));
  fireEvent("SD#Uploaded", p);
  return sendJson(req, "{\"Upload\":\"OK\",\"File\":" + jstr(p) + "}");
}

static esp_err_t h_rm(httpd_req_t* req) {
  AUTH;
  String p = qparam(req, "f");
  bool ok = sdRemove(p);
  if (ok) logf("SD: geloescht %s", p.c_str());
  return sendJson(req, String("{\"Delete\":") + (ok ? "\"OK\"" : "\"Fehler\"") + "}");
}

static esp_err_t h_mkdir(httpd_req_t* req) {
  AUTH;
  String p = qparam(req, "d");
  bool ok = sdMkdir(p);
  return sendJson(req, String("{\"Mkdir\":") + (ok ? "\"OK\"" : "\"Fehler\"") + "}");
}

// Letztes Foto aus dem RAM (wenn keine SD-Karte) -> Download am PC
static esp_err_t h_lastphoto(httpd_req_t* req) {
  AUTH;
  const uint8_t* buf;
  size_t len;
  String name;
  if (!photoRamLock(&buf, &len, &name)) { httpd_resp_send_404(req); return ESP_OK; }
  String disp = "attachment; filename=\"" + name + "\"";
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", disp.c_str());
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t r = httpd_resp_send(req, (const char*)buf, len);
  photoRamUnlock();
  return r;
}

// /sd/<pfad> = Datei direkt anzeigen (z.B. /sd/cam/img_xxx.jpg)
static esp_err_t h_sdfile(httpd_req_t* req) {
  AUTH;
  String raw = req->uri + 3;   // ab "/" nach "/sd"
  int q = raw.indexOf('?');
  if (q >= 0) raw = raw.substring(0, q);
  String p = urlDecode(raw.c_str());
  if (!sdOk || !sdSafePath(p)) { httpd_resp_send_404(req); return ESP_OK; }
  return sendFile(req, p, true);
}

static esp_err_t h_rules(httpd_req_t* req) {
  AUTH;
  String j = "{\"rules\":[";
  for (int i = 1; i <= NUM_RULES; i++) {
    if (i > 1) j += ",";
    j += "{\"text\":" + jstr(ruleGet(i)) + ",\"on\":" + (ruleEnabled(i) ? "true" : "false") + "}";
  }
  j += "],\"vars\":[";
  for (int i = 1; i <= NUM_VARS; i++) { if (i > 1) j += ","; j += jstr(varGet(i)); }
  j += "],\"mems\":[";
  for (int i = 1; i <= NUM_VARS; i++) { if (i > 1) j += ","; j += jstr(memGet(i)); }
  j += "],\"timers\":[";
  for (int i = 1; i <= NUM_TIMERS; i++) { if (i > 1) j += ","; j += String(ruleTimerLeft(i)); }
  j += "]}";
  return sendJson(req, j);
}

static esp_err_t h_rule(httpd_req_t* req) {
  AUTH;
  int n = qparam(req, "n").toInt();
  String on = qparam(req, "on");
  String body = readBody(req, 2100);
  if (n < 1 || n > NUM_RULES) return sendJson(req, "{\"Error\":\"n=1..3\"}");
  ruleSet(n, body);
  if (on.length()) ruleEnable(n, on == "1");
  logf("Rule%d gespeichert (%s)", n, ruleEnabled(n) ? "ON" : "OFF");
  return sendJson(req, "{\"Rule" + String(n) + "\":\"" + (ruleEnabled(n) ? "ON" : "OFF") + "\",\"Saved\":true}");
}

static esp_err_t h_ota(httpd_req_t* req) {
  AUTH;
  size_t total = req->content_len;
  logf("OTA: Start, %u Bytes", (unsigned)total);
  if (total < 1024 || !Update.begin(total)) return sendJson(req, "{\"OTA\":\"Update.begin fehlgeschlagen\"}");
  std::unique_ptr<uint8_t[]> buf(new uint8_t[2048]);
  size_t got = 0;
  int timeouts = 0;
  while (got < total) {
    size_t want = total - got;
    if (want > 2048) want = 2048;
    int r = httpd_req_recv(req, (char*)buf.get(), want);
    if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 20) continue;
    if (r <= 0) { Update.abort(); logf("OTA: Abbruch"); return ESP_FAIL; }
    if (Update.write(buf.get(), r) != (size_t)r) { Update.abort(); return sendJson(req, "{\"OTA\":\"Schreibfehler\"}"); }
    got += r;
  }
  if (!Update.end(true)) {
    logf("OTA: Fehler %s", Update.errorString());
    return sendJson(req, "{\"OTA\":\"Fehler: " + String(Update.errorString()) + "\"}");
  }
  logf("OTA: erfolgreich, Neustart");
  restartAt = millis() + 1500;
  return sendJson(req, "{\"OTA\":\"OK - Neustart...\"}");
}

static esp_err_t h_scan(httpd_req_t* req) { AUTH; return sendJson(req, execCommand("WifiScan", false)); }
static esp_err_t h_plugins(httpd_req_t* req) { AUTH; return sendJson(req, pluginsJson()); }

static esp_err_t h_plugin(httpd_req_t* req) {
  AUTH;
  String path = req->uri + 3;   // nach "/p/"
  int q = path.indexOf('?');
  if (q >= 0) path = path.substring(0, q);
  String body = req->method == HTTP_POST ? readBody(req) : String("");
  String ctype = "application/json", out;
  if (!pluginsHttp(path, rawQuery(req), body, ctype, out)) { httpd_resp_send_404(req); return ESP_OK; }
  return sendStr(req, out, ctype.c_str());
}

// Unbekannte URL: Captive-Portal-Weiterleitung fuer AP-Clients
static esp_err_t h_404(httpd_req_t* req, httpd_err_code_t) {
  char host[64] = {0};
  httpd_req_get_hdr_value_str(req, "Host", host, sizeof host);
  String h = host;
  bool own = h == AP_IP_STR || h.startsWith(cfg.hostname) || (wifiConnected() && h == WiFi.localIP().toString());
  if (own) { httpd_resp_send_404(req); return ESP_OK; }
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "http://" AP_IP_STR "/");
  httpd_resp_send(req, "", 0);
  return ESP_OK;
}

// ---------------- Stream ----------------
#define BOUNDARY "camosframe"
static esp_err_t h_stream(httpd_req_t* req) {
  if (cfg.webPass.length() && qparam(req, "t") != streamToken && !authOk(req)) return ESP_OK;
  if (!cameraOk) { httpd_resp_send_500(req); return ESP_FAIL; }
  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=" BOUNDARY);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  streamClients++;
  char part[96];
  esp_err_t res = ESP_OK;
  while (res == ESP_OK) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }
    int hl = snprintf(part, sizeof part, "\r\n--" BOUNDARY "\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)fb->len);
    res = httpd_resp_send_chunk(req, part, hl);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
  streamClients--;
  return res;
}

static void reg(httpd_handle_t s, const char* uri, httpd_method_t m, esp_err_t (*fn)(httpd_req_t*)) {
  httpd_uri_t u = {};
  u.uri = uri;
  u.method = m;
  u.handler = fn;
  u.user_ctx = nullptr;
  httpd_register_uri_handler(s, &u);
}

void webStart() {
  httpd_config_t c = HTTPD_DEFAULT_CONFIG();
  c.server_port = 80;
  c.ctrl_port = 32768;
  c.max_uri_handlers = 24;
  c.max_open_sockets = 6;
  c.stack_size = 8192;
  c.lru_purge_enable = true;
  c.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&web, &c) == ESP_OK) {
    reg(web, "/", HTTP_GET, h_index);
    reg(web, "/api/status", HTTP_GET, h_status);
    reg(web, "/api/cmd", HTTP_POST, h_cmd);
    reg(web, "/api/cmd", HTTP_GET, h_cmd);
    reg(web, "/api/log", HTTP_GET, h_log);
    reg(web, "/capture", HTTP_GET, h_capture);
    reg(web, "/sd/*", HTTP_GET, h_sdfile);
    reg(web, "/api/fs", HTTP_GET, h_fs);
    reg(web, "/api/dl", HTTP_GET, h_dl);
    reg(web, "/api/upload", HTTP_POST, h_upload);
    reg(web, "/api/rm", HTTP_POST, h_rm);
    reg(web, "/api/mkdir", HTTP_POST, h_mkdir);
    reg(web, "/api/lastphoto", HTTP_GET, h_lastphoto);
    reg(web, "/api/rules", HTTP_GET, h_rules);
    reg(web, "/api/rule", HTTP_POST, h_rule);
    reg(web, "/api/ota", HTTP_POST, h_ota);
    reg(web, "/api/scan", HTTP_GET, h_scan);
    reg(web, "/api/plugins", HTTP_GET, h_plugins);
    reg(web, "/p/*", HTTP_GET, h_plugin);
    reg(web, "/p/*", HTTP_POST, h_plugin);
    httpd_register_err_handler(web, HTTPD_404_NOT_FOUND, h_404);
    logf("Web: Port 80 gestartet");
  } else logf("Web: Start fehlgeschlagen");

  httpd_config_t s = HTTPD_DEFAULT_CONFIG();
  s.server_port = 81;
  s.ctrl_port = 32769;
  s.max_uri_handlers = 2;
  s.max_open_sockets = 3;
  s.lru_purge_enable = true;
  if (httpd_start(&strm, &s) == ESP_OK) {
    reg(strm, "/stream", HTTP_GET, h_stream);
    logf("Stream: Port 81 gestartet");
  }
}
