#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <M5TimerCAM.h>
#include <SPIFFS.h>
#include <PubSubClient.h>

// ==========================================
// 1. VARIABLES & CONFIG
// ==========================================

const char* AP_SSID = "Nichoir_Config_dejonghe";
const char* AP_PASS = "12345678";

String wifi_ssid = "";
String wifi_pass = "";
String mqtt_server = "";
int mqtt_port = 1883; // Port par défaut

WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);

const char* TOPIC_DATA  = "nichoir/data";
const char* TOPIC_PHOTO = "nichoir/photo";
#define TIME_TO_SLEEP  300 

// ==========================================
// 2. LE CODE HTML/CSS DU DESIGN "CAST"
// ==========================================
// Note : J'ai ajouté %WIFI_LIST% là où on veut la liste déroulante
const char html_page_template[] PROGMEM = R"=====(
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

        /* J'ai ajouté 'select' ici pour qu'il ait le même style que input */
        input, select {
            width: 100%;
            background-color: var(--bg-dark);
            border: 1px solid var(--border);
            padding: 15px;
            border-radius: 12px;
            color: white;
            font-size: 1rem;
            margin-bottom: 20px;
            transition: 0.3s;
            appearance: none; /* Pour nettoyer le style par défaut du select */
        }

        /* Petite flèche personnalisée pour le select si possible, sinon standard */
        select {
            cursor: pointer;
            background-image: url("data:image/svg+xml;charset=US-ASCII,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%22292.4%22%20height%3D%22292.4%22%3E%3Cpath%20fill%3D%22%236c5dd3%22%20d%3D%22M287%2069.4a17.6%2017.6%200%200%200-13-5.4H18.4c-5%200-9.3%201.8-12.9%205.4A17.6%2017.6%200%200%200%200%2082.2c0%205%201.8%209.3%205.4%2012.9l128%20127.9c3.6%203.6%207.8%205.4%2012.8%205.4s9.2-1.8%2012.8-5.4L287%2095c3.5-3.5%205.4-7.8%205.4-12.8%200-5-1.9-9.2-5.5-12.8z%22%2F%3E%3C%2Fsvg%3E");
            background-repeat: no-repeat;
            background-position: right 15px top 50%;
            background-size: 12px auto;
        }

        input:focus, select:focus {
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
        <h1>Configuration</h1>
        <div class="subtitle">Sélectionnez votre réseau</div>

        <form action="/save" method="POST">
            <label>Réseau WiFi (SSID)</label>
            <select name="ssid">
                %WIFI_LIST%
            </select>

            <label>Mot de passe WiFi</label>
            <input type="password" name="pass" placeholder="••••••••" required>

            <label>Adresse IP Raspberry Pi (MQTT)</label>
            <input type="text" name="mqtt" placeholder="Ex: 192.168.1.45" required>

            <label>Port MQTT</label>
            <input type="number" name="mqtt_port" value="1883" required>

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

  // Extraction manuelle des données JSON
  int s1 = data.indexOf("\"wifi_ssid\":\"") + 13;
  int s2 = data.indexOf("\"", s1);
  wifi_ssid = data.substring(s1, s2);

  int p1 = data.indexOf("\"wifi_password\":\"") + 17;
  int p2 = data.indexOf("\"", p1);
  wifi_pass = data.substring(p1, p2);

  int h1 = data.indexOf("\"mqtt_host\":\"") + 13;
  int h2 = data.indexOf("\"", h1);
  mqtt_server = data.substring(h1, h2);

  // Extraction du Port (Nombre entier, pas de guillemets)
  int mp1 = data.indexOf("\"mqtt_port\":") + 12;
  int mp2 = data.indexOf("}", mp1); // On suppose que c'est la fin du JSON ou qu'il y a une virgule
  if(mp2 == -1) mp2 = data.indexOf(",", mp1);
  String portStr = data.substring(mp1, mp2);
  mqtt_port = portStr.toInt();
  if(mqtt_port == 0) mqtt_port = 1883; // Sécurité

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
// 4. GESTION DU SERVEUR WEB AVEC SCAN
// ==========================================

// Fonction pour générer la liste des options HTML
String getWifiListHTML() {
    Serial.println("Scan WiFi en cours...");
    int n = WiFi.scanNetworks();
    String options = "";
    
    if (n == 0) {
        options = "<option value=''>Aucun réseau trouvé</option>";
    } else {
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            // On ajoute l'option dans le menu déroulant
            options += "<option value='" + ssid + "'>" + ssid + " (" + WiFi.RSSI(i) + " dBm)</option>";
        }
    }
    return options;
}

void handleRoot() {
  // 1. On récupère la liste des réseaux (ça prend quelques secondes)
  String wifiOptions = getWifiListHTML();

  // 2. On prend le template HTML
  String page = html_page_template;

  // 3. On remplace le placeholder %WIFI_LIST% par les vraies options
  page.replace("%WIFI_LIST%", wifiOptions);

  // 4. On envoie la page complète
  server.send(200, "text/html", page);
}

void handleSave() {
  String s = server.arg("ssid");
  String p = server.arg("pass");
  String m = server.arg("mqtt");
  String port = server.arg("mqtt_port"); // Récupération du port
  
  // Construction du JSON avec le port (entier, sans guillemets)
  String json = "{\"wifi_ssid\":\""+s+"\",\"wifi_password\":\""+p+"\",\"mqtt_host\":\""+m+"\",\"mqtt_port\":"+port+"}";
  
  File file = SPIFFS.open("/config.json", "w");
  file.print(json);
  file.close();
  
  String successHtml = "<body style='background:#131525;color:white;font-family:sans-serif;text-align:center;padding:50px;'>";
  successHtml += "<h1 style='color:#6c5dd3'>Sauvegarde OK !</h1>";
  successHtml += "<p>Le nichoir va redémarrer sur le port " + port + ".</p></body>";
  
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

  // On essaie de se connecter (Mode TRAVAIL)
  if (loadConfig() && connectWiFi()) {
      executerTacheNichoir();
      TimerCAM.Power.timerSleep(TIME_TO_SLEEP);
  }

  // Si on est ici : ÉCHEC -> Mode CONFIGURATION
  Serial.println("Démarrage Mode AP Config...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void loop() {
  server.handleClient();
}