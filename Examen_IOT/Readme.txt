PROJET NICHOIR CONNECTÉ
ESP32 TimerCAM – Projet IoT

========================================
1. PRÉSENTATION DU PROJET
========================================

Ce projet consiste à développer un nichoir connecté autonome basé sur un ESP32
TimerCAM. Le système est capable de détecter un mouvement à l’aide d’un capteur PIR,
de capturer une image infrarouge et de transmettre des données vers un serveur distant
via le protocole MQTT.

L’objectif principal est de concevoir un dispositif basse consommation, configurable
à distance, capable de fonctionner sur batterie et de transmettre uniquement des
informations utiles afin d’optimiser l’autonomie énergétique.

========================================
2. MATÉRIEL UTILISÉ
========================================

- ESP32 TimerCAM
- Capteur de mouvement PIR
- LED infrarouge (vision nocturne)
- Batterie Li-Ion
- Serveur MQTT (ex: Raspberry Pi)
- Réseau WiFi local

========================================
3. FONCTIONNALITÉS PRINCIPALES
========================================

- Interface Web de configuration (WiFi + MQTT)
- Stockage de la configuration dans SPIFFS
- Détection de mouvement par capteur PIR
- Capture d’image avec la caméra embarquée
- Transmission MQTT de la photo
- Envoi périodique du niveau de batterie
- Fonctionnement autonome sur batterie
- Gestion du deep sleep pour réduire la consommation

========================================
4. PRINCIPE DE FONCTIONNEMENT
========================================

Au démarrage, le nichoir vérifie la présence d’une configuration enregistrée.
Si aucune configuration n’est trouvée, le système démarre en mode point d’accès
afin de permettre la configuration via une interface Web.

En fonctionnement normal, le microcontrôleur reste en sommeil profond.
Il se réveille périodiquement pour vérifier la présence d’un mouvement.
En cas de détection, une image est capturée et envoyée via MQTT, puis le système
retourne en sommeil profond.

========================================
5. ÉVOLUTION DU PROJET
========================================

Le projet a été développé de manière incrémentale, en ajoutant progressivement
les fonctionnalités suivantes :

- Interface Web basique
- Mode point d’accès
- Mode station WiFi
- Envoi MQTT
- Capture d’image
- Optimisation énergétique
- Gestion avancée du deep sleep

L’historique détaillé des versions est disponible dans le fichier CHANGELOG.txt.

========================================
6. AUTEURS
========================================

Projet réalisé par :
- Collard T.
- Dejonghe A.

Année académique : 2025 – 2026
