import paho.mqtt.client as mqtt
import os
import mysql.connector
from datetime import datetime

# ------------------------------------------------------------------
# CONFIGURATION GÉNÉRALE
# ------------------------------------------------------------------

# Adresse du broker MQTT
MQTT_BROKER = "localhost"

# Abonnement à tous les topics liés aux nichoirs
MQTT_TOPIC = "nichoir/#"

# Dossier de base du script
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Dossier dans lequel les photos seront stockées
DOSSIER_PHOTOS = os.path.join(BASE_DIR, "static", "photos")

# Paramètres de connexion à la base de données MySQL
DB_CONFIG = {
    'user': 'nichoir',
    'password': 'nichoir',
    'host': 'localhost',
    'database': 'projet_nichoir'
}

# Dictionnaire permettant de mémoriser le dernier niveau de batterie reçu
last_known_battery = {}

# Création du dossier des photos s’il n’existe pas
if not os.path.exists(DOSSIER_PHOTOS):
    os.makedirs(DOSSIER_PHOTOS)

# ------------------------------------------------------------------
# FONCTIONS UTILITAIRES BASE DE DONNÉES
# ------------------------------------------------------------------

# Retourne une connexion active à la base de données
def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# Met à jour l’état courant d’un nichoir dans la table Nichoirs
# Les informations mises à jour sont le niveau de batterie et la dernière activité
def update_nichoir_status(nichoir_id, voltage):
    try:
        conn = get_db()
        cursor = conn.cursor()

        # Date et heure actuelles
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Conversion de la valeur reçue en nombre flottant
        volts = float(voltage)

        # Insertion ou mise à jour du nichoir selon son existence
        sql = """
        INSERT INTO Nichoirs (id, nom, battery_level, last_seen)
        VALUES (%s, %s, %s, %s)
        ON DUPLICATE KEY UPDATE
            battery_level = %s,
            last_seen = %s
        """
        cursor.execute(
            sql,
            (nichoir_id, f"Nichoir {nichoir_id}", volts, now, volts, now)
        )

        conn.commit()
        conn.close()

    except Exception as e:
        print(f"Erreur SQL Status: {e}")

# ------------------------------------------------------------------
# SAUVEGARDE D’UNE PHOTO
# ------------------------------------------------------------------

# Sauvegarde la photo reçue sur le disque et enregistre l’événement en base
def save_photo(nichoir_id, photo_data):
    try:
        # Date et heure actuelles
        now = datetime.now()

        # Génération d’un nom de fichier unique
        filename = f"{nichoir_id}_{now.strftime('%Y-%m-%d_%H-%M-%S')}.jpg"
        filepath = os.path.join(DOSSIER_PHOTOS, filename)

        # Écriture des données binaires de l’image dans un fichier
        with open(filepath, "wb") as f:
            f.write(photo_data)

        # Récupération du dernier niveau de batterie connu
        batt = last_known_battery.get(nichoir_id, 0.0)

        # Insertion de la capture dans la table Captures
        conn = get_db()
        cursor = conn.cursor()
        sql = """
            INSERT INTO Captures
            (nichoir_id, filename, date, battery_level, event_type)
            VALUES (%s, %s, %s, %s, %s)
        """
        cursor.execute(sql, (nichoir_id, filename, now, batt, "mouvement"))
        conn.commit()
        conn.close()

    except Exception as e:
        print(f"Erreur Save Photo: {e}")

# ------------------------------------------------------------------
# CALLBACK MQTT
# ------------------------------------------------------------------

# Fonction appelée automatiquement lors de la réception d’un message MQTT
def on_message(client, userdata, msg):
    try:
        # Découpage du topic MQTT (exemple : nichoir/1/status)
        parts = msg.topic.split('/')
        if len(parts) < 3:
            return

        nichoir_id = parts[1]
        type_msg = parts[2]

        # Message contenant l’état de la batterie
        if type_msg == 'status':
            payload = msg.payload.decode('utf-8')

            try:
                voltage = float(payload)

                # Correction éventuelle si la valeur est transmise en millivolts
                if voltage > 100:
                    voltage = voltage / 1000.0

                last_known_battery[nichoir_id] = voltage
                update_nichoir_status(nichoir_id, voltage)

            except:
                pass

        # Message contenant une photo
        elif type_msg == 'photo':
            save_photo(nichoir_id, msg.payload)

    except Exception as e:
        print(f"Erreur MQTT globale: {e}")

# ------------------------------------------------------------------
# INITIALISATION DU CLIENT MQTT
# ------------------------------------------------------------------

client = mqtt.Client()

# Association de la fonction de traitement des messages
client.on_message = on_message

# Connexion au broker MQTT
client.connect(MQTT_BROKER, 1883, 60)

# Abonnement aux topics définis
client.subscribe(MQTT_TOPIC)

# Boucle principale bloquante
print("Réception MQTT active. En attente de messages...")
client.loop_forever()
