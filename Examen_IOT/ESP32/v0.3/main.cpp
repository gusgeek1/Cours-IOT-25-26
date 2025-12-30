#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>

using namespace m5;

WebServer server(80);

// AP MODE
const char* ssid_ap = "APNichoir";
const char* password_ap = "12345678";

// CONFIG VARS (from config.json)
String wifi_ssid = "";
String wifi_password = "";
String mqtt_host = "";
int mqtt_port = 1883;

// ------------------------------------------------
//      SCAN WIFI (GARDÉ DE LA v2)
// ------------------------------------------------
String getWifiList() {
  int n = WiFi.scanNetworks();
  String options = "";

  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    options += "<option value='" + ssid + "'>" + ssid + "</option>";
  }

  if (n == 0) {
    options = "<option value=''>Aucun réseau détecté</option>";
  }

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
  if (!file) {
    Serial.println("Impossible d'ouvrir config.json");
    return false;
  }

  String data = file.readString();
  file.close();

  Serial.println("Config détectée :");
  Serial.println(data);

  // Simple parsing (sans lib JSON)
  int s1 = data.indexOf("\"wifi_ssid\":\"");
  int s2 = data.indexOf("\"", s1 + 14);
  wifi_ssid = data.substring(s1 + 14, s2);

  int p1 = data.indexOf("\"wifi_password\":\"");
  int p2 = data.indexOf("\"", p1 + 18);
  wifi_password = data.substring(p1 + 18, p2);

  int h1 = data.indexOf("\"mqtt_host\":\"");
  int h2 = data.indexOf("\"", h1 + 13);
  mqtt_host = data.substring(h1 + 13, h2);

  int mp1 = data.indexOf("\"mqtt_port\":");
  int mp2 = data.indexOf("}", mp1);
  mqtt_port = data.substring(mp1 + 12, mp2).toInt();

  return true;
}

// ------------------------------------------------
//      CONNECT TO WIFI (STA MODE)
// ------------------------------------------------
bool connectToWiFi() {

  Serial.println("Connexion WiFi…");
  Serial.println("SSID : " + wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  int timeout = 0;

  while (WiFi.status() != WL_CONNECTED && timeout < 100) {
    delay(100);
    timeout++;
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✔ Connecté au WiFi !");
    Serial.print("IP : ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("❌ Échec connexion → retour en AP");
  return false;
}

// ------------------------------------------------
//      ROUTE /info EN MODE STA
// ------------------------------------------------
void handleInfo() {
  String json = "{";
  json += "\"mode\":\"STA\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"wifi_ssid\":\"" + wifi_ssid + "\",";
  json += "\"mqtt_host\":\"" + mqtt_host + "\",";
  json += "\"mqtt_port\":" + String(mqtt_port) + ",";
  json += "\"connected\":true";
  json += "}";
  server.send(200, "application/json", json);
}

// ------------------------------------------------
//      SAVE CONFIG (identique v2)
// ------------------------------------------------
void handleSave() {

  if (!server.hasArg("ssid") || !server.hasArg("password") ||
      !server.hasArg("mqtt_host") || !server.hasArg("mqtt_port")) {

    server.send(400, "text/plain", "Missing fields");
    return;
  }

  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  String host = server.arg("mqtt_host");
  String port = server.arg("mqtt_port");

  String json = "{";
  json += "\"wifi_ssid\":\"" + ssid + "\",";
  json += "\"wifi_password\":\"" + pass + "\",";
  json += "\"mqtt_host\":\"" + host + "\",";
  json += "\"mqtt_port\":" + port;
  json += "}";

  File file = SPIFFS.open("/config.json", "w");
  if (!file) {
    server.send(500, "text/plain", "Erreur écriture fichier");
    return;
  }

  file.print(json);
  file.close();

  Serial.println("Config sauvegardée !");
  Serial.println(json);

  server.send(200, "text/html", "<h3>Configuration sauvegardée ✓</h3>Redémarrage...");
  delay(1500);
  ESP.restart();
}

// ------------------------------------------------
//      PAGE CONFIGURATION (v2 complète)
// ------------------------------------------------
void handleRoot() {

  String wifiOptions = getWifiList();

  String page = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Nichoir – Configuration</title>

<style>
    body {
        font-family: Arial, sans-serif;
        background: linear-gradient(135deg, #e8f0ff, #ffffff);
        margin: 0; padding: 0;
        display: flex; justify-content: center; align-items: center;
        height: 100vh;
    }
    .card {
        background: #fff;
        padding: 25px;
        width: 90%; max-width: 380px;
        border-radius: 16px;
        box-shadow: 0 8px 20px rgba(0,0,0,0.15);
        animation: fadein 0.6s ease;
    }
    @keyframes fadein {
        from { opacity: 0; transform: translateY(20px); }
        to   { opacity: 1; transform: translateY(0); }
    }
    h2 {
        margin-top: 0; text-align: center;
        color: #2a4d9b; font-size: 26px;
    }
    label {
        font-weight: bold; color: #4a4a4a;
        margin-top: 10px; display: block;
    }
    select, input {
        width: 100%; padding: 12px;
        margin-top: 5px;
        border: 1px solid #d0d0d0;
        border-radius: 8px;
        font-size: 15px;
        background-color: #f7f9fc;
        transition: all 0.2s;
    }
    select:focus, input:focus {
        border-color: #3d7bff;
        background: #fff;
        outline: none;
        box-shadow: 0 0 5px rgba(61,123,255,0.4);
    }
    button {
        width: 100%; margin-top: 20px; padding: 14px;
        font-size: 16px; color: white;
        background: #3d7bff; border: none;
        border-radius: 10px; cursor: pointer;
        transition: 0.2s;
    }
    button:hover {
        background: #2b62d4; transform: scale(1.02);
    }
    .footer { margin-top: 10px; text-align: center; font-size: 12px; color: #777; }
</style>
</head>

<body>
<div class="card">

<div style="text-align:center;margin-bottom:10px;">
<svg width="60" height="60" viewBox="0 0 24 24" fill="#3d7bff">
  <path d="M12 3L2 9l10 6 10-6-10-6zm0 11l-7-4.2V17l7 4 7-4V9.8L12 14z"/>
</svg>
</div>

<h2>Configuration Nichoir</h2>

<form action="/save" method="POST">

<label>Wi-Fi</label>
<select name="ssid">%WIFI_OPTIONS%</select>

<label>Mot de passe Wi-Fi</label>
<input name="password" type="password" placeholder="Mot de passe">

<label>Hôte MQTT</label>
<input name="mqtt_host" placeholder="ex: raspberry.local">

<label>Port MQTT</label>
<input name="mqtt_port" placeholder="1883" value="1883">

<button type="submit">Enregistrer</button>
</form>

<div class="footer">Projet IOT - 2025-2026 - Collard T. & Dejonghe A.</div>
<div class="footer">v0.3</div>

</div>
</body>
</html>
)HTML";

  page.replace("%WIFI_OPTIONS%", wifiOptions);
  server.send(200, "text/html", page);
}

// ------------------------------------------------
//                    SETUP
// ------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  SPIFFS.begin(true);

  // Init TimerCAM (caméra uniquement)
  TimerCAM.Power.begin();
  TimerCAM.Rtc.begin();
  TimerCAM.Camera.begin();

  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  // 1️⃣ Lire la config
  bool loaded = loadConfig();

  // 2️⃣ Essayer WiFi STA
  if (loaded && connectToWiFi()) {

    server.on("/info", handleInfo);
    server.begin();

    Serial.println("Mode STA opérationnel.");
    return;
  }

  // 3️⃣ Sinon → AP
  Serial.println("Fallback en MODE AP");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, password_ap);

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  server.begin();
}

// ------------------------------------------------
//                    LOOP
// ------------------------------------------------
void loop() {
  server.handleClient();
}
