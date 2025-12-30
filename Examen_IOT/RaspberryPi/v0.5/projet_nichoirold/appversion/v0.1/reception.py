import paho.mqtt.client as mqtt
import os
import mysql.connector
from datetime import datetime

# --- CONFIGURATION ---
MQTT_BROKER = "localhost"
MQTT_TOPIC = "nichoir/+/photo" # Le + remplace le numéro
DOSSIER_PHOTOS = "/home/augustin/projet_nichoir/photos_nichoir" # Chemin ABSOLU pour éviter les erreurs

# Config BDD
DB_CONFIG = {
    'user': 'nichoir', 'password': 'nichoir',
    'host': 'localhost', 'database': 'projet_nichoir',
    'unix_socket': '/var/run/mysqld/mysqld.sock'
}

# Création dossier si inexistant
if not os.path.exists(DOSSIER_PHOTOS):
    os.makedirs(DOSSIER_PHOTOS)

def sauver_en_bdd(nichoir_id, filename):
    try:
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        # Vérifie si le nichoir existe, sinon le crée
        cursor.execute("SELECT id FROM Nichoirs WHERE id = %s", (nichoir_id,))
        if not cursor.fetchone():
            print(f"✨ Nouveau Nichoir détecté (ID: {nichoir_id})")
            cursor.execute("INSERT INTO Nichoirs (id, nom, emplacement) VALUES (%s, %s, %s)", 
                           (nichoir_id, f"Nichoir {nichoir_id}", "Jardin"))
            conn.commit()

        # Enregistre la photo
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        sql = "INSERT INTO Captures (nichoir_id, filename, date, battery_level, event_type) VALUES (%s, %s, %s, %s, %s)"
        cursor.execute(sql, (nichoir_id, filename, timestamp, 100, "mouvement"))
        conn.commit()
        
        print(f"✅ BDD mise à jour pour Nichoir {nichoir_id}")
        conn.close()
    except Exception as e:
        print(f"❌ Erreur SQL : {e}")

def on_connect(client, userdata, flags, rc):
    print(f"🔌 Connecté au Broker (Code: {rc})")
    client.subscribe(MQTT_TOPIC)
    print(f"👂 Écoute sur : {MQTT_TOPIC}")

def on_message(client, userdata, msg):
    print(f"📩 Message reçu ! Sujet: {msg.topic}")
    
    try:
        # On découpe : "nichoir/1/photo" devient ["nichoir", "1", "photo"]
        parts = msg.topic.split('/')
        
        # Vérification simple
        if len(parts) == 3 and parts[2] == 'photo':
            nichoir_id = parts[1] # On prend le chiffre au milieu
            
            timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
            filename = f"nichoir{nichoir_id}_{timestamp}.jpg"
            chemin_complet = os.path.join(DOSSIER_PHOTOS, filename)
            
            # Sauvegarde Fichier
            with open(chemin_complet, "wb") as f:
                f.write(msg.payload)
            print(f"💾 Image sauvegardée : {filename}")
            
            # Sauvegarde BDD
            sauver_en_bdd(nichoir_id, filename)
            
        else:
            print("⚠️ Format du sujet incorrect")
            
    except Exception as e:
        print(f"❌ Erreur Traitement : {e}")

# Lancement
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("🚀 Script de réception démarré...")
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()
