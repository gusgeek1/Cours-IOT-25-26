# v0.1 — TimerCAM Streaming (ESP32 TimerCAM)

## 📌 Description
Cette version **v0.1** constitue la première étape du projet *Nichoir Connecté*.  
Elle met en place un firmware minimal mais fonctionnel pour la caméra **M5 TimerCAM**, permettant :

- la création d’un réseau WiFi en mode **Access Point (AP)**
- l’accès à une interface Web minimale
- l’affichage d’un **flux vidéo en temps réel (MJPEG)**
- la **rotation de l’image** pour un affichage correct

Cette version sert de base de travail pour les futures étapes : configuration, MQTT, PIR, deep sleep…

---

## 🚀 Fonctionnalités

### ✔ AP Mode
Au démarrage, l’ESP32 crée un point d’accès :

- **SSID :** `TimerCam-AP`
- **Mot de passe :** `12345678`
- **Adresse :** http://192.168.4.1

### ✔ Interface Web
En accédant à l’IP de l’ESP, une page simple s’affiche :

- Vidéo en direct via `/stream`

### ✔ Streaming vidéo (MJPEG)
Le flux vidéo est généré par une route HTTP :

```
GET /stream
```

Le navigateur affiche une **vidéo temps réel** (~10–20 FPS selon la luminosité).

### ✔ Rotation verticale
Le capteur OV3660 est monté à l’envers sur la TimerCAM → la rotation est corrigée automatiquement.

---

## 📁 Structure du projet

```
v0.1_streaming/
 ├── platformio.ini
 └── src/
      └── main.cpp
```

- **platformio.ini** : configuration PlatformIO pour carte ESP32CAM + dépendance Timer-CAM  
- **main.cpp** : code du firmware TimerCAM (AP + MJPEG streaming)

---

## 🛠 Dépendances techniques
- PlatformIO  
- Carte : **esp32cam**
- Librairie : `Timer-CAM @ 1.0.1`
- Framework : Arduino

---

## 📄 Notes techniques
- Utilisation de l’objet global `TimerCAM` fourni par la librairie.
- Capture via `TimerCAM.Camera.get()`
- Libération du buffer via `TimerCAM.Camera.free()`
- Correction orientation via `set_vflip()` et `set_hmirror()`

---

## 🎯 Objectif pédagogique
Cette version constitue :

- une **base stable** pour la suite du projet
- un environnement de test pour la caméra et le WiFi
- une démonstration fonctionnelle pour vérifier le matériel

---

## 🔜 Prochaines versions
- **v0.2** : Page de configuration AP (WiFi + MQTT)
- **v0.3** : Connexion WiFi Client
- **v0.4** : PIR + capture auto
- **v0.5** : Publication MQTT
- **v1.0** : Deep sleep + gestion batterie + wake-up PIR
