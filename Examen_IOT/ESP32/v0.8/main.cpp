#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PubSubClient.h> // 👈 NOUVELLE LIBRAIRIE

using namespace m5;

WebServer server(80);

// CLIENT MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// AP MODE
const char* ssid_ap = "APNichoir";
const char* password_ap = "12345678";

// CONFIG VARS (valeurs par défaut)
String wifi_ssid = "";
String wifi_password = "";
String mqtt_host = "";
int mqtt_port = 1883;
String mqtt_topic_img = "nichoir/photo"; // Topic pour l'image

// PHOTO BUFFER
camera_fb_t* lastPhoto = nullptr;

// ------------------------------------------------
//      SCAN WIFI
// ------------------------------------------------
String getWifiList() {
  int n = WiFi.scanNetworks();
  String options = "";
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    options += "<option value='" + ssid + "'>" + ssid + "</option>";
  }
  if (n == 0) options = "<option value=''>Aucun réseau détecté</option>";
  return options;
}

// ------------------------------------------------
//      LOAD CONFIG.JSON
// ------------------------------------------------
bool loadConfig() {
  if (!SPIFFS.exists("/config.json")) {
    Serial.println("Aucun fichier config.json → AP MODE");
    return false;
  }
  File file = SPIFFS.open("/config.json", "r");
  if (!file) return false;

  String data = file.readString();
  file.close();

  Serial.println("Config chargée :");
  Serial.println(data);

  // Parsing manuel (CORRECTION DES INDEX)
  // "wifi_ssid":" fait 13 caractères
  int s1 = data.indexOf("\"wifi_ssid\":\"");
  int s2 = data.indexOf("\"", s1 + 13); 
  wifi_ssid = data.substring(s1 + 13, s2);

  // "wifi_password":" fait 17 caractères
  int p1 = data.indexOf("\"wifi_password\":\"");
  int p2 = data.indexOf("\"", p1 + 17);
  wifi_password = data.substring(p1 + 17, p2);

  // "mqtt_host":" fait 13 caractères
  int h1 = data.indexOf("\"mqtt_host\":\"");
  int h2 = data.indexOf("\"", h1 + 13);
  mqtt_host = data.substring(h1 + 13, h2);

  int mp1 = data.indexOf("\"mqtt_port\":");
  int mp2 = data.indexOf("}", mp1);
  String portStr = data.substring(mp1 + 12, mp2);
  if (portStr.length() > 0) mqtt_port = portStr.toInt();

  return true;
}

// ------------------------------------------------
//      CONNECT WIFI
// ------------------------------------------------
bool connectToWiFi() {
  Serial.println("Connexion WiFi à " + wifi_ssid + "...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 100) { // 10 sec max
    delay(100);
    timeout++;
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✔ WiFi Connecté ! IP: " + WiFi.localIP().toString());
    return true;
  }
  Serial.println("❌ Échec WiFi");
  return false;
}

// ------------------------------------------------
//      FONCTION D'ENVOI MQTT (NOUVEAU)
// ------------------------------------------------
void sendPhotoToMQTT() {
  if (!lastPhoto) return;

  Serial.println("Tentative connexion MQTT...");
  client.setServer(mqtt_host.c_str(), mqtt_port);
  
  // Augmenter la taille du buffer pour accepter une image (IMPORTANT)
  // Une image QVGA fait ~4-8ko, VGA peut faire 15-20ko. On met 40ko par sécurité.
  client.setBufferSize(40000); 

  if (client.connect("ESP32_Nichoir")) {
    Serial.println("✔ Connecté au Broker MQTT !");
    Serial.print("Envoi de l'image (" + String(lastPhoto->len) + " bytes) sur " + mqtt_topic_img + "... ");
    
    // Publication de l'image binaire
    // false = pas de 'retain' (on ne garde pas l'image sur le broker indéfiniment)
    bool success = client.publish(mqtt_topic_img.c_str(), lastPhoto->buf, lastPhoto->len, false);
    
    if (success) {
      Serial.println("SUCCÈS !");
    } else {
      Serial.println("ÉCHEC (Buffer trop petit ?)");
    }
    
    client.disconnect();
  } else {
    Serial.print("❌ Échec connexion MQTT. État rc=");
    Serial.println(client.state());
  }
}

// ------------------------------------------------
//      HANDLERS WEB
// ------------------------------------------------

void handleInfo() {
  String json = "{";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mqtt_host\":\"" + mqtt_host + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleSave() {
  if (!server.hasArg("ssid")) { server.send(400, "text/plain", "Missing fields"); return; }

  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  String host = server.arg("mqtt_host");
  String port = server.arg("mqtt_port");

  String json = "{\"wifi_ssid\":\"" + ssid + "\",\"wifi_password\":\"" + pass + "\",\"mqtt_host\":\"" + host + "\",\"mqtt_port\":" + port + "}";
  
  File file = SPIFFS.open("/config.json", "w");
  file.print(json);
  file.close();
  
  server.send(200, "text/html", "Sauvegarde OK. Redemarrage...");
  delay(1000);
  ESP.restart();
}

// ⭐ /capture modifiée : Prend la photo ET l'envoie en MQTT
void handleCapture() {
  Serial.println("📸 Capture demandée...");
  
  // 1. Nettoyer
  if (lastPhoto) {
    TimerCAM.Camera.free(); 
    lastPhoto = nullptr;
  }

  // 2. Capturer
  if (!TimerCAM.Camera.get()) {
    server.send(500, "text/plain", "Erreur Camera");
    return;
  }
  lastPhoto = TimerCAM.Camera.fb;

  // 3. Envoyer en MQTT (Si connecté au WiFi)
  if (WiFi.status() == WL_CONNECTED) {
    sendPhotoToMQTT();
  } else {
    Serial.println("Pas de WiFi, envoi MQTT impossible.");
  }

  // 4. Afficher le résultat au navigateur
  server.sendHeader("Location", "/photo.jpg", true);
  server.send(302, "text/plain", "");
}

void handlePhoto() {
  if (!lastPhoto) { server.send(404, "text/plain", "Aucune photo"); return; }
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.println("Content-Length: " + String(lastPhoto->len));
  client.println();
  client.write(lastPhoto->buf, lastPhoto->len);
  client.println();
}

void handleRoot() {
  String wifiOptions = getWifiList();
  String page = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Nichoir Config v0.5</title>
<style>
 body{font-family:sans-serif;background:#f0f2f5;padding:20px;text-align:center}
 .card{background:#fff;padding:20px;border-radius:10px;max-width:400px;margin:auto;box-shadow:0 4px 10px rgba(0,0,0,0.1)}
 input,select{width:100%;padding:10px;margin:5px 0;border:1px solid #ccc;border-radius:5px}
 button{width:100%;padding:12px;margin-top:10px;background:#007bff;color:#fff;border:none;border-radius:5px;cursor:pointer}
 button:hover{background:#0056b3}
 h2{color:#333}
</style>
</head>
<body>
<div class="card">
 <h2>Nichoir MQTT</h2>
 <form action="/save" method="POST">
  <label>WiFi</label><select name="ssid">%WIFI_OPTIONS%</select>
  <input name="password" type="password" placeholder="Password">
  <label>MQTT Host (IP Raspberry)</label><input name="mqtt_host" placeholder="192.168.x.x">
  <label>MQTT Port</label><input name="mqtt_port" value="1883">
  <button type="submit">Enregistrer & Redémarrer</button>
 </form>
 <hr>
 <form action="/capture" method="POST">
   <button style="background:#28a745">📸 Prendre Photo + Envoi MQTT</button>
 </form>
</div>
</body>
</html>
)HTML";
  page.replace("%WIFI_OPTIONS%", wifiOptions);
  server.send(200, "text/html", page);
}

// ------------------------------------------------
//      SETUP
// ------------------------------------------------
void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);

  // Init Caméra
  TimerCAM.Power.begin();
  TimerCAM.Rtc.begin();
  TimerCAM.Camera.begin();
  
  // Réglages image (Qualité vs Taille)
  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_framesize(s, FRAMESIZE_QVGA); // 320x240 (léger pour MQTT)
  // s->set_framesize(s, FRAMESIZE_VGA); // 640x480 (plus lourd, tester le buffer)

  // Chargement Config
  bool loaded = loadConfig();

  // Tentative connexion WiFi STA
  if (loaded && connectToWiFi()) {
    server.on("/capture", handleCapture);
    server.on("/photo.jpg", handlePhoto);
    server.on("/info", handleInfo);
    server.on("/", handleRoot); // On garde l'interface même en STA pour tester
    server.on("/save", HTTP_POST, handleSave); // Pour pouvoir modifier la config sans reset
    server.begin();
    Serial.println("Serveur Web actif. IP: " + WiFi.localIP().toString());
    return;
  }

  // Fallback AP Mode
  Serial.println("Fallback AP MODE");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, password_ap);
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void loop() {
  server.handleClient();
}