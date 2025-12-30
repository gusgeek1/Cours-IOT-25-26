import paho.mqtt.client as mqtt
import os
import mysql.connector
from datetime import datetime
import time

# ------------------------------------------------------------------
# CONFIGURATION GÉNÉRALE
# ------------------------------------------------------------------

# Adresse du broker MQTT
MQTT_BROKER = "localhost"

# Abonnement à tous les topics liés aux nichoirs
MQTT_TOPIC = "nichoir/#"

# Dossier de base du script
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Dossier dans lequel les images seront stockées
DOSSIER_PHOTOS = os.path.join(BASE_DIR, "static", "photos")

# Paramètres de connexion à la base de données MySQL
DB_CONFIG = {
    'user': 'nichoir',
    'password': 'nichoir',
    'host': 'localhost',
    'database': 'projet_nichoir'
}

# Dictionnaire permettant de mémoriser la dernière valeur de batterie reçue
last_known_battery = {}

# Création du dossier de stockage des photos s’il n’existe pas
if not os.path.exists(DOSSIER_PHOTOS):
    os.makedirs(DOSSIER_PHOTOS)

# ------------------------------------------------------------------
# FONCTIONS UTILITAIRES BASE DE DONNÉES
# ------------------------------------------------------------------

# Retourne une connexion active à la base de données
def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# Vérifie si un nichoir existe dans la table Nichoirs
# Si ce n’est pas le cas, une entrée minimale est créée
def ensure_nichoir_exists(nichoir_id):
    try:
        conn = get_db()
        cursor = conn.cursor()

        # Recherche du nichoir par son identifiant
        cursor.execute(
            "SELECT id FROM Nichoirs WHERE id = %s",
            (nichoir_id,)
        )

        # Création du nichoir s’il n’existe pas encore
        if not cursor.fetchone():
            cursor.execute(
                "INSERT INTO Nichoirs (id, nom, battery_level) VALUES (%s, %s, %s)",
                (nichoir_id, f"Nichoir {nichoir_id}", 0.0)
            )
            conn.commit()

        conn.close()

    except Exception as e:
        print(f"Erreur SQL Check: {e}")

# ------------------------------------------------------------------
# MISE À JOUR RAPIDE DU DASHBOARD
# ------------------------------------------------------------------

# Met à jour uniquement le niveau de batterie du nichoir
def update_dashboard_battery(nichoir_id, voltage):
    try:
        conn = get_db()
        cursor = conn.cursor()

        sql_update = """
            UPDATE Nichoirs
            SET battery_level = %s
            WHERE id = %s
        """
        cursor.execute(sql_update, (voltage, nichoir_id))
        conn.commit()
        conn.close()

    except Exception as e:
        print(f"Erreur Update Rapide: {e}")

# ------------------------------------------------------------------
# ENREGISTREMENT D’UNE CAPTURE
# ------------------------------------------------------------------

# Enregistre une capture dans l’historique avec une gestion des erreurs
def save_capture(nichoir_id, photo_data=None):
    attempts = 0

    # Date et heure utilisées pour le nom de fichier et la base de données
    now = datetime.now()
    timestamp_str = now.strftime("%Y-%m-%d_%H-%M-%S")

    filename = ""
    image_path = ""

    # Préparation des informations liées à l’image
    if photo_data:
        filename = f"{nichoir_id}_{timestamp_str}.jpg"
        image_path = f"static/photos/{filename}"

    # Tentatives multiples pour gérer les conflits d’accès à la base
    while attempts < 3:
        try:
            # Sauvegarde du fichier image sur le disque
            if photo_data:
                full_path = os.path.join(DOSSIER_PHOTOS, filename)

                # Écriture uniquement si le fichier n’existe pas déjà
                if not os.path.exists(full_path):
                    with open(full_path, "wb") as f:
                        f.write(photo_data)

            # Récupération de la dernière valeur de batterie connue
            batt_val = last_known_battery.get(nichoir_id, 0.0)

            conn = get_db()
            cursor = conn.cursor()

            # Insertion dans la table Captures (historique)
            sql = """
                INSERT INTO Captures
                (nichoir_id, date, image_path, filename, battery_level, event_type)
                VALUES (%s, %s, %s, %s, %s, %s)
            """
            cursor.execute(
                sql,
                (nichoir_id, now, image_path, filename, batt_val, "mouvement")
            )

            # Mise à jour de l’état courant dans la table Nichoirs
            sql_update = """
                UPDATE Nichoirs
                SET battery_level = %s
                WHERE id = %s
            """
            cursor.execute(sql_update, (batt_val, nichoir_id))

            conn.commit()
            conn.close()
            break

        except mysql.connector.Error as err:
            # Gestion des erreurs SQL spécifiques
            if err.errno == 1062:
                # Cas d’un enregistrement déjà existant
                break

            elif err.errno == 1213:
                # Conflit d’accès concurrent à la base
                time.sleep(0.5)
                attempts += 1
            else:
                print(f"Erreur SQL Save: {err}")
                break

        except Exception as e:
            print(f"Erreur Générale: {e}")
            break

# ------------------------------------------------------------------
# CALLBACK MQTT
# ------------------------------------------------------------------

# Fonction appelée automatiquement lors de la réception d’un message MQTT
def on_message(client, userdata, msg):
    try:
        # Découpage du topic MQTT (exemple : nichoir/1/batterie)
        parts = msg.topic.split('/')
        if len(parts) < 3:
            return

        try:
            nichoir_id = int(parts[1])
        except:
            return

        msg_type = parts[2]

        # Vérification de l’existence du nichoir
        ensure_nichoir_exists(nichoir_id)

        # Message contenant l’état de la batterie
        if msg_type == 'batterie':
            payload = msg.payload.decode('utf-8')
            try:
                valeur = float(payload)
                last_known_battery[nichoir_id] = valeur
                update_dashboard_battery(nichoir_id, valeur)
            except:
                pass

        # Message contenant une photo
        elif msg_type == 'photo':
            save_capture(nichoir_id, msg.payload)

    except Exception as e:
        print(f"Erreur MQTT: {e}")

# ------------------------------------------------------------------
# INITIALISATION DU CLIENT MQTT
# ------------------------------------------------------------------

client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC)

# Boucle principale bloquante
print("Service MQTT actif. En attente de messages...")
client.loop_forever()
