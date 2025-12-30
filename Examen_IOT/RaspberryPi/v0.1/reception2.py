import paho.mqtt.client as mqtt
import os
import mysql.connector
from datetime import datetime
import re  # Utilisé pour analyser le topic MQTT et récupérer l’identifiant du nichoir

# ------------------------------------------------------------------
# CONFIGURATION GÉNÉRALE
# ------------------------------------------------------------------

# Adresse du broker MQTT (ici le Raspberry Pi en local)
MQTT_BROKER = "localhost"

# Topic MQTT écouté :
# le symbole '+' permet de recevoir les messages de tous les nichoirs
MQTT_TOPIC_SUBSCRIPTION = "nichoir/+/photo"

# Dossier dans lequel les photos reçues seront stockées
DOSSIER_PHOTOS = "photos_nichoir"

# Paramètres de connexion à la base de données MySQL
DB_CONFIG = {
    'user': 'nichoir',
    'password': 'nichoir',
    'host': 'localhost',
    'database': 'projet_nichoir',
    'unix_socket': '/var/run/mysqld/mysqld.sock'
}

# Création du dossier de stockage s’il n’existe pas encore
if not os.path.exists(DOSSIER_PHOTOS):
    os.makedirs(DOSSIER_PHOTOS)

# ------------------------------------------------------------------
# FONCTION D’ENREGISTREMENT EN BASE DE DONNÉES
# ------------------------------------------------------------------

def sauver_en_bdd(nichoir_id, filename, chemin_complet):
    try:
        # Connexion à la base de données
        conn = mysql.connector.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        # Vérifie si le nichoir existe déjà dans la table Nichoirs
        cursor.execute("SELECT id FROM Nichoirs WHERE id = %s", (nichoir_id,))
        
        # Si le nichoir n’existe pas, il est créé automatiquement
        if not cursor.fetchone():
            cursor.execute(
                "INSERT INTO Nichoirs (id, nom, emplacement) VALUES (%s, %s, %s)",
                (nichoir_id, f"Nichoir {nichoir_id}", "Inconnu")
            )
            conn.commit()

        # Insertion de la capture dans la table Captures
        sql = """
        INSERT INTO Captures (nichoir_id, image_path, filename, battery_level, event_type)
        VALUES (%s, %s, %s, %s, %s)
        """
        
        # Le niveau de batterie est fixé à 100 % par défaut
        valeurs = (nichoir_id, chemin_complet, filename, 100, "mouvement")
        
        cursor.execute(sql, valeurs)
        conn.commit()
        
        cursor.close()
        conn.close()
        
    except Exception as e:
        # Affiche l’erreur si la base de données ne répond pas
        print(f"Erreur BDD : {e}")

# ------------------------------------------------------------------
# CALLBACK APPELÉ À LA RÉCEPTION D’UN MESSAGE MQTT
# ------------------------------------------------------------------

def on_message(client, userdata, msg):
    try:
        # Le topic est de la forme : nichoir/<id>/photo
        # Une expression régulière permet d’extraire l’identifiant du nichoir
        match = re.search(r"nichoir/(\d+)/photo", msg.topic)
        
        if match:
            # Conversion de l’ID récupéré en entier
            nichoir_id = int(match.group(1))
            
            # Création d’un nom de fichier unique basé sur la date et l’heure
            timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
            filename = f"nichoir{nichoir_id}_{timestamp}.jpg"
            chemin = f"{DOSSIER_PHOTOS}/{filename}"
            
            # Écriture des données binaires reçues dans un fichier image
            with open(chemin, "wb") as f:
                f.write(msg.payload)
            
            # Enregistrement des informations de la photo dans la base de données
            sauver_en_bdd(nichoir_id, filename, chemin)
        
        else:
            # Cas où le topic ne correspond pas au format attendu
            print(f"Topic non reconnu : {msg.topic}")
            
    except Exception as e:
        # Gestion des erreurs lors de la réception ou du traitement du message
        print(f"Erreur réception : {e}")

# ------------------------------------------------------------------
# INITIALISATION DU CLIENT MQTT
# ------------------------------------------------------------------

client = mqtt.Client()

# Abonnement automatique au topic lors de la connexion au broker
client.on_connect = lambda c, u, f, rc: c.subscribe(MQTT_TOPIC_SUBSCRIPTION)

# Association de la fonction de traitement des messages
client.on_message = on_message

# Connexion au broker MQTT
client.connect(MQTT_BROKER, 1883, 60)

# Boucle principale bloquante qui attend les messages
print("Serveur Multi-Nichoirs prêt...")
client.loop_forever()
