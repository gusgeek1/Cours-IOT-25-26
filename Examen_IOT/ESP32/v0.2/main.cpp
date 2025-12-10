#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>

using namespace m5;

WebServer server(80);

// WiFi AP
const char* ssid_ap = "APNichoir";
const char* password_ap = "12345678";


// ⭐ Fonction pour scanner les réseaux WiFi
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


// ⭐ Sauvegarder config.json
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

  Serial.println("Config sauvegardée :");
  Serial.println(json);

  server.send(200, "text/html",
    "<html><body><h3>Configuration enregistrée ✓</h3>"
    "Redémarrage dans 2 sec..."
    "<script>setTimeout(()=>{location.href='/'},2000);</script>"
    "</body></html>"
  );

  delay(2000);
  ESP.restart();
}


// ⭐ Streaming vidéo déplacé en /video (diagnostic seulement)
void handleStream() {
  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {

    if (!TimerCAM.Camera.get()) {
      break;
    }

    camera_fb_t* fb = TimerCAM.Camera.fb;

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();
    client.write(fb->buf, fb->len);
    client.println();

    TimerCAM.Camera.free();
    delay(1);
  }
}


// ⭐ Page de configuration stylée + liste des WiFi
void handleRoot() {

  String wifiOptions = getWifiList(); // génère les <option>

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

<label>Hote MQTT</label>
<input name="mqtt_host" placeholder="ex: raspberry.local">

<label>Port MQTT</label>
<input name="mqtt_port" placeholder="1883" value="1883">

<button type="submit">Enregistrer</button>
</form>

<div class="footer">Projet IOT - 2025-2026 - Collard T. & Dejonghe A.</div>
<div class="footer">v0.2</div>

</div>
</body>
</html>
)HTML";

  page.replace("%WIFI_OPTIONS%", wifiOptions);

  server.send(200, "text/html", page);
}


void setup() {
  Serial.begin(115200);
  delay(200);

  SPIFFS.begin(true);

  // ⭐ Initialise TimerCAM (caméra seulement)
  TimerCAM.Power.begin();
  TimerCAM.Rtc.begin();
  TimerCAM.Camera.begin();

  // correction orientation caméra
  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  // ⭐ Mode AP
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);      
  server.on("/video", handleStream);
  server.on("/save", HTTP_POST, handleSave);

  server.begin();
}

void loop() {
  server.handleClient();
}
