// Bewegungserkennung: JPEG wird auf 1/8 Groesse dekodiert, in Graustufen
// umgerechnet und mit dem vorherigen Bild verglichen.
#include "core.h"
#include "img_converters.h"

volatile int  motionLevel = 0;
volatile bool motionActive = false;
volatile uint32_t motionCount = 0;
volatile uint32_t motionLastMs = 0;

static uint8_t* rgb = nullptr;
static uint8_t* prevY = nullptr;
static int bw = 0, bh = 0;
static bool baseline = true;

static void* bigAlloc(size_t n) {
  void* p = psramFound() ? ps_malloc(n) : nullptr;
  return p ? p : malloc(n);
}

static void motionTask(void*) {
  int lastFlash = -1;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(max(50, cfg.motionInterval)));
    if (!cfg.motionEnabled || !cameraOk) {
      if (motionActive) { motionActive = false; fireEvent("Motion#End", String(motionCount)); }
      motionLevel = 0;
      baseline = true;
      continue;
    }
    // Blitz-Aenderung wuerde als Bewegung erkannt -> neu kalibrieren
    if (flashGet() != lastFlash) { lastFlash = flashGet(); baseline = true; }
    // IO0-Taste gedrueckt -> Kamera-Takt gestoert, Bilder ignorieren
    if (buttonHeld) { baseline = true; continue; }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) continue;
    if (fb->format != PIXFORMAT_JPEG) { esp_camera_fb_return(fb); continue; }
    int w = fb->width / 8, h = fb->height / 8;
    if (w != bw || h != bh || !rgb || !prevY) {
      free(rgb); free(prevY);
      rgb = (uint8_t*)bigAlloc((size_t)(w + 2) * (h + 2) * 2);
      prevY = (uint8_t*)bigAlloc((size_t)w * h);
      bw = w; bh = h;
      baseline = true;
      if (!rgb || !prevY) { esp_camera_fb_return(fb); logf("Motion: kein Speicher"); vTaskDelay(5000); continue; }
    }
    bool ok = jpg2rgb565(fb->buf, fb->len, rgb, JPG_SCALE_8X);
    esp_camera_fb_return(fb);
    if (!ok) continue;

    int total = w * h, changed = 0;
    int thr = cfg.motionThreshold;
    for (int i = 0; i < total; i++) {
      uint16_t c = ((uint16_t)rgb[2 * i] << 8) | rgb[2 * i + 1];
      int r = (c >> 11) << 3, g = ((c >> 5) & 0x3F) << 2, b = (c & 0x1F) << 3;
      uint8_t y = (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
      if (!baseline && abs((int)y - (int)prevY[i]) > thr) changed++;
      prevY[i] = y;
    }
    if (baseline) { baseline = false; continue; }

    int lvl = total ? changed * 1000 / total : 0;
    motionLevel = lvl;
    uint32_t now = millis();
    if (lvl >= cfg.motionSensitivity) {
      motionLastMs = now;
      if (!motionActive) {
        motionActive = true;
        motionCount++;
        fireEvent("Motion#Detected", String(lvl / 10.0f, 1));
      }
    } else if (motionActive && now - motionLastMs > (uint32_t)cfg.motionCooldown * 1000UL) {
      motionActive = false;
      fireEvent("Motion#End", String(motionCount));
    }
  }
}

void motionStart() {
  xTaskCreatePinnedToCore(motionTask, "motion", 8192, nullptr, 1, nullptr, 1);
}
