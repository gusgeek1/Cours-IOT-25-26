#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5TimerCAM.h>
#include <SPIFFS.h>
#include <PubSubClient.h>

// ==========================================
// 1. VARIABLES & CONFIG
// ==========================================

const char* AP_SSID = "Nichoir_Config";
const char* AP_PASS = "12345678";

String wifi_ssid = "";
String wifi_pass = "";
String mqtt_server = "";
int mqtt_port = 1883;

WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

const char* TOPIC_DATA  = "nichoir/data";
const char* TOPIC_PHOTO = "nichoir/photo";
#define TIME_TO_SLEEP  300 

// ==========================================
// 2. LE CODE HTML/CSS DU DESIGN "CAST"
// ==========================================
// Nous utilisons R"=====( ... )=====" pour mettre le HTML brut
const char html_page_config[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Setup Nichoir</title>
    <style>
        /* --- DESIGN CAST (SaaS Dark Mode) --- */
        :root {
            --bg-dark: #131525;
            --bg-panel: #1e1f3a;
            --primary: #6c5dd3;
            --primary-hover: #5a4cb5;
            --text-main: #ffffff;
            --text-muted: #8e92bc;
            --border: #2d2f4e;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', sans-serif; }
        
        body {
            background-color: var(--bg-dark);
            color: var(--text-main);
            display: flex;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            padding: 20px;
        }

        .card {
            background-color: var(--bg-panel);
            width: 100%;
            max-width: 400px;
            padding: 40px;
            border-radius: 20px;
            box-shadow: 0 20px 50px rgba(0,0,0,0.3);
            text-align: center;
            border: 1px solid var(--border);
        }

        h1 { margin-bottom: 10px; font-size: 1.8rem; letter-spacing: 1px; }
        .subtitle { color: var(--text-muted); font-size: 0.9rem; margin-bottom: 30px; }
        
        /* Icone style CAST */
        .icon-box {
            width: 60px; height: 60px;
            background: rgba(108, 93, 211, 0.1);
            border-radius: 15px;
            display: flex; align-items: center; justify-content: center;
            margin: 0 auto 20px auto;
            color: var(--primary);
            font-size: 24px; font-weight: bold;
        }

        label {
            display: block;
            text-align: left;
            margin-bottom: 8px;
            color: var(--text-muted);
            font-size: 0.85rem;
            font-weight: 600;
            text-transform: uppercase;
        }

        input {
            width: 100%;
            background-color: var(--bg-dark);
            border: 1px solid var(--border);
            padding: 15px;
            border-radius: 12px;
            color: white;
            font-size: 1rem;
            margin-bottom: 20px;
            transition: 0.3s;
        }

        input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 3px rgba(108, 93, 211, 0.2);
        }

        button {
            background-color: var(--primary);
            color: white;
            border: none;
            width: 100%;
            padding: 16px;
            border-radius: 12px;
            font-size: 1rem;
            font-weight: bold;
            cursor: pointer;
            transition: 0.3s;
            margin-top: 10px;
        }

        button:hover { background-color: var(--primary-hover); transform: translateY(-2px); }

        .footer { margin-top: 30px; color: var(--text-muted); font-size: 0.75rem; }
    </style>
</head>
<body>
    <div class="card">
        <div class="icon-box">IoT</div>
        <h1>Bienvenue !</h1>
        <div class="subtitle">Paramètres de connexion au serveur</div>

        <form action="/save" method="POST">
            <label>Nom du WiFi (SSID)</label>
            <input type="text" name="ssid" placeholder="Ex: VOO-123456" required>

            <label>Mot de passe WiFi</label>
            <input type="password" name="pass" placeholder="••••••••" required>

            <label>Adresse IP Raspberry Pi (MQTT)</label>
            <input type="text" name="mqtt" placeholder="Ex: 192.168.1.45" required>

            <button type="submit">SAUVEGARDER & DÉMARRER</button>
        </form>

        <div class="footer">Projet IOT • 2025-2026 • Collard T. & Dejonghe A.</div>
    </div>
</body>
</html>
)=====";


// ==========================================
// 3. FONCTIONS SYSTÈME
// ==========================================

bool loadConfig() {
  if (!SPIFFS.exists("/config.json")) return false;
  File file = SPIFFS.open("/config.json", "r");
  String data = file.readString();
  file.close();

  int s1 = data.indexOf("\"wifi_ssid\":\"") + 13;
  int s2 = data.indexOf("\"", s1);
  wifi_ssid = data.substring(s1, s2);

  int p1 = data.indexOf("\"wifi_password\":\"") + 17;
  int p2 = data.indexOf("\"", p1);
  wifi_pass = data.substring(p1, p2);

  int h1 = data.indexOf("\"mqtt_host\":\"") + 13;
  int h2 = data.indexOf("\"", h1);
  mqtt_server = data.substring(h1, h2);

  return true;
}

bool connectWiFi() {
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(100);
    attempts++;
  }
  return (WiFi.status() == WL_CONNECTED);
}

float getBatteryVoltage() {
    return (float)(TimerCAM.Power.getBatteryVoltage()) / 1000.0;
}

void executerTacheNichoir() {
  client.setServer(mqtt_server.c_str(), mqtt_port);
  client.setBufferSize(20000); 

  if (!client.connect("NichoirCam")) return;

  if (TimerCAM.Camera.get()) {
    String json = "{\"id\":\"NCH-001\",\"batterie\":" + String(getBatteryVoltage()) + "}";
    client.publish(TOPIC_DATA, json.c_str());

    client.beginPublish(TOPIC_PHOTO, TimerCAM.Camera.fb->len, false);
    client.write(TimerCAM.Camera.fb->buf, TimerCAM.Camera.fb->len);
    client.endPublish();
    TimerCAM.Camera.free();
  }
  client.disconnect();
}

// ==========================================
// 4. GESTION DU SERVEUR WEB (DESIGN PRO)
// ==========================================

void handleRoot() {
  // On sert la page HTML propre définie plus haut
  server.send(200, "text/html", html_page_config);
}

void handleSave() {
  String s = server.arg("ssid");
  String p = server.arg("pass");
  String m = server.arg("mqtt");
  
  String json = "{\"wifi_ssid\":\""+s+"\",\"wifi_password\":\""+p+"\",\"mqtt_host\":\""+m+"\"}";
  
  File file = SPIFFS.open("/config.json", "w");
  file.print(json);
  file.close();
  
  // Page de confirmation simple mais stylée (inline pour simplifier)
  String successHtml = "<body style='background:#131525;color:white;font-family:sans-serif;text-align:center;padding:50px;'>";
  successHtml += "<h1 style='color:#6c5dd3'>Sauvegarde OK !</h1>";
  successHtml += "<p>Le nichoir va redémarrer et tenter de se connecter.</p></body>";
  
  server.send(200, "text/html", successHtml);
  delay(1000);
  ESP.restart();
}

// ==========================================
// 5. SETUP & LOOP
// ==========================================

void setup() {
  Serial.begin(115200);
  SPIFFS.begin(true);
  
  TimerCAM.Power.begin();
  TimerCAM.Camera.begin();
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SVGA);

  if (loadConfig() && connectWiFi()) {
      executerTacheNichoir();
      TimerCAM.Power.timerSleep(TIME_TO_SLEEP);
  }

  // Si échec -> Mode AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void loop() {
  server.handleClient();
}