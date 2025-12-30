#include <Arduino.h>
#include <WiFi.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PubSubClient.h>

using namespace m5;

WiFiClient espClient;
PubSubClient client(espClient);

// Variables de config
String wifi_ssid, wifi_pass, mqtt_host;
int mqtt_port = 1883;

// Lecture config
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
  TimerCAM.Power.begin();
  TimerCAM.Camera.begin();
  SPIFFS.begin(true);
  
  // Petite config caméra pour alléger le WiFi
  sensor_t* s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA); 

  if (!loadConfig()) {
    Serial.println("❌ Erreur Config !");
    while(1); // Bloque ici si pas de config
  }

  // Connexion WiFi (Une seule fois au début)
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  Serial.print("Connexion WiFi à " + wifi_ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✔ WiFi Connecté ! IP: " + WiFi.localIP().toString());

  // Config MQTT
  client.setServer(mqtt_host.c_str(), mqtt_port);
  client.setBufferSize(45000); // Important pour la photo
}

void loop() {
  // 1. Vérifier connexion MQTT
  if (!client.connected()) {
    Serial.print("Connexion au Broker MQTT (" + mqtt_host + ")... ");
    if (client.connect("NichoirTesteur")) {
      Serial.println("Connecté !");
    } else {
      Serial.print("Echec, rc=");
      Serial.println(client.state());
      delay(2000);
      return;
    }
  }
  client.loop();

  // 2. Prendre et envoyer photo
  if (TimerCAM.Camera.get()) {
    Serial.print("📸 Capture... ");
    size_t len = TimerCAM.Camera.fb->len;
    uint8_t* buf = TimerCAM.Camera.fb->buf;

    Serial.print("Envoi (" + String(len) + " bytes)... ");
    
    if (client.publish("nichoir/photo", buf, len, false)) {
      Serial.println("SUCCÈS ! ✅");
    } else {
      Serial.println("ÉCHEC ❌ (Buffer trop petit ?)");
    }
    
    TimerCAM.Camera.free();
  } else {
    Serial.println("Erreur Caméra");
  }

  // 3. Attendre 10 secondes avant la prochaine
  Serial.println("Attente 10s...");
  delay(10000);
}