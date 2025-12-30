#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5TimerCAM.h>
#include <FS.h>
#include <SPIFFS.h>

using namespace m5;

WebServer server(80);

const char* ssid = "Test1234";
const char* password = "12345678";

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

  server.send(200, "text/html",
    "<html><body><h3>Configuration enregistrée ✓</h3>"
    "Redémarrage dans 2 sec..."
    "<script>setTimeout(()=>{location.href='/'},2000);</script>"
    "</body></html>"
  );

  delay(2000);
  ESP.restart();
}

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

void handleRoot() {
  server.send(200, "text/html", R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Nichoir – Configuration</title>
<style>
    body {
        font-family: Arial, sans-serif;
        background: linear-gradient(135deg, #e8f0ff, #ffffff);
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
    }
    .card {
        background: #fff;
        padding: 25px;
        width: 90%;
        max-width: 380px;
        border-radius: 16px;
        box-shadow: 0 8px 20px rgba(0,0,0,0.15);
        animation: fadein 0.6s ease;
    }
    @keyframes fadein {
        from { opacity: 0; transform: translateY(20px); }
        to   { opacity: 1; transform: translateY(0); }
    }
    h2 {
        margin-top: 0;
        text-align: center;
        color: #2a4d9b;
        font-size: 26px;
    }
    label {
        font-weight: bold;
        color: #4a4a4a;
        margin-top: 10px;
        display: block;
    }
    input {
        width: 100%;
        padding: 12px;
        margin-top: 5px;
        border: 1px solid #d0d0d0;
        border-radius: 8px;
        font-size: 15px;
        background-color: #f7f9fc;
        transition: all 0.2s;
    }
    input:focus {
        border-color: #3d7bff;
        background: #fff;
        outline: none;
        box-shadow: 0 0 5px rgba(61,123,255,0.4);
    }
    button {
        width: 100%;
        margin-top: 20px;
        padding: 14px;
        font-size: 16px;
        color: white;
        background: #3d7bff;
        border: none;
        border-radius: 10px;
        cursor: pointer;
        transition: 0.2s;
    }
    button:hover {
        background: #2b62d4;
        transform: scale(1.02);
    }
    .footer {
        margin-top: 10px;
        text-align: center;
        font-size: 12px;
        color: #777;
    }
</style>
</head>
<body>
<div class="card">
    <h2>Configuration Nichoir</h2>
    <form action="/save" method="POST">
        <label>WiFi SSID</label>
        <input name="ssid" placeholder="Nom du WiFi">
        <label>WiFi Password</label>
        <input name="password" type="password" placeholder="Mot de passe">
        <label>MQTT Host</label>
        <input name="mqtt_host" placeholder="ex: 192.168.1.10">
        <label>MQTT Port</label>
        <input name="mqtt_port" placeholder="1883" value="1883">
        <button type="submit">Enregistrer</button>
    </form>
    <div class="footer">Nichoir Connecté – v0.2</div>
</div>
</body>
</html>
)HTML");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  SPIFFS.begin(true);

  TimerCAM.begin();
  TimerCAM.Camera.begin();

  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/video", handleStream);
  server.on("/save", HTTP_POST, handleSave);

  server.begin();
}

void loop() {
  server.handleClient();
}
