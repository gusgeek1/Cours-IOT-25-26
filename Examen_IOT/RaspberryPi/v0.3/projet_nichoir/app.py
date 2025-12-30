from flask import Flask, render_template, url_for, request, session, redirect, jsonify
import mysql.connector
import json
from datetime import datetime

# Création de l’application Flask
app = Flask(__name__)

# Clé secrète utilisée pour gérer les sessions utilisateur
app.secret_key = 'mon_mot_de_passe_super_secret'

# ------------------------------------------------------------------
# CONFIGURATION DE L’ADMINISTRATION
# ------------------------------------------------------------------

# Mot de passe administrateur utilisé pour l’accès au site
ADMIN_PASSWORD = "admin"

# Paramètres de connexion à la base de données MySQL
DB_CONFIG = {
    'user': 'nichoir',
    'password': 'nichoir',
    'host': 'localhost',
    'database': 'projet_nichoir'
}

# ------------------------------------------------------------------
# FONCTIONS UTILITAIRES
# ------------------------------------------------------------------

# Retourne une connexion à la base de données
def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# Convertit les objets datetime en texte pour l’envoi en JSON
def default_converter(o):
    if isinstance(o, datetime):
        return o.strftime("%d/%m/%Y - %H:%M")
    return o.__str__()

# ------------------------------------------------------------------
# CALCUL DE L’AUTONOMIE DE LA BATTERIE
# ------------------------------------------------------------------

# Estime l’autonomie restante en fonction de la valeur de batterie reçue
def calcul_autonomie(valeur_batt):
    try:
        val = float(valeur_batt)

        # Si la valeur est supérieure à 12, elle est considérée comme un pourcentage
        if val > 12:
            pourcentage = min(val, 100)
        else:
            # Sinon la valeur est considérée comme une tension (entre 3.3V et 4.2V)
            pourcentage = ((val - 3.3) / (4.2 - 3.3)) * 100

        # Limitation de la valeur entre 0 et 100 %
        pourcentage = max(0, min(100, pourcentage))

        # Hypothèse : une batterie pleine correspond à environ 6 mois d’autonomie
        jours_restants = (pourcentage / 100) * 180
        mois = int(jours_restants / 30)

        if mois >= 1:
            return f"~ {mois} mois"
        return f"~ {int(jours_restants)} jours"

    except:
        return "--"

# ------------------------------------------------------------------
# ROUTE PRINCIPALE
# ------------------------------------------------------------------

@app.route('/', methods=['GET', 'POST'])
def index():
    # --------------------------------------------------------------
    # 1. GESTION DE L’AUTHENTIFICATION
    # --------------------------------------------------------------

    # Si l’utilisateur n’est pas connecté, affichage de la page de login
    if not session.get('logged_in'):
        if request.method == 'POST':
            # Vérification du mot de passe saisi
            if request.form.get('password') == ADMIN_PASSWORD:
                session['logged_in'] = True
                return redirect(url_for('index'))
            else:
                return render_template('index.html', error="Mot de passe incorrect")

        return render_template('index.html', login_page=True)

    # --------------------------------------------------------------
    # 2. RÉCUPÉRATION DES DONNÉES APRÈS CONNEXION
    # --------------------------------------------------------------

    try:
        conn = get_db()
        cursor = conn.cursor(dictionary=True)

        # Récupération de la liste des nichoirs avec leur dernière batterie et date
        query_nichoirs = """
        SELECT n.id, n.nom, n.emplacement,
               (SELECT battery_level FROM Captures WHERE nichoir_id = n.id ORDER BY date DESC LIMIT 1) AS battery,
               (SELECT date FROM Captures WHERE nichoir_id = n.id ORDER BY date DESC LIMIT 1) AS last_seen
        FROM Nichoirs n
        """
        cursor.execute(query_nichoirs)
        nichoirs_list = cursor.fetchall()

        # Calcul de l’autonomie estimée pour chaque nichoir
        for n in nichoirs_list:
            n['autonomie'] = calcul_autonomie(n['battery'] or 0)

        # Récupération de toutes les photos enregistrées
        cursor.execute("SELECT * FROM Captures WHERE filename != '' ORDER BY date DESC")
        photos_raw = cursor.fetchall()
        conn.close()

        # Préparation des données pour l’affichage côté JavaScript
        photos_data = []
        for p in photos_raw:
            photos_data.append({
                "id": p['id'],
                "nichoir": f"Nichoir {p['nichoir_id']}",
                "nichoir_id": p['nichoir_id'],
                "date": p['date'].strftime("%d/%m/%Y - %H:%M"),
                "url": url_for('static', filename='photos/' + p['filename']),
                "battery": p['battery_level'],
                "is_favorite": bool(p['is_favorite'])
            })

        return render_template(
            'index.html',
            nichoirs=nichoirs_list,
            photos_json=json.dumps(photos_data, default=default_converter)
        )

    except Exception as e:
        return f"Erreur BDD : {e}"

# ------------------------------------------------------------------
# ROUTE DE DÉCONNEXION
# ------------------------------------------------------------------

@app.route('/logout')
def logout():
    # Suppression de toutes les données de session
    session.clear()
    return redirect(url_for('index'))

# ------------------------------------------------------------------
# API POUR LA GESTION DES FAVORIS
# ------------------------------------------------------------------

@app.route('/api/toggle_fav/<int:photo_id>', methods=['POST'])
def toggle_fav(photo_id):
    # Refus de la requête si l’utilisateur n’est pas connecté
    if not session.get('logged_in'):
        return jsonify({'status': 'error'}), 403

    # Inversion de l’état favori de la photo sélectionnée
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        "UPDATE Captures SET is_favorite = NOT is_favorite WHERE id = %s",
        (photo_id,)
    )
    conn.commit()
    conn.close()

    return jsonify({'status': 'success'})

# ------------------------------------------------------------------
# LANCEMENT DU SERVEUR FLASK
# ------------------------------------------------------------------

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)
