from flask import Flask, render_template, url_for, request, session, redirect, jsonify
import mysql.connector
import json
from datetime import datetime

# ------------------------------------------------------------------
# INITIALISATION DE L’APPLICATION FLASK
# ------------------------------------------------------------------

# Création de l’application web Flask
app = Flask(__name__)

# Clé secrète utilisée pour la gestion des sessions
app.secret_key = 'cle_secrete_nichoir'

# ------------------------------------------------------------------
# CONFIGURATION GÉNÉRALE
# ------------------------------------------------------------------

# Mot de passe administrateur pour l’accès à l’interface
ADMIN_PASSWORD = "admin"

# Chaîne affichée dans l’interface pour identifier le projet
AUTEURS = "Projet IOT - A. DEJONGHE & T. COLLARD - v0.3"

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

# Estime l’autonomie restante à partir de la valeur de batterie
def estimate_autonomy(v):
    # Si aucune valeur n’est disponible, on retourne une valeur neutre
    if not v:
        return "--"

    # Si la valeur est supérieure à 12, elle est considérée comme un pourcentage
    # Sinon, elle est considérée comme une tension comprise entre 3.3 V et 4.2 V
    pct = v if v > 12 else ((v - 3.3) / 0.9) * 100

    # Limitation du pourcentage entre 0 et 100
    pct = max(0, min(100, pct))

    # Estimation de l’autonomie sur base de 6 mois à 100 %
    mois = int((pct / 100) * 6)

    if mois > 0:
        return f"~ {mois} mois"
    return "< 1 mois"

# ------------------------------------------------------------------
# ROUTE PRINCIPALE
# ------------------------------------------------------------------

@app.route('/', methods=['GET', 'POST'])
def index():
    # --------------------------------------------------------------
    # GESTION DE L’AUTHENTIFICATION
    # --------------------------------------------------------------

    # Si l’utilisateur n’est pas connecté, affichage du formulaire de login
    if not session.get('logged_in'):
        if request.method == 'POST' and request.form.get('password') == ADMIN_PASSWORD:
            session['logged_in'] = True
            return redirect(url_for('index'))

        return render_template(
            'index.html',
            login_page=True,
            auteurs=AUTEURS
        )

    # --------------------------------------------------------------
    # RÉCUPÉRATION DES DONNÉES APRÈS CONNEXION
    # --------------------------------------------------------------

    conn = get_db()
    cursor = conn.cursor(dictionary=True)

    # Récupération de tous les nichoirs
    cursor.execute("SELECT * FROM Nichoirs")
    nichoirs = cursor.fetchall()

    # Calcul de l’autonomie estimée pour chaque nichoir
    for n in nichoirs:
        n['autonomie'] = estimate_autonomy(n['battery_level'])

    # Récupération des captures (photos) triées par date décroissante
    cursor.execute("SELECT * FROM Captures ORDER BY date DESC")
    photos_raw = cursor.fetchall()

    conn.close()

    # --------------------------------------------------------------
    # PRÉPARATION DES DONNÉES POUR L’AFFICHAGE
    # --------------------------------------------------------------

    photos_list = []
    for p in photos_raw:
        photos_list.append({
            "id": p['id'],
            "url": url_for('static', filename='photos/' + p['filename']),
            "date": p['date'].strftime("%d/%m/%Y %H:%M"),
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
    # Inversion de l’état favori d’une capture
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
