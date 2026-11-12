// Taste (Standard: IO0-Taste des ESP32-CAM-MB Boards) + Foto auf SD oder in RAM (PC-Download)
//
// GPIO0 ist gleichzeitig der 20-MHz-Takt (XCLK) der Kamera. Wir schalten nur den
// Eingangspuffer zu und lesen den Pin: nicht gedrueckt = Takt wechselt staendig,
// gedrueckt = Pin dauerhaft LOW. Waehrend des Drueckens ist das Kamerabild gestoert,
// deshalb wird das Foto erst NACH dem Loslassen aufgenommen.
#include "core.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"

volatile bool buttonHeld = false;
volatile uint32_t photoSeq = 0;

static uint8_t* ramBuf = nullptr;
static size_t ramLen = 0, ramCap = 0;
static String ramName;
static SemaphoreHandle_t ramMtx = nullptr;

void buttonInit() {
  if (!ramMtx) ramMtx = xSemaphoreCreateMutex();
  int p = cfg.buttonPin;
  if (p == 0) PIN_INPUT_ENABLE(PERIPHS_IO_MUX_GPIO0_U);   // Ausgang (XCLK) bleibt aktiv
  else if (p > 0) pinMode(p, INPUT_PULLUP);
  if (p >= 0) logf("Taste: GPIO%d (kurz = Foto, lang = Button#Long)", p);
}

static bool rawPressed() {
  int p = cfg.buttonPin;
  if (p < 0) return false;
  if (p != 0) return digitalRead(p) == LOW;
  for (int i = 0; i < 160; i++) {
    if (REG_READ(GPIO_IN_REG) & 1) return false;   // HIGH gesehen -> nicht gedrueckt
    for (volatile int k = 0; k < (i & 7); k++) {}  // Jitter gegen Aliasing mit dem Takt
  }
  return true;
}

void buttonLoop() {
  static uint32_t last = 0, downAt = 0;
  static uint8_t cnt = 0;
  static bool down = false, longFired = false;
  if (millis() - last < 10) return;
  last = millis();
  bool r = rawPressed();
  if (r != down) {
    if (++cnt < 3) return;   // 30 ms entprellen
    cnt = 0;
    down = r;
    if (down) {
      downAt = millis();
      longFired = false;
      buttonHeld = cfg.buttonPin == 0;
    } else {
      uint32_t d = millis() - downAt;
      buttonHeld = false;
      if (!longFired) {
        fireEvent("Button#Short", String(d));
        if (cfg.buttonPhoto) queueCommand("Photo btn", 300);   // Kamera kurz erholen lassen
      }
    }
  } else cnt = 0;
  if (down && !longFired && millis() - downAt >= 1500) {
    longFired = true;
    fireEvent("Button#Long", String(millis() - downAt));
  }
}

String takePhoto(const char* prefix) {
  if (!cameraOk) return "";
  camera_fb_t* d = esp_camera_fb_get();   // evtl. gestoerten Frame verwerfen
  if (d) esp_camera_fb_return(d);

  if (sdOk) {
    String f = sdSaveSnapshot(prefix);
    if (f.length()) return "sd:" + f;
    logf("Foto: SD fehlgeschlagen, lege Foto zum PC-Download in den RAM");
  }

  camera_fb_t* fb = cameraCapture(cfg.snapFlash);
  if (!fb) { logf("Foto: kein Frame"); return ""; }
  String name = String("CamOS_") + prefix + "_" +
                (timeValid() ? timeStr("%Y%m%d_%H%M%S") : String(millis())) + ".jpg";
  bool ok = false;
  xSemaphoreTake(ramMtx, portMAX_DELAY);
  if (ramCap < fb->len) {
    free(ramBuf);
    size_t cap = fb->len + 16 * 1024;
    ramBuf = (uint8_t*)(psramFound() ? ps_malloc(cap) : malloc(cap));
    ramCap = ramBuf ? cap : 0;
  }
  if (ramBuf) {
    memcpy(ramBuf, fb->buf, fb->len);
    ramLen = fb->len;
    ramName = name;
    photoSeq++;
    ok = true;
  }
  xSemaphoreGive(ramMtx);
  esp_camera_fb_return(fb);
  if (!ok) { logf("Foto: kein Speicher"); return ""; }
  logf("Foto: %s (%u KB) bereit zum PC-Download", name.c_str(), (unsigned)(ramLen / 1024));
  fireEvent("Photo#Ready", name);
  return "ram:" + name;
}

bool photoRamLock(const uint8_t** buf, size_t* len, String* name) {
  if (!ramMtx) return false;
  xSemaphoreTake(ramMtx, portMAX_DELAY);
  if (!ramBuf || !ramLen) { xSemaphoreGive(ramMtx); return false; }
  *buf = ramBuf;
  *len = ramLen;
  *name = ramName;
  return true;
}

void photoRamUnlock() { xSemaphoreGive(ramMtx); }

String photoRamName() {
  if (!ramMtx) return "";
  xSemaphoreTake(ramMtx, portMAX_DELAY);
  String n = ramName;
  xSemaphoreGive(ramMtx);
  return n;
}
