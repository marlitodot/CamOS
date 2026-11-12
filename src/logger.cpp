#include "core.h"
#include <time.h>

static String logBuf[LOG_LINES];
static uint32_t logHead = 0;
static SemaphoreHandle_t logMtx = nullptr;

void logInit() {
  if (!logMtx) logMtx = xSemaphoreCreateMutex();
}

void logLine(const String& s) {
  char ts[16];
  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm t;
    localtime_r(&now, &t);
    strftime(ts, sizeof ts, "%H:%M:%S", &t);
  } else {
    snprintf(ts, sizeof ts, "+%lus", (unsigned long)(millis() / 1000));
  }
  String l = String(ts) + " " + s;
  if (l.length() > 600) l = l.substring(0, 600) + "...";
  Serial.println(l);
  if (!logMtx) return;
  xSemaphoreTake(logMtx, portMAX_DELAY);
  logBuf[logHead % LOG_LINES] = l;
  logHead++;
  xSemaphoreGive(logMtx);
}

void logf(const char* fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  logLine(String(buf));
}

String logGet(uint32_t from, uint32_t& next) {
  String o;
  if (!logMtx) { next = 0; return o; }
  xSemaphoreTake(logMtx, portMAX_DELAY);
  uint32_t oldest = logHead > LOG_LINES ? logHead - LOG_LINES : 0;
  uint32_t start = from;
  if (start > logHead) start = oldest;   // Geraet wurde neu gestartet
  if (start < oldest) start = oldest;
  for (uint32_t i = start; i < logHead; i++) {
    o += logBuf[i % LOG_LINES];
    o += '\n';
  }
  next = logHead;
  xSemaphoreGive(logMtx);
  return o;
}
