#include <Arduino.h>
#include <M5TimerCAM.h> 
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "esp_camera.h"
#include <Wire.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_sleep.h"

// --- VARIABLES SAUVEGARDÉES (MÉMOIRE RTC) ---
RTC_DATA_ATTR uint32_t uptimeSeconds = 0;
RTC_DATA_ATTR uint32_t lastBatterySendEpoch = 0;

/* =========================================================
   ================ PARAMÈTRES MODIFIABLES =================
   ========================================================= */

//⏱️ Temps de sommeil entre deux réveils
#define SLEEP_TIME_SEC        30    

// ⏱️ Temps de vérification PIR (ms)
#define PIR_CHECK_TIME_MS     3000   

// ⏱️ Délai allumage IR avant photo (ms)
#define IR_STABILISATION_MS   1000   

// ⏱️ Délai après envoi batterie (MQTT) (ms)
#define DELAY_AFTER_BATT_MS  300    

// ⏱️ Délai après envoi photo (vider buffer MQTT) (ms)
#define DELAY_AFTER_PHOTO_MS 1000   

/* ========================================================= */
#define BATTERY_SEND_INTERVAL_SEC (24 * 60 * 60)

// --- PINS ---
#define PIN_LED_INTERNE 2
#define PIN_LED_IR 4
#define PIN_PIR GPIO_NUM_13

// --- IDENTITÉ ---
#define ID_NICHOIR "1"

// ================= CONFIG =================
char wifi_ssid[32] = "";
char wifi_pass[64] = "";
char serveur_mqtt[40] = "192.168.0.16";
char port_mqtt[6] = "1883";

// ================= OBJETS =================
WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);

/* =========================================================
   ======================= HTML COMPLET ====================
   ========================================================= */

const char PAGE_CONFIG[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Connexion Nichoir</title>
<style>
  body {
    background: radial-gradient(circle,#1f3328,#0b1410);
    font-family: Arial, sans-serif;
    height: 100vh;
    display: flex;
    justify-content: center;
    align-items: center;
    margin: 0;
  }
  .card {
    background: #111;
    padding: 30px;
    border-radius: 12px;
    width: 300px;
    color: white;
    box-shadow: 0 4px 15px rgba(0,0,0,0.5);
  }
  h2 {
    text-align: center;
    margin-bottom: 20px;
    color: #4CAF50;
  }
  input, select, button {
    width: 100%;
    box-sizing: border-box;
    margin-top: 10px;
    padding: 12px;
    border-radius: 6px;
    border: 1px solid #333;
    font-size: 14px;
  }
  input, select {
    background: #222;
    color: #fff;
  }
  button {
    background: #4CAF50;
    color: white;
    font-weight: bold;
    cursor: pointer;
    border: none;
    margin-top: 20px;
  }
  .footer {
    margin-top: 20px;
    font-size: 11px;
    text-align: center;
    color: #666;
  }
</style>
</head>
<body>
<div class="card">
  <h2>🐦 Config Nichoir</h2>
  <form method="POST" action="/save">
    <label>Réseau WiFi :</label>
    <select name="ssid">%WIFI%</select>

    <label>Mot de passe :</label>
    <input type="password" name="password">

    <label>Serveur MQTT :</label>
    <input type="text" name="mqtt" value="192.168.0.16">

    <label>Port :</label>
    <input type="number" name="port" value="1883">

    <button type="submit">ENREGISTRER & REDÉMARRER</button>
  </form>
  <div class="footer">Projet IOT – vFinal</div>
</div>
</body>
</html>
)rawliteral";

/* =========================================================
   ======================= DEEP SLEEP ======================
   ========================================================= */

// On garde ta méthode ESP_DEEP_SLEEP qui fonctionne bien avec le câble.
void dodoProfond(int secondes) {
  uptimeSeconds += secondes;

  Serial.printf(">>> DODO (Deep Sleep) %d s | Uptime %lu <<<\n", secondes, uptimeSeconds);
  Serial.flush();

  // 1. Arrêt propre WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // ============================================================
  // 🛠️ CORRECTION BATTERIE M5 TIMERCAM
  // ============================================================
  // Sur la TimerCAM, le GPIO 33 maintient l'alimentation batterie.
  // Si on ne le "verrouille" pas (HOLD), la batterie se coupe pendant le sommeil.
  
  // On s'assure que le pin est en sortie et HAUT
  gpio_pad_select_gpio(GPIO_NUM_33);
  gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_33, 1); 
  
  // On active le verrouillage du pin pour qu'il reste HAUT même en dormant
  gpio_hold_en(GPIO_NUM_33);
  gpio_deep_sleep_hold_en();
  // ============================================================

  // 2. IMPORTANT : Maintien des périphériques RTC pour éviter les bugs au réveil
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

  // 3. Configuration du timer de réveil ESP32
  esp_sleep_enable_timer_wakeup((uint64_t)secondes * 1000000ULL);
  
  // 4. Lancement du sommeil
  esp_deep_sleep_start();
}

/* =========================================================
   ======================= CAMERA ==========================
   ========================================================= */

bool demarrerCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = 32; config.pin_d1 = 35; config.pin_d2 = 34; config.pin_d3 = 5;
  config.pin_d4 = 39; config.pin_d5 = 18; config.pin_d6 = 36; config.pin_d7 = 19;
  config.pin_xclk = 27; config.pin_pclk = 21; config.pin_vsync = 22;
  config.pin_href = 26; config.pin_sccb_sda = 25; config.pin_sccb_scl = 23;
  config.pin_pwdn = -1; config.pin_reset = 15;

  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  return esp_camera_init(&config) == ESP_OK;
}

/* =========================================================
   ======================= MQTT ============================
   ========================================================= */

void envoyerBatterieEtPhoto(camera_fb_t *fb) {
  WiFi.begin(wifi_ssid, wifi_pass);
  
  // On insiste un peu plus sur la connexion WiFi
  unsigned long startWifi = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWifi < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Echec WiFi !");
    return;
  }

  mqtt.setServer(serveur_mqtt, atoi(port_mqtt));
  // Augmentation du buffer pour être sûr que la photo passe
  mqtt.setBufferSize(50000); 

  if (mqtt.connect("NichoirESP32")) {
    Serial.println("MQTT Connecté !");

    // 🔋 BATTERIE
    uint32_t elapsed = uptimeSeconds - lastBatterySendEpoch;
    if (lastBatterySendEpoch == 0 || elapsed >= BATTERY_SEND_INTERVAL_SEC) {
      float batt = TimerCAM.Power.getBatteryVoltage() / 1000.0;
      mqtt.publish(("nichoir/" + String(ID_NICHOIR) + "/batterie").c_str(), String(batt, 2).c_str());
      lastBatterySendEpoch = uptimeSeconds;
      Serial.printf("Batterie envoyée : %.2f V\n", batt);
      delay(DELAY_AFTER_BATT_MS);
    }

    // 📸 PHOTO
    if (fb) {
      bool success = mqtt.publish(
        ("nichoir/" + String(ID_NICHOIR) + "/photo").c_str(),
        fb->buf,
        fb->len
      );
      
      if (success) {
        Serial.println("📸 Photo envoyée avec succès");
      } else {
        Serial.println("❌ Erreur envoi photo (Trop lourd ?)");
      }
      delay(DELAY_AFTER_PHOTO_MS);
    }

    mqtt.disconnect();
  } else {
    Serial.println("Echec connexion MQTT");
  }

  WiFi.disconnect(true);
}


/* =========================================================
   ======================= SPIFFS ==========================
   ========================================================= */

bool configExiste() {
  SPIFFS.begin(true);
  return SPIFFS.exists("/config.json");
}

void chargerConfig() {
  File f = SPIFFS.open("/config.json", "r");
  if (!f) return;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, f);
  strlcpy(wifi_ssid, doc["ssid"] | "", sizeof(wifi_ssid));
  strlcpy(wifi_pass, doc["pass"] | "", sizeof(wifi_pass));
  strlcpy(serveur_mqtt, doc["mqtt"] | "192.168.0.16", sizeof(serveur_mqtt));
  strlcpy(port_mqtt, doc["port"] | "1883", sizeof(port_mqtt));
  f.close();
}

void sauverConfig() {
    File f = SPIFFS.open("/config.json", "w");
    if (!f) return;
    DynamicJsonDocument doc(1024);
    doc["ssid"] = wifi_ssid;
    doc["pass"] = wifi_pass;
    doc["mqtt"] = serveur_mqtt;
    doc["port"] = port_mqtt;
    serializeJson(doc, f);
    f.close();
}

String scanWifi() {
  int n = WiFi.scanNetworks();
  String list = "<option value=''>Choisir un WiFi</option>";
  for (int i = 0; i < n; i++) {
    list += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + "dBm)</option>";
  }
  return list;
}

void modeConfiguration() {
    Serial.println(">>> MODE CONFIGURATION <<<");
    WiFi.softAP("Nichoir-Config");
    
    server.on("/", HTTP_GET, []() {
        String page = PAGE_CONFIG;
        page.replace("%WIFI%", scanWifi());
        server.send(200, "text/html", page);
    });

    server.on("/save", HTTP_POST, []() {
        if(server.hasArg("ssid")) strcpy(wifi_ssid, server.arg("ssid").c_str());
        if(server.hasArg("password")) strcpy(wifi_pass, server.arg("password").c_str());
        if(server.hasArg("mqtt")) strcpy(serveur_mqtt, server.arg("mqtt").c_str());
        if(server.hasArg("port")) strcpy(port_mqtt, server.arg("port").c_str());
        
        sauverConfig();
        server.send(200, "text/plain", "Sauvegarde OK. Redemarrage...");
        delay(2000);
        ESP.restart();
    });

    server.begin();
    while(true) {
        server.handleClient();
        delay(10);
    }
}

/* =========================================================
   ======================= SETUP ===========================
   ========================================================= */

void setup() {
  // 1. PROTECTION ABSOLUE POUR LA BATTERIE
  // Désactive la détection de chute de tension qui tue le module au réveil sur batterie
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // 2. Initialisation TimerCAM
  // Force le maintient de l'alimentation (Power Hold)
  TimerCAM.Power.begin();

  Serial.begin(115200);
  Serial.println("\n--- BOOT NICHOIR ---");

  pinMode(PIN_LED_IR, OUTPUT);
  pinMode(PIN_LED_INTERNE, OUTPUT);
  pinMode(PIN_PIR, INPUT);

  digitalWrite(PIN_LED_INTERNE, LOW);

  if (!configExiste()) {
    modeConfiguration();
  }
  chargerConfig();

  // 🔍 VERIFICATION PIR
  bool mouvement = false;
  unsigned long start = millis();
  
  Serial.println("Check Mouvement...");
  while (millis() - start < PIR_CHECK_TIME_MS) {
    if (digitalRead(PIN_PIR) == HIGH) {
      mouvement = true;
      break;
    }
    delay(50);
  }

  if (!mouvement) {
    Serial.println("Aucun mouvement -> Dodo");
    dodoProfond(SLEEP_TIME_SEC);
  }

  Serial.println("!!! MOUVEMENT DETECTE !!!");

  if (!demarrerCamera()) {
    Serial.println("Erreur Cam");
    dodoProfond(SLEEP_TIME_SEC);
  }

  // 🌙 IR + PHOTO
  digitalWrite(PIN_LED_IR, HIGH); // Allumer IR
  delay(IR_STABILISATION_MS);

  camera_fb_t *fb = esp_camera_fb_get(); // Clic Clac
  
  digitalWrite(PIN_LED_IR, LOW); // Eteindre IR

  if (fb) {
    Serial.printf("Photo prise: %d bytes\n", fb->len);
    // On envoie la photo
    envoyerBatterieEtPhoto(fb);
    esp_camera_fb_return(fb);
  } else {
    Serial.println("Echec capture photo");
  }

  dodoProfond(SLEEP_TIME_SEC);
}

void loop() {}