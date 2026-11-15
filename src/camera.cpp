#include "core.h"
#include <Preferences.h>

bool cameraOk = false;
static int flashLevel = 0;

struct FsEntry { const char* n; framesize_t v; };
static const FsEntry fsTable[] = {
  {"QQVGA", FRAMESIZE_QQVGA}, {"QVGA", FRAMESIZE_QVGA}, {"CIF", FRAMESIZE_CIF},
  {"HVGA", FRAMESIZE_HVGA},   {"VGA", FRAMESIZE_VGA},   {"SVGA", FRAMESIZE_SVGA},
  {"XGA", FRAMESIZE_XGA},     {"HD", FRAMESIZE_HD},     {"SXGA", FRAMESIZE_SXGA},
  {"UXGA", FRAMESIZE_UXGA},   {"FHD", FRAMESIZE_FHD},   {"QXGA", FRAMESIZE_QXGA},
  {"QSXGA", FRAMESIZE_QSXGA},
};

// Erkannter Sensor (OV2640, OV3660, OV5640 ...)
static uint16_t sensorPid = 0;
static String sensorNm = "-";
static int sensorMax = FRAMESIZE_UXGA;

const char* cameraSensorName() { return sensorNm.c_str(); }
int cameraMaxFramesize() { return sensorMax; }

const char* framesizeName(int fs) {
  for (auto& e : fsTable) if (e.v == fs) return e.n;
  return "?";
}

int framesizeFromName(const String& s) {
  for (auto& e : fsTable) if (s.equalsIgnoreCase(e.n)) return e.v;
  return -1;
}

static esp_err_t cameraStart(framesize_t bufferSize) {
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
  c.frame_size = bufferSize;   // bestimmt die Puffergroesse = max. moegliche Aufloesung
  if (psramFound()) {
    c.jpeg_quality = 10;
    c.fb_count = 2;
    c.fb_location = CAMERA_FB_IN_PSRAM;
    c.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    c.jpeg_quality = 12;
    c.fb_count = 1;
    c.fb_location = CAMERA_FB_IN_DRAM;
    c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }
  return esp_camera_init(&c);
}

// Sensor-spezifische Standardwerte, nur wenn ein ANDERER Sensor als beim letzten Start erkannt wird
static void sensorDefaults() {
  Preferences p;
  p.begin("cfg", false);
  uint16_t last = p.getUShort("sensor", 0);
  if (last != sensorPid) {
    if (sensorPid == OV3660_PID) {
      // OV3660 ist auf den Modulen kopfueber verbaut und etwas dunkler/gesaettigter
      cfg.vflip = true;
      cfg.hmirror = false;
      cfg.brightness = 1;
      cfg.saturation = -2;
    } else {
      cfg.vflip = false;
      cfg.hmirror = false;
      cfg.brightness = 0;
      cfg.saturation = 0;
    }
    p.putUShort("sensor", sensorPid);
    p.end();
    settingsSave();
    if (last) logf("Kamera: neuer Sensor erkannt, Standardwerte fuer %s gesetzt", sensorNm.c_str());
    return;
  }
  p.end();
}

bool cameraInit() {
  bool psram = psramFound();
  esp_err_t err = cameraStart(psram ? FRAMESIZE_UXGA : FRAMESIZE_SVGA);
  if (err != ESP_OK) {
    logf("Kamera: Init fehlgeschlagen (0x%x)", err);
    cameraOk = false;
    return false;
  }
  sensor_t* s = esp_camera_sensor_get();
  sensorPid = s ? s->id.PID : 0;
  camera_sensor_info_t* info = s ? esp_camera_sensor_get_info(&s->id) : nullptr;
  if (info) {
    sensorNm = info->name;
    sensorMax = info->max_size;
  } else {
    sensorNm = "PID 0x" + String(sensorPid, HEX);
    sensorMax = FRAMESIZE_UXGA;
  }

  if (!psram) {
    if (sensorMax > FRAMESIZE_SVGA) sensorMax = FRAMESIZE_SVGA;
  } else if (sensorMax > FRAMESIZE_UXGA) {
    // OV3660 (QXGA 3 MP) / OV5640 (QSXGA 5 MP): mit groesseren Puffern neu starten
    esp_camera_deinit();
    delay(50);
    err = cameraStart((framesize_t)sensorMax);
    if (err != ESP_OK) {
      logf("Kamera: %s volle Aufloesung nicht moeglich (0x%x), nutze UXGA", sensorNm.c_str(), err);
      esp_camera_deinit();
      delay(50);
      sensorMax = FRAMESIZE_UXGA;
      if (cameraStart(FRAMESIZE_UXGA) != ESP_OK) { cameraOk = false; return false; }
    }
  }
  cameraOk = true;
  sensorDefaults();
  if (cfg.framesize > sensorMax) cfg.framesize = sensorMax;
  logf("Kamera: %s erkannt (max. %s), PSRAM %s", sensorNm.c_str(), framesizeName(sensorMax), psram ? "ja" : "nein");
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
