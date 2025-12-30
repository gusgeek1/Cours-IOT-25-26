from flask import Flask, render_template, send_from_directory, request
import mysql.connector

app = Flask(__name__)

DB_CONFIG = {
    'user': 'nichoir', 'password': 'nichoir',
    'host': 'localhost', 'database': 'projet_nichoir',
    'unix_socket': '/var/run/mysqld/mysqld.sock'
}
DOSSIER_PHOTOS = 'photos_nichoir'

def get_db():
    return mysql.connector.connect(**DB_CONFIG)

@app.route('/')
def dashboard():
    conn = get_db()
    cursor = conn.cursor(dictionary=True)
    
    # Récupère la liste des nichoirs et compte leurs photos
    query = """
    SELECT n.id, n.nom, n.emplacement, COUNT(c.id) as total_photos, MAX(c.date) as last_seen
    FROM Nichoirs n
    LEFT JOIN Captures c ON n.id = c.nichoir_id
    GROUP BY n.id
    """
    cursor.execute(query)
    nichoirs = cursor.fetchall()
    conn.close()
    return render_template('dashboard.html', nichoirs=nichoirs)

@app.route('/galerie')
def galerie():
    nichoir_id = request.args.get('id') # Permet de filtrer ?id=1
    conn = get_db()
    cursor = conn.cursor(dictionary=True)
    
    if nichoir_id:
        sql = "SELECT * FROM Captures WHERE nichoir_id = %s ORDER BY date DESC LIMIT 50"
        cursor.execute(sql, (nichoir_id,))
    else:
        sql = "SELECT * FROM Captures ORDER BY date DESC LIMIT 50"
        cursor.execute(sql)
        
    photos = cursor.fetchall()
    conn.close()
    return render_template('galerie.html', photos=photos, filter_id=nichoir_id)

@app.route('/photos/<path:filename>')
def serve_photo(filename):
    return send_from_directory(DOSSIER_PHOTOS, filename)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001, debug=True)
