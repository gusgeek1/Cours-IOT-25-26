from flask import Flask, render_template, url_for, request, session, redirect, jsonify
import mysql.connector
import json
from datetime import datetime

# ------------------------------------------------------------------
# INITIALISATION DE L’APPLICATION FLASK
# ------------------------------------------------------------------

# Création de l’application Flask
app = Flask(__name__)

# Clé secrète utilisée pour la gestion des sessions utilisateur
app.secret_key = 'smart_birdhouse_key'

# ------------------------------------------------------------------
# CONFIGURATION GÉNÉRALE
# ------------------------------------------------------------------

# Mot de passe administrateur pour accéder à l’interface
ADMIN_PASSWORD = "admin"

# Texte affiché dans l’interface pour identifier le projet
AUTEURS = "Projet IOT - A. DEJONGHE & T. COLLARD - vFinale"

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

# Retourne une connexion active à la base de données
def get_db():
    return mysql.connector.connect(**DB_CONFIG)

# Estime l’état de la batterie à partir de la tension mesurée
def estimate_autonomy(voltage):
    # Si aucune valeur n’est disponible, on retourne une valeur neutre
    if not voltage:
        return "--"

    # Conversion explicite de la valeur reçue
    v = float(voltage)

    # Classification simple de l’état de la batterie
    if v >= 4.1:
        return "Excellente (>90%)"
    if v >= 3.9:
        return "Bonne (~70%)"
    if v >= 3.7:
        return "Moyenne (~40%)"
    if v < 3.5:
        return "Critique (Recharger !)"

    return "Stable"

# ------------------------------------------------------------------
# ROUTE PRINCIPALE
# ------------------------------------------------------------------

@app.route('/', methods=['GET', 'POST'])
def index():
    error_msg = None

    # --------------------------------------------------------------
    # GESTION DE L’AUTHENTIFICATION
    # --------------------------------------------------------------

    # Si l’utilisateur n’est pas connecté, affichage du formulaire de login
    if not session.get('logged_in'):
        if request.method == 'POST':
            # Vérification du mot de passe saisi
            if request.form.get('password') == ADMIN_PASSWORD:
                session['logged_in'] = True
                return redirect(url_for('index'))
            else:
                error_msg = "Mot de passe incorrect !"

        return render_template(
            'index.html',
            login_page=True,
            auteurs=AUTEURS,
            error=error_msg
        )

    # --------------------------------------------------------------
    # AFFICHAGE DU DASHBOARD APRÈS CONNEXION
    # --------------------------------------------------------------

    conn = get_db()
    cursor = conn.cursor(dictionary=True)

    # Récupération de tous les nichoirs enregistrés
    cursor.execute("SELECT * FROM Nichoirs")
    nichoirs = cursor.fetchall()

    # Calcul de l’état de batterie pour chaque nichoir
    for n in nichoirs:
        n['autonomie'] = estimate_autonomy(n['battery_level'])

    # Récupération des 50 dernières photos enregistrées
    cursor.execute(
        "SELECT * FROM Captures WHERE filename != '' ORDER BY date DESC LIMIT 50"
    )
    photos_raw = cursor.fetchall()

    conn.close()

    # --------------------------------------------------------------
    # PRÉPARATION DES DONNÉES POUR L’AFFICHAGE DES PHOTOS
    # --------------------------------------------------------------

    photos_list = []
    for p in photos_raw:
        photos_list.append({
            "id": p['id'],
            "url": url_for('static', filename='photos/' + p['filename']),
            "date": p['date'].strftime("%d/%m %H:%M"),
            "fav": bool(p.get('is_favorite', 0)),
            "nichoir_id": p['nichoir_id']
        })

    # Envoi des données au template HTML
    return render_template(
        'index.html',
        nichoirs=nichoirs,
        photos_json=json.dumps(photos_list),
        auteurs=AUTEURS
    )

# ------------------------------------------------------------------
# ROUTE DE DÉCONNEXION
# ------------------------------------------------------------------

@app.route('/logout')
def logout():
    # Suppression de toutes les informations de session
    session.clear()
    return redirect(url_for('index'))

# ------------------------------------------------------------------
# API POUR LA GESTION DES FAVORIS
# ------------------------------------------------------------------

@app.route('/api/fav/<int:id>', methods=['POST'])
def toggle_fav(id):
    # Inversion de l’état favori d’une capture dans la base de données
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute(
        "UPDATE Captures SET is_favorite = NOT is_favorite WHERE id = %s",
        (id,)
    )
    conn.commit()
    conn.close()

    return jsonify(success=True)

# ------------------------------------------------------------------
# LANCEMENT DU SERVEUR FLASK
# ------------------------------------------------------------------

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)
