#include <Arduino.h>
#include <WiFi.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PubSubClient.h>

using namespace m5;

// --- TEMPS DE SOMMEIL ---
#define SLEEP_SEC 30 
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
  
  // ⏳ PAUSE DEBUG : On attend 2s pour être sûr que le moniteur série est prêt
  delay(2000); 

  TimerCAM.Power.begin();
  TimerCAM.Camera.begin();
  SPIFFS.begin(true);

  Serial.println("\n--- REVEIL DU NICHOIR 1 ---");
  
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
           Serial.print("Envoi photo vers 'nichoir/1/photo'... ");
           
           // 👇 ICI C'EST IMPORTANT : On a ajouté le '/1/'
           if (client.publish("nichoir/1/photo", TimerCAM.Camera.fb->buf, TimerCAM.Camera.fb->len, false)) {
               Serial.println("SUCCÈS !");
           } else {
               Serial.println("ECHEC (Buffer overflow ?)");
           }
           
           Serial.println("Attente fin transmission (2s)...");
           delay(2000); 
        } else {
            Serial.println("Erreur Camera !");
        }
        TimerCAM.Camera.free();
        client.disconnect();
      } else {
          Serial.print("Echec connexion MQTT rc=");
          Serial.println(client.state());
      }
    } else {
        Serial.println("Echec connexion WiFi !");
    }
  } else {
      Serial.println("Erreur lecture Config !");
  }

  Serial.println("💤 Dodo (Deep Sleep ESP32)...");
  Serial.flush();
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  esp_sleep_enable_timer_wakeup(SLEEP_SEC * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {}