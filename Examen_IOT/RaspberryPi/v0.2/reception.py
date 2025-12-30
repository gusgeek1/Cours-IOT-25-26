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

# Détermination du dossier de base du script
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Dossier dans lequel les images seront enregistrées
DOSSIER_PHOTOS = os.path.join(BASE_DIR, "static", "photos")

# Paramètres de connexion à la base de données MySQL
DB_CONFIG = {
    'user': 'nichoir',
    'password': 'nichoir',
    'host': 'localhost',
    'database': 'projet_nichoir'
}

# Dictionnaire utilisé pour mémoriser le dernier niveau de batterie reçu par nichoir
last_known_battery = {}

# Création du dossier des photos s’il n’existe pas encore
if not os.path.exists(DOSSIER_PHOTOS):
    os.makedirs(DOSSIER_PHOTOS)

# ------------------------------------------------------------------
# FONCTIONS UTILITAIRES BASE DE DONNÉES
# ------------------------------------------------------------------

# Retourne une connexion à la base de données
def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# Vérifie si un nichoir existe dans la table Nichoirs
# S’il n’existe pas, il est créé automatiquement
def ensure_nichoir_exists(nichoir_id):
    try:
        conn = get_db()
        cursor = conn.cursor()

        # Recherche du nichoir par son identifiant
        cursor.execute("SELECT id FROM Nichoirs WHERE id = %s", (nichoir_id,))
        result = cursor.fetchone()
        
        # Insertion du nichoir s’il n’est pas présent
        if not result:
            sql = """
                INSERT INTO Nichoirs (id, nom, emplacement)
                VALUES (%s, %s, %s)
            """
            cursor.execute(sql, (nichoir_id, f"Nichoir {nichoir_id}", "Jardin"))
            conn.commit()

        conn.close()
    except Exception as e:
        print(f"Erreur SQL Check Nichoir: {e}")

# ------------------------------------------------------------------
# ENREGISTREMENT D’UNE CAPTURE
# ------------------------------------------------------------------

# Enregistre une photo et/ou un événement dans la table Captures
def save_capture(nichoir_id, photo_data=None):
    try:
        # Date et heure actuelles
        now = datetime.now()
        timestamp_str = now.strftime("%Y-%m-%d_%H-%M-%S")

        filename = ""
        image_path = ""

        # Si des données image sont reçues, elles sont enregistrées sur le disque
        if photo_data:
            filename = f"{nichoir_id}_{timestamp_str}.jpg"
            image_path = f"static/photos/{filename}"
            full_path = os.path.join(DOSSIER_PHOTOS, filename)

            with open(full_path, "wb") as f:
                f.write(photo_data)

        # Récupération du dernier niveau de batterie connu pour ce nichoir
        batt_float = last_known_battery.get(nichoir_id, 0.0)

        # Conversion en entier pour correspondre au type de la base de données
        batt_int = int(batt_float)

        # Insertion de la capture dans la table Captures
        conn = get_db()
        cursor = conn.cursor()

        sql = """
            INSERT INTO Captures
            (nichoir_id, date, image_path, filename, battery_level, event_type)
            VALUES (%s, %s, %s, %s, %s, %s)
        """
        cursor.execute(
            sql,
            (nichoir_id, now, image_path, filename, batt_int, "mouvement")
        )

        conn.commit()
        conn.close()

    except Exception as e:
        print(f"Erreur Save: {e}")

# ------------------------------------------------------------------
# CALLBACK MQTT
# ------------------------------------------------------------------

# Fonction appelée automatiquement lors de la réception d’un message MQTT
def on_message(client, userdata, msg):
    try:
        # Découpage du topic MQTT (ex: nichoir/1/photo)
        parts = msg.topic.split('/')
        if len(parts) < 3:
            return

        # Conversion de l’identifiant du nichoir en entier
        try:
            nichoir_id = int(parts[1])
        except ValueError:
            print(f"ID invalide : {parts[1]}")
            return

        msg_type = parts[2]

        # Vérification de l’existence du nichoir dans la base
        ensure_nichoir_exists(nichoir_id)

        # Message de type statut (batterie)
        if msg_type == 'status':
            payload = msg.payload.decode('utf-8')

            # Conversion de la valeur reçue en float
            last_known_battery[nichoir_id] = float(payload)

        # Message de type photo
        elif msg_type == 'photo':
            save_capture(nichoir_id, msg.payload)

    except Exception as e:
        print(f"Erreur globale MQTT: {e}")

# ------------------------------------------------------------------
# INITIALISATION MQTT
# ------------------------------------------------------------------

client = mqtt.Client()

# Association de la fonction de traitement des messages
client.on_message = on_message

# Connexion au broker MQTT
client.connect(MQTT_BROKER, 1883, 60)

# Abonnement aux topics définis
client.subscribe(MQTT_TOPIC)

# Boucle principale bloquante
print("Réception MQTT prête. En attente de messages...")
client.loop_forever()
