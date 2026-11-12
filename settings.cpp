#include "core.h"
#include <Preferences.h>
#include "nvs_flash.h"

Settings cfg;

static void defaults() {
  cfg.wifiSsid = "";
  cfg.wifiPass = "";
  cfg.apSsid = "CamOS-" + deviceId;
  cfg.apPass = "camos1234";
  cfg.apMode = 0;
  String h = "camos-" + deviceId;
  h.toLowerCase();
  cfg.hostname = h;
  cfg.webPass = "";
  cfg.tz = "CET-1CEST,M3.5.0,M10.5.0/3";
  cfg.ntp = "pool.ntp.org";

  cfg.framesize = FRAMESIZE_VGA;
  cfg.quality = 12;
  cfg.brightness = 0;
  cfg.contrast = 0;
  cfg.saturation = 0;
  cfg.effect = 0;
  cfg.vflip = false;
  cfg.hmirror = false;
  cfg.awb = true;

  cfg.motionEnabled = true;
  cfg.motionThreshold = 25;
  cfg.motionSensitivity = 30;
  cfg.motionInterval = 500;
  cfg.motionCooldown = 10;
  cfg.motionSave = false;

  cfg.snapFlash = false;
  cfg.flashSnapLevel = 255;

  cfg.buttonPin = 0;
  cfg.buttonPhoto = true;
}

void settingsLoad() {
  uint64_t mac = ESP.getEfuseMac();
  char id[8];
  snprintf(id, sizeof id, "%02X%02X%02X", (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  deviceId = id;
  defaults();

  Preferences p;
  if (!p.begin("cfg", false)) return;
  cfg.wifiSsid   = p.getString("wssid", cfg.wifiSsid);
  cfg.wifiPass   = p.getString("wpass", cfg.wifiPass);
  cfg.apSsid     = p.getString("apssid", cfg.apSsid);
  cfg.apPass     = p.getString("appass", cfg.apPass);
  cfg.apMode     = p.getUChar("apmode", cfg.apMode);
  cfg.hostname   = p.getString("host", cfg.hostname);
  cfg.webPass    = p.getString("webpass", cfg.webPass);
  cfg.tz         = p.getString("tz", cfg.tz);
  cfg.ntp        = p.getString("ntp", cfg.ntp);

  cfg.framesize  = p.getInt("fs", cfg.framesize);
  cfg.quality    = p.getInt("qual", cfg.quality);
  cfg.brightness = p.getInt("bright", cfg.brightness);
  cfg.contrast   = p.getInt("contr", cfg.contrast);
  cfg.saturation = p.getInt("satur", cfg.saturation);
  cfg.effect     = p.getInt("effect", cfg.effect);
  cfg.vflip      = p.getBool("vflip", cfg.vflip);
  cfg.hmirror    = p.getBool("hmirror", cfg.hmirror);
  cfg.awb        = p.getBool("awb", cfg.awb);

  cfg.motionEnabled     = p.getBool("m_en", cfg.motionEnabled);
  cfg.motionThreshold   = p.getInt("m_thr", cfg.motionThreshold);
  cfg.motionSensitivity = p.getInt("m_sens", cfg.motionSensitivity);
  cfg.motionInterval    = p.getInt("m_int", cfg.motionInterval);
  cfg.motionCooldown    = p.getInt("m_cool", cfg.motionCooldown);
  cfg.motionSave        = p.getBool("m_save", cfg.motionSave);

  cfg.snapFlash      = p.getBool("snapfl", cfg.snapFlash);
  cfg.flashSnapLevel = p.getInt("snaplvl", cfg.flashSnapLevel);
  cfg.buttonPin      = p.getInt("btnpin", cfg.buttonPin);
  cfg.buttonPhoto    = p.getBool("btnphoto", cfg.buttonPhoto);
  p.end();
}

void settingsSave() {
  Preferences p;
  if (!p.begin("cfg", false)) return;
  p.putString("wssid", cfg.wifiSsid);
  p.putString("wpass", cfg.wifiPass);
  p.putString("apssid", cfg.apSsid);
  p.putString("appass", cfg.apPass);
  p.putUChar("apmode", cfg.apMode);
  p.putString("host", cfg.hostname);
  p.putString("webpass", cfg.webPass);
  p.putString("tz", cfg.tz);
  p.putString("ntp", cfg.ntp);

  p.putInt("fs", cfg.framesize);
  p.putInt("qual", cfg.quality);
  p.putInt("bright", cfg.brightness);
  p.putInt("contr", cfg.contrast);
  p.putInt("satur", cfg.saturation);
  p.putInt("effect", cfg.effect);
  p.putBool("vflip", cfg.vflip);
  p.putBool("hmirror", cfg.hmirror);
  p.putBool("awb", cfg.awb);

  p.putBool("m_en", cfg.motionEnabled);
  p.putInt("m_thr", cfg.motionThreshold);
  p.putInt("m_sens", cfg.motionSensitivity);
  p.putInt("m_int", cfg.motionInterval);
  p.putInt("m_cool", cfg.motionCooldown);
  p.putBool("m_save", cfg.motionSave);

  p.putBool("snapfl", cfg.snapFlash);
  p.putInt("snaplvl", cfg.flashSnapLevel);
  p.putInt("btnpin", cfg.buttonPin);
  p.putBool("btnphoto", cfg.buttonPhoto);
  p.end();
}

void settingsFactoryReset() {
  nvs_flash_erase();
  nvs_flash_init();
}
