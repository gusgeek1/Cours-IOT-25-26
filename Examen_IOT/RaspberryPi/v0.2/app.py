from flask import Flask, render_template, url_for
import mysql.connector
import json
from datetime import datetime

# Création de l’application Flask
app = Flask(__name__)

# ------------------------------------------------------------------
# CONFIGURATION DE LA BASE DE DONNÉES
# ------------------------------------------------------------------

# Paramètres de connexion à la base de données MySQL
DB_CONFIG = {
    'user': 'nichoir',
    'password': 'nichoir',
    'host': 'localhost',
    'database': 'projet_nichoir'
}

# Fonction utilitaire qui retourne une connexion à la base de données
def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# ------------------------------------------------------------------
# CONVERSION DES DATES POUR L’ENVOI EN JSON
# ------------------------------------------------------------------

# Cette fonction transforme les objets datetime en texte lisible
# afin qu’ils puissent être correctement envoyés vers JavaScript
def default_converter(o):
    if isinstance(o, datetime):
        return o.strftime("%d/%m/%Y - %H:%M")
    return o.__str__()

# ------------------------------------------------------------------
# ROUTE PRINCIPALE DU SITE
# ------------------------------------------------------------------

@app.route('/')
def index():
    # Connexion à la base de données
    conn = get_db()
    cursor = conn.cursor(dictionary=True)

    # --------------------------------------------------------------
    # 1. RÉCUPÉRATION DES NICHoirs ET DE LEURS DERNIÈRES INFORMATIONS
    # --------------------------------------------------------------

    # Cette requête récupère tous les nichoirs
    # et associe à chacun la dernière batterie et la dernière date connue
    query_nichoirs = """
    SELECT n.id, n.nom, n.emplacement,
           (SELECT battery_level FROM Captures WHERE nichoir_id = n.id ORDER BY date DESC LIMIT 1) AS battery,
           (SELECT date FROM Captures WHERE nichoir_id = n.id ORDER BY date DESC LIMIT 1) AS last_seen
    FROM Nichoirs n
    """
    cursor.execute(query_nichoirs)
    nichoirs_list = cursor.fetchall()

    # --------------------------------------------------------------
    # 2. RÉCUPÉRATION DES PHOTOS
    # --------------------------------------------------------------

    # Cette requête récupère les 50 dernières captures
    # qui possèdent un nom de fichier valide
    query_photos = """
    SELECT * FROM Captures
    WHERE filename != ''
    ORDER BY date DESC
    LIMIT 50
    """
    cursor.execute(query_photos)
    photos_raw = cursor.fetchall()

    # --------------------------------------------------------------
    # 3. PRÉPARATION DES DONNÉES POUR JAVASCRIPT
    # --------------------------------------------------------------

    photos_data = []

    for p in photos_raw:
        # Génération du lien vers l’image stockée dans le dossier static
        url_img = url_for('static', filename='photos/' + p['filename'])

        # Création d’un dictionnaire contenant les informations utiles
        photos_data.append({
            "id": p['id'],
            "nichoir": f"Nichoir {p['nichoir_id']}",
            "date": p['date'].strftime("%d/%m/%Y %H:%M"),
            "url": url_img,
            "battery": p['battery_level']
        })

    # Fermeture de la connexion à la base de données
    conn.close()

    # Envoi des données vers le template HTML
    return render_template(
        'index.html',
        nichoirs=nichoirs_list,
        photos_json=json.dumps(photos_data, default=default_converter)
    )

# ------------------------------------------------------------------
# LANCEMENT DU SERVEUR FLASK
# ------------------------------------------------------------------

if __name__ == '__main__':
    # Le serveur écoute sur toutes les interfaces réseau
    # Le mode debug est activé pour faciliter le développement
    app.run(host='0.0.0.0', port=5001, debug=True)
