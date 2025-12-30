import paho.mqtt.client as mqtt
import os
import mysql.connector
from datetime import datetime
import re # Pour extraire l'ID du topic

# --- CONFIGURATION ---
MQTT_BROKER = "localhost"
MQTT_TOPIC_SUBSCRIPTION = "nichoir/+/photo" # Le + accepte n'importe quel chiffre
DOSSIER_PHOTOS = "photos_nichoir"

# BDD
DB_CONFIG = {
    'user': 'nichoir', 'password': 'nichoir',
    'host': 'localhost', 'database': 'projet_nichoir',
    'unix_socket': '/var/run/mysqld/mysqld.sock'
}

if not os.path.exists(DOSSIER_PHOTOS):
    os.makedirs(DOSSIER_PHOTOS)

def sauver_en_bdd(nichoir_id, filename, chemin_complet):
    try:
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        # 1. On s'assure que le nichoir existe, sinon on le crée
        cursor.execute("SELECT id FROM Nichoirs WHERE id = %s", (nichoir_id,))
        if not cursor.fetchone():
            print(f"⚠️ Nouveau nichoir détecté (ID: {nichoir_id}) ! Création en BDD...")
            cursor.execute("INSERT INTO Nichoirs (id, nom, emplacement) VALUES (%s, %s, %s)", 
                           (nichoir_id, f"Nichoir {nichoir_id}", "Inconnu"))
            conn.commit()

        # 2. On insère la capture
        sql = "INSERT INTO Captures (nichoir_id, image_path, filename, battery_level, event_type) VALUES (%s, %s, %s, %s, %s)"
        # Note: On simulera la batterie à 100% ici si elle n'est pas dans le message
        # Plus tard, on pourra décoder un JSON pour avoir la vraie batterie
        valeurs = (nichoir_id, chemin_complet, filename, 100, "mouvement")
        
        cursor.execute(sql, valeurs)
        conn.commit()
        print(f"📝 Photo du Nichoir {nichoir_id} enregistrée !")
        
        cursor.close()
        conn.close()
    except Exception as e:
        print(f"❌ Erreur BDD : {e}")

def on_message(client, userdata, msg):
    try:
        # Le topic est sous la forme : nichoir/1/photo
        # On utilise une "Expression Régulière" pour extraire le chiffre '1'
        match = re.search(r"nichoir/(\d+)/photo", msg.topic)
        
        if match:
            nichoir_id = int(match.group(1)) # On a trouvé l'ID !
            
            timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
            filename = f"nichoir{nichoir_id}_{timestamp}.jpg"
            chemin = f"{DOSSIER_PHOTOS}/{filename}"
            
            with open(chemin, "wb") as f:
                f.write(msg.payload)
            
            print(f"📸 Reçu de Nichoir {nichoir_id}")
            sauver_en_bdd(nichoir_id, filename, chemin)
        else:
            print(f"Format de topic inconnu: {msg.topic}")
            
    except Exception as e:
        print(f"❌ Erreur réception : {e}")

client = mqtt.Client()
client.on_connect = lambda c, u, f, rc: c.subscribe(MQTT_TOPIC_SUBSCRIPTION)
client.on_message = on_message

print("⏳ Serveur Multi-Nichoirs prêt...")
client.connect(MQTT_BROKER, 1883, 60)
client.loop_forever()
