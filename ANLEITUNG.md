# CamOS – Firmware für ESP32-CAM

Firmware für das **AI-Thinker ESP32-CAM** Modul (OV2640). Sie bringt einen Webserver, einen eigenen Access Point, ein optionales Heim-WLAN, Bewegungserkennung, eine Konsole, Regeln im Tasmota-Stil, Blitzsteuerung, SD-Karte, OTA-Updates und ein Plugin-System mit.

## Funktionen

| Bereich | Was es kann |
|---|---|
| **Access Point** | Eigenes WLAN `CamOS-XXXXXX`, Passwort `camos1234`, IP `192.168.4.1`, Captive Portal (Seite öffnet sich am Handy automatisch) |
| **Heim-WLAN** | Optional. AP-Modus „immer an“ oder „nur wenn kein WLAN“. mDNS: `http://camos-xxxxxx.local` |
| **Live-Stream** | MJPEG auf Port 81 (`http://IP:81/stream`), Einzelbild unter `/capture` |
| **Kamera-Sensoren** | **OV2640** (bis UXGA 1600×1200), **OV3660** (bis QXGA 2048×1536, 3 MP) und OV5640 (bis 5 MP) werden automatisch erkannt. Beim Wechsel des Sensors werden passende Standardwerte gesetzt (OV3660: Bild gedreht, Helligkeit +1, Sättigung −2) |
| **Kamera** | Auflösung QQVGA–UXGA (OV3660: bis QXGA), Qualität, Helligkeit, Kontrast, Sättigung, Effekte, Spiegeln, Weissabgleich |
| **Bewegungserkennung** | Bildvergleich auf 1/8 Auflösung. Schwelle, Empfindlichkeit, Intervall und Nachlaufzeit lassen sich einstellen. Live-Graph im Browser |
| **Blitz** | LED an GPIO4 dimmbar (0–255), Puls, Blitz bei Fotos |
| **Konsole** | Im Browser und über Serial (115200 Baud), mit Befehlsverlauf |
| **Regeln** | Wie Tasmota: `ON Motion#Detected DO Flash 255 ENDON`, Variablen, Timer, Backlog mit Delay |
| **IO0-Taste** | Kurz drücken = Foto. Mit SD-Karte wird es auf der SD gespeichert, ohne SD automatisch auf den PC heruntergeladen (die Weboberfläche muss offen sein) |
| **SD-Dateimanager** | Belegten und freien Speicher anzeigen, Ordner durchsuchen, Dateien hochladen (auch per Drag & Drop), herunterladen, ansehen und löschen, Ordner anlegen |
| **Benachrichtigungen** | `WebSend <url>` (HTTP/HTTPS GET), z. B. an ntfy.sh oder einen Home-Assistant-Webhook |
| **Zeit** | NTP mit Zeitzone, Events `Time#Minute` für Zeitpläne |
| **OTA** | Firmware-Update über den Browser |
| **Sicherheit** | Optionales Web-Passwort (Benutzer `admin`) |
| **Plugins** | Eine `.cpp`-Datei in `src/plugins/` – siehe [PLUGINS_AI.md](PLUGINS_AI.md) |
| **GPIO** | `Gpio12 1` usw., PIR-Plugin für Bewegungsmelder |

---

## 1. Flashen der fertigen .bin

Im Ordner `bin/` liegen zwei Dateien:

| Datei | Wofür | Adresse |
|---|---|---|
| `CamOS-esp32cam-factory.bin` | **Erstinstallation** (Bootloader, Partitionen und App) | `0x0` |
| `CamOS-esp32cam-ota.bin` | Update über die Weboberfläche (System → OTA) | – |

### Variante A: Im Browser (am einfachsten)
1. Chrome oder Edge öffnen: https://espressif.github.io/esptool-js/
2. ESP32-CAM mit USB-Seriell-Adapter (oder ESP32-CAM-MB-Board) anschliessen.
3. **GPIO0 mit GND verbinden** und Reset drücken (Flash-Modus). Beim MB-Board: IO0-Taste halten und RST drücken.
4. „Connect“ klicken, den Port wählen, Adresse `0x0` eintragen, `CamOS-esp32cam-factory.bin` wählen und „Program“ klicken.
5. GPIO0 von GND trennen und Reset drücken.

### Variante B: esptool
```bash
pip install esptool
```
```bash
esptool.py --chip esp32 --port COM5 --baud 460800 write_flash 0x0 bin/CamOS-esp32cam-factory.bin
```

### Variante C: Selbst kompilieren (PlatformIO)
```bash
pip install platformio
```
```bash
pio run
```
```bash
pio run -t upload
```
Nach jedem Build legt `tools/merge_bin.py` die fertigen Dateien automatisch in `bin/` ab.

> **Stromversorgung:** Die ESP32-CAM braucht stabile **5 V / mind. 500 mA**. Viele Probleme (Neustarts, „Brownout“, Kamera-Fehler) kommen von schwachen USB-Adaptern.

---

## 2. Erster Start

1. Mit dem WLAN **`CamOS-XXXXXX`** verbinden (Passwort **`camos1234`**).
2. Die Seite öffnet sich meist automatisch. Sonst im Browser **http://192.168.4.1** aufrufen.
3. Optional: Unter **System → WLAN** das Heim-WLAN eintragen. Die Kamera bleibt trotzdem über den AP erreichbar (AP-Modus „Immer an“).
4. Empfohlen: Unter **System** das AP-Passwort ändern und ein Web-Passwort setzen.

Wichtige URLs:

| URL | Inhalt |
|---|---|
| `/` | Weboberfläche |
| `http://IP:81/stream` | MJPEG-Stream (z. B. für VLC, Home Assistant, Frigate) |
| `/capture` | Einzelbild JPEG (`?flash=1` = mit Blitz) |
| `/api/status` | Status als JSON |
| `/api/cmd?c=Flash%20255` | Befehl per HTTP (auch als POST mit dem Befehl im Body) |
| `/p/<plugin>/...` | HTTP-Endpunkte von Plugins |
| `/api/lastphoto` | letztes Foto aus dem RAM (wenn keine SD-Karte) |
| `/api/fs?dir=/` | Ordnerinhalt der SD als JSON |
| `/api/dl?f=/pfad` | Datei herunterladen (`&inline=1` = im Browser anzeigen) |
| `/api/upload?f=/pfad` | Datei hochladen (POST, Dateiinhalt als Body) |
| `/api/rm?f=/pfad`, `/api/mkdir?d=/pfad` | Löschen (Ordner mit Inhalt) / Ordner anlegen (POST) |
| `/sd/<pfad>` | Datei direkt anzeigen, z. B. `/sd/cam/img_xxx.jpg` |

### Foto-Taste (IO0)
Drückst du die **IO0-Taste kurz** (auf dem ESP32-CAM-MB-Board oder einen Taster zwischen GPIO0 und GND), wird nach dem Loslassen ein Foto gemacht:
* **Mit SD-Karte:** Es wird als `/cam/btn_DATUM.jpg` gespeichert.
* **Ohne SD-Karte:** Es bleibt im RAM, und der Browser lädt es automatisch auf den PC herunter, solange die Weboberfläche offen ist. Das lässt sich im Live-Tab abschalten. Mit „Letztes RAM-Foto laden“ holst du es auch später noch.

GPIO0 ist zugleich der Takt der Kamera. Solange die Taste gedrückt ist, ist das Bild gestört, und die Bewegungserkennung pausiert. Deshalb wird das Foto erst nach dem Loslassen aufgenommen. **Nicht beim Einschalten oder Reset drücken**, sonst startet der ESP32 im Flash-Modus. Falls dein Board stattdessen einen eigenen Taster hat: `ButtonPin 13` (Taster zwischen GPIO13 und GND).

Ist ein Web-Passwort gesetzt, braucht der Stream Basic-Auth oder `?t=<token>`. Den Token liefert `/api/status`, er wird bei jedem Neustart neu erzeugt.

---

## 3. Konsole & Befehle

Befehle funktionieren in der Web-Konsole, über Serial, in Regeln und per HTTP. Gross-/Kleinschreibung spielt keine Rolle. Ein Befehl **ohne Argument** zeigt den aktuellen Wert. Ein leerer Wert wird als `""` geschrieben.

| Befehl | Beschreibung |
|---|---|
| `Help` | Alle Befehle anzeigen |
| `Status` | Kompletter Status (JSON) |
| `Restart 1` / `Reset 1` | Neustart / Werkseinstellungen |
| `Flash <0-255\|on\|off\|toggle>` | Blitz-LED |
| `FlashPulse <ms> [level]` | Blitz kurz einschalten |
| `SnapFlash <0\|1>`, `SnapLevel <0-255>` | Blitz bei Fotos |
| `Photo [prefix]` | Foto auf SD, ohne SD in den RAM zum PC-Download (`/api/lastphoto`) |
| `Snapshot [prefix]` | Foto nur auf SD (`/cam/prefix_DATUM.jpg`) |
| `SDInit` | SD-Karte nachträglich einbinden |
| `ButtonPin <-1\|0\|12\|13\|14\|15>` | Tasten-Pin (-1 = aus, 0 = IO0-Taste) |
| `ButtonPhoto <0\|1>` | Kurz drücken = Foto |
| `Motion <0\|1>` | Bewegungserkennung an/aus |
| `MotionThreshold <1-255>` | Helligkeitsänderung pro Pixel, ab der ein Pixel als „geändert“ gilt |
| `MotionSensitivity <0.1-100>` | Ab wie viel % geänderter Pixel ausgelöst wird |
| `MotionInterval <ms>` / `MotionCooldown <s>` / `MotionSave <0\|1>` | Prüfintervall / Nachlaufzeit / Foto auf SD |
| `FrameSize <VGA\|SVGA\|…\|UXGA\|FHD\|QXGA>` (FHD/QXGA nur OV3660/OV5640), `Quality`, `Brightness`, `Contrast`, `Saturation`, `Effect`, `VFlip`, `HMirror`, `AWB` | Kamera |
| `WifiSSID`, `WifiPass`, `WifiConnect`, `WifiScan` | Heim-WLAN |
| `APSSID`, `APPass`, `APMode <0\|1>` | Access Point (mit `WifiConnect` anwenden) |
| `Hostname`, `WebPassword`, `Timezone`, `NtpServer` | Allgemein |
| `Rule<1-3> …`, `RuleTimer<1-8> <s>` | Regeln (siehe unten) |
| `Var<1-8>`, `Mem<1-8>`, `Add<n>`, `Sub<n>`, `Mult<n>` | Variablen (`Mem` wird dauerhaft gespeichert) |
| `Event <name>[=wert]` | Eigenes Event `Event#name` auslösen |
| `Backlog cmd1; Delay 10; cmd2` | Mehrere Befehle, `Delay` in 0,1 s |
| `WebSend <url>` | HTTP(S) GET, Antwortcode als Event `WebSend#Code` |
| `Gpio<2\|12\|13\|14\|15> [0\|1]` | GPIO schalten/lesen (2, 14 und 15 belegt die SD-Karte) |
| `Echo <text>` / `Time` | Text ins Log / aktuelle Zeit |
| `Plugins`, `Plugin <name> <0\|1>` | Plugins auflisten / ein- und ausschalten |

---

## 4. Regeln (wie Tasmota)

Es gibt 3 Regel-Slots (`Rule1`–`Rule3`). Jeder kann mehrere `ON … ENDON`-Blöcke enthalten.

```
ON <Event>[<op><Wert>] DO <Befehl> ENDON
```

* Operatoren: `=` `!=` `>` `<` `>=` `<=`
* Wildcard: `ON Event#* DO …`
* Mehrere Befehle: `DO Backlog Flash 255; Delay 20; Flash 0 ENDON`, oder einfach mit `;` trennen
* Aktivieren: `Rule1 1`, deaktivieren: `Rule1 0`, anhängen: `Rule1 +ON … ENDON`, löschen: `Rule1 ""`

**Events**

| Event | Wert |
|---|---|
| `System#Boot` | Firmware-Version |
| `Motion#Detected` / `Motion#End` | % geänderter Pixel / Zähler |
| `Wifi#Connected` / `Wifi#Disconnected` / `Wifi#AP` | IP / – / 1 oder 0 |
| `Time#Initialized`, `Time#Minute` | Zeit / Minute des Tages (480 = 08:00) |
| `Rules#Timer` | Nummer des abgelaufenen RuleTimers |
| `Flash#State`, `Gpio<n>#State` | Wert |
| `SD#Saved` / `SD#Uploaded` | Dateiname |
| `Button#Short` / `Button#Long` | Dauer in ms (lang = ab 1,5 s) |
| `Photo#Ready` | Dateiname des Fotos im RAM (ohne SD) |
| `WebSend#Code` | HTTP-Code |
| `Event#<name>` | über den Befehl `Event` |
| Plugins: `PIR#State`, `Timelapse#Saved`, … | |

**Platzhalter:** `%value%` `%var1%`…`%var8%` `%mem1%`…`%mem8%` `%time%` `%date%` `%timestamp%` `%uptime%` `%motion%` `%motioncount%` `%flash%` `%ip%` `%device%` `%hostname%`

**Beispiele**
```
Rule1 ON Motion#Detected DO Backlog Flash 255; Delay 50; Flash 0 ENDON
Rule1 1

Rule2 ON Motion#Detected DO WebSend https://ntfy.sh/mein_geheimes_topic?m=Bewegung+%value%25 ENDON
Rule2 1

Rule3 ON Time#Minute=1320 DO Motion 0 ENDON ON Time#Minute=360 DO Motion 1 ENDON
Rule3 1

ON System#Boot DO RuleTimer1 60 ENDON ON Rules#Timer=1 DO Backlog Snapshot tl; RuleTimer1 60 ENDON
ON PIR#State=1 DO Backlog Flash 255; Snapshot pir; Flash 0 ENDON
```

---

## 5. Plugins

Ein Plugin ist **eine einzige `.cpp`-Datei** in `src/plugins/`. Es wird beim nächsten Build automatisch eingebunden. Es kann eigene Befehle, Events, Status-Werte, HTTP-Endpunkte und Einstellungen haben.

Mitgelieferte Plugins:

| Plugin | Funktion | Befehle |
|---|---|---|
| `heartbeat` | Rote LED blinkt als Lebenszeichen, schnell bei Bewegung | `Heartbeat <ms>` |
| `timelapse` | Zeitraffer auf SD | `Timelapse <s>` (0 = aus) |
| `pir` | PIR-Sensor an GPIO13 → `PIR#State` (standardmässig aus) | `PirPin <n>` |
| `webhook` | URL-Aufruf bei Bewegung | `WebhookUrl <url>`, `Webhook` (Test) |

### Plugins mit KI erstellen
Gib einer KI (Claude, ChatGPT …) die Datei **[PLUGINS_AI.md](PLUGINS_AI.md)** und beschreibe, was das Plugin tun soll, zum Beispiel:

> „Lies PLUGINS_AI.md. Erstelle ein CamOS-Plugin, das bei Bewegung ein Foto per Telegram-Bot sendet. Bot-Token und Chat-ID per Konsolenbefehl einstellbar.“

Die Antwort als `src/plugins/<name>.cpp` speichern, `pio run` ausführen und die neue `bin/CamOS-esp32cam-ota.bin` über die Weboberfläche hochladen.

---

## 6. Projektstruktur

```
platformio.ini        Build-Konfiguration
tools/merge_bin.py    erzeugt bin/*.bin nach dem Build
bin/                  fertige Firmware
src/
  main.cpp            Setup, Loop, WLAN/AP, Zeit, serielle Konsole
  config.h            Pins, Version, Limits
  core.h              interne API
  plugin_api.h        Plugin-API (für Plugin-Autoren)
  settings.cpp        Einstellungen (NVS)
  camera.cpp          Kamera + Blitz
  motion.cpp          Bewegungserkennung (eigener Task)
  rules.cpp           Regel-Engine, Events, Befehls-Queue, Variablen
  commands.cpp        alle Konsolenbefehle + Status-JSON
  storage.cpp         SD-Karte
  webserver.cpp       HTTP-Server (Port 80) + Stream (Port 81) + OTA
  web_ui.h            Weboberfläche (HTML/CSS/JS)
  plugins.cpp         Plugin-Verwaltung + api::
  plugins/*.cpp       Plugins
```

## 7. Hinweise & Fehlersuche

* **Kamera-Fehler 0x105/0x20001:** Flachbandkabel prüfen, stabile 5-V-Versorgung verwenden.
* **Nur ein Stream gleichzeitig.** Ein zweiter Client wartet, bis der erste schliesst.
* **Bewegung löst ständig aus:** `MotionThreshold` erhöhen (z. B. 35) oder `MotionSensitivity` erhöhen (z. B. 5). Bei Dunkelheit rauscht der Sensor stark.
* **Blitz und Bewegung:** Beim Ein- oder Ausschalten des Blitzes kalibriert sich die Erkennung neu, damit kein Fehlalarm entsteht.
* **SD-Karte:** FAT32, läuft im 1-Bit-Modus (GPIO4 bleibt für den Blitz frei). Die Blitz-LED kann beim Zugriff kurz glimmen.
* **Passwort vergessen:** Über Serial `Reset 1` eingeben oder den Flash komplett löschen (`esptool.py erase_flash`) und neu flashen.
* Logs: Web-Konsole oder serieller Monitor mit 115200 Baud.
