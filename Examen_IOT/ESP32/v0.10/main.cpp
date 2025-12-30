#include <Arduino.h>
#include <WiFi.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PubSubClient.h>

using namespace m5;

// --- TEMPS DE SOMMEIL (30s) ---
#define SLEEP_SEC 30 
// Facteur de conversion secondes -> microsecondes
#define uS_TO_S_FACTOR 1000000ULL  

WiFiClient espClient;
PubSubClient client(espClient);

String wifi_ssid, wifi_pass, mqtt_host;
int mqtt_port = 1883;

bool loadConfig() {
  if (!SPIFFS.exists("/config.json")) return false;
  File f = SPIFFS.open("/config.json", "r");
  String d = f.readString();
  f.close();
  
  int s1 = d.indexOf("ssid\":\"") + 7;
  wifi_ssid = d.substring(s1, d.indexOf("\"", s1));
  int p1 = d.indexOf("password\":\"") + 11;
  wifi_pass = d.substring(p1, d.indexOf("\"", p1));
  int h1 = d.indexOf("host\":\"") + 7;
  mqtt_host = d.substring(h1, d.indexOf("\"", h1));
  return true;
}

void setup() {
  Serial.begin(115200);
  
  // Init
  TimerCAM.Power.begin();
  TimerCAM.Camera.begin();
  SPIFFS.begin(true);

  Serial.println("\n--- REVEIL (Mode Interne) ---");
  
  float vbat = TimerCAM.Power.getBatteryVoltage();
  Serial.print("Batterie: "); Serial.print(vbat); Serial.println(" V");

  sensor_t* s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

  if (loadConfig()) {
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
    Serial.print("Connexion WiFi");
    
    int trys = 0;
    while (WiFi.status() != WL_CONNECTED && trys < 20) {
      delay(500); Serial.print("."); trys++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(" OK!");
      client.setServer(mqtt_host.c_str(), mqtt_port);
      client.setBufferSize(45000); 

      if (client.connect("NichoirESP32")) {
        if (TimerCAM.Camera.get()) {
           Serial.print("Envoi photo... ");
           client.publish("nichoir/photo", TimerCAM.Camera.fb->buf, TimerCAM.Camera.fb->len, false);
           Serial.println("FAIT !");
           
           Serial.println("Attente fin transmission (2s)...");
           delay(2000); 
        }
        TimerCAM.Camera.free();
        client.disconnect();
      }
    }
  }

  Serial.println("💤 Dodo (Deep Sleep ESP32)...");
  Serial.flush();
  
  // Coupure WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  // --- NOUVELLE METHODE DE SOMMEIL (PLUS FIABLE) ---
  // On configure le réveil sur le timer interne du processeur
  esp_sleep_enable_timer_wakeup(SLEEP_SEC * uS_TO_S_FACTOR);
  
  // On lance le sommeil profond
  esp_deep_sleep_start();
  
  // Le code ne va jamais ici
}

void loop() {}