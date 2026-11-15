#pragma once
// ============================================================
//  CamOS - Konfiguration / Pins (AI-Thinker ESP32-CAM)
// ============================================================
#define FW_NAME    "CamOS"
#define FW_VERSION "1.2.0"

// Kamera-Pins AI-Thinker
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN      4    // weisse Blitz-LED
#define STATUS_LED_PIN    33    // rote LED auf der Rueckseite (LOW = an)
#define FLASH_LEDC_CH      2    // LEDC Kanal 2 (Timer 1) - Kanal 0 nutzt die Kamera

#define NUM_RULES          3
#define NUM_VARS           8
#define NUM_TIMERS         8
#define LOG_LINES        150

#define AP_IP_STR  "192.168.4.1"
