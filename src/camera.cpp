#include "core.h"

bool cameraOk = false;
static int flashLevel = 0;

struct FsEntry { const char* n; framesize_t v; };
static const FsEntry fsTable[] = {
  {"QQVGA", FRAMESIZE_QQVGA}, {"QVGA", FRAMESIZE_QVGA}, {"CIF", FRAMESIZE_CIF},
  {"HVGA", FRAMESIZE_HVGA},   {"VGA", FRAMESIZE_VGA},   {"SVGA", FRAMESIZE_SVGA},
  {"XGA", FRAMESIZE_XGA},     {"HD", FRAMESIZE_HD},     {"SXGA", FRAMESIZE_SXGA},
  {"UXGA", FRAMESIZE_UXGA},
};

const char* framesizeName(int fs) {
  for (auto& e : fsTable) if (e.v == fs) return e.n;
  return "?";
}

int framesizeFromName(const String& s) {
  for (auto& e : fsTable) if (s.equalsIgnoreCase(e.n)) return e.v;
  return -1;
}

bool cameraInit() {
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;
  c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM;
  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;
  c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM;
  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;
  c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM;
  c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM;
  c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;
  c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  if (psramFound()) {
    c.frame_size = FRAMESIZE_UXGA;   // Puffer fuer max. Aufloesung reservieren
    c.jpeg_quality = 10;
    c.fb_count = 2;
    c.fb_location = CAMERA_FB_IN_PSRAM;
    c.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    c.frame_size = FRAMESIZE_SVGA;
    c.jpeg_quality = 12;
    c.fb_count = 1;
    c.fb_location = CAMERA_FB_IN_DRAM;
    c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    if (cfg.framesize > FRAMESIZE_SVGA) cfg.framesize = FRAMESIZE_SVGA;
  }
  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) {
    logf("Kamera: Init fehlgeschlagen (0x%x)", err);
    cameraOk = false;
    return false;
  }
  cameraOk = true;
  sensor_t* s = esp_camera_sensor_get();
  logf("Kamera: OK, Sensor PID 0x%x, PSRAM %s", s ? s->id.PID : 0, psramFound() ? "ja" : "nein");
  cameraApply();
  return true;
}

void cameraApply() {
  if (!cameraOk) return;
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return;
  s->set_framesize(s, (framesize_t)cfg.framesize);
  s->set_quality(s, cfg.quality);
  s->set_brightness(s, cfg.brightness);
  s->set_contrast(s, cfg.contrast);
  s->set_saturation(s, cfg.saturation);
  s->set_special_effect(s, cfg.effect);
  s->set_vflip(s, cfg.vflip ? 1 : 0);
  s->set_hmirror(s, cfg.hmirror ? 1 : 0);
  s->set_whitebal(s, cfg.awb ? 1 : 0);
}

camera_fb_t* cameraCapture(bool useFlash) {
  if (!cameraOk) return nullptr;
  int prev = flashGet();
  if (useFlash) {
    flashSet(cfg.flashSnapLevel);
    delay(120);
    // alte Frames verwerfen, damit die Belichtung mit Blitz stimmt
    for (int i = 0; i < 2; i++) {
      camera_fb_t* d = esp_camera_fb_get();
      if (d) esp_camera_fb_return(d);
    }
  }
  camera_fb_t* fb = esp_camera_fb_get();
  if (useFlash) flashSet(prev);
  return fb;
}

void flashInit() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(FLASH_LED_PIN, 5000, 8, FLASH_LEDC_CH);
  ledcWrite(FLASH_LED_PIN, 0);
#else
  ledcSetup(FLASH_LEDC_CH, 5000, 8);
  ledcAttachPin(FLASH_LED_PIN, FLASH_LEDC_CH);
  ledcWrite(FLASH_LEDC_CH, 0);
#endif
}

void flashSet(int level) {
  level = constrain(level, 0, 255);
  flashLevel = level;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(FLASH_LED_PIN, level);
#else
  ledcWrite(FLASH_LEDC_CH, level);
#endif
}

int flashGet() { return flashLevel; }
