from flask import Flask, request, jsonify
import sqlite3
import hashlib
import secrets
import json
import os
from datetime import datetime

app = Flask(__name__)
DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'vgc.db')

def get_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db()
    conn.executescript('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            token TEXT UNIQUE,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS profiles (
            user_id INTEGER PRIMARY KEY,
            profile_data TEXT,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    ''')
    conn.commit()
    conn.close()

def hash_password(password):
    return hashlib.sha256(password.encode('utf-8')).hexdigest()

def get_user_by_token(token):
    conn = get_db()
    row = conn.execute('SELECT * FROM users WHERE token = ?', (token,)).fetchone()
    conn.close()
    return dict(row) if row else None

@app.route('/api/ping', methods=['GET'])
def ping():
    return jsonify({'status': 'ok', 'version': '1.0'})

@app.route('/api/register', methods=['POST'])
def register():
    data = request.get_json(silent=True) or {}
    username = data.get('username', '').strip()
    email = data.get('email', '').strip().lower()
    password = data.get('password', '')

    if not username or not email or not password:
        return jsonify({'success': False, 'message': 'Dati mancanti'}), 400
    if len(password) < 6:
        return jsonify({'success': False, 'message': 'Password troppo corta (min 6 caratteri)'}), 400
    if '@' not in email:
        return jsonify({'success': False, 'message': 'Email non valida'}), 400

    token = secrets.token_hex(32)
    pw_hash = hash_password(password)

    try:
        conn = get_db()
        conn.execute(
            'INSERT INTO users (username, email, password_hash, token) VALUES (?, ?, ?, ?)',
            (username, email, pw_hash, token)
        )
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': 'Registrazione completata!', 'token': token, 'username': username})
    except sqlite3.IntegrityError as e:
        conn.close()
        msg = 'Email gia registrata' if 'email' in str(e) else 'Username gia in uso'
        return jsonify({'success': False, 'message': msg}), 400

@app.route('/api/login', methods=['POST'])
def login():
    data = request.get_json(silent=True) or {}
    email = data.get('email', '').strip().lower()
    password = data.get('password', '')

    pw_hash = hash_password(password)
    conn = get_db()
    user = conn.execute(
        'SELECT * FROM users WHERE email = ? AND password_hash = ?',
        (email, pw_hash)
    ).fetchone()

    if not user:
        conn.close()
        return jsonify({'success': False, 'message': 'Email o password errati'}), 401

    token = secrets.token_hex(32)
    conn.execute('UPDATE users SET token = ? WHERE id = ?', (token, user['id']))
    conn.commit()
    conn.close()

    return jsonify({'success': True, 'token': token, 'username': user['username']})

@app.route('/api/profile', methods=['GET'])
def get_profile():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    row = conn.execute('SELECT profile_data FROM profiles WHERE user_id = ?', (user['id'],)).fetchone()
    conn.close()

    if row and row['profile_data']:
        profile = json.loads(row['profile_data'])
    else:
        profile = {}

    return jsonify({'success': True, 'profile': profile})

@app.route('/api/profile', methods=['POST'])
def save_profile():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data = request.get_json(silent=True) or {}
    profile_json = json.dumps(data)

    conn = get_db()
    conn.execute(
        'INSERT OR REPLACE INTO profiles (user_id, profile_data, updated_at) VALUES (?, ?, ?)',
        (user['id'], profile_json, datetime.now().isoformat())
    )
    conn.commit()
    conn.close()

    return jsonify({'success': True})

@app.route('/api/leaderboard/<game>', methods=['GET'])
def leaderboard(game):
    conn = get_db()
    rows = conn.execute('''
        SELECT u.username, p.profile_data
        FROM profiles p JOIN users u ON p.user_id = u.id
        WHERE p.profile_data IS NOT NULL
    ''').fetchall()
    conn.close()

    scores = []
    for row in rows:
        try:
            profile = json.loads(row['profile_data'])
            if game == 'snake':
                score = profile.get('snakeBestScore', 0)
                diff = profile.get('snakeBestDifficulty', 0)
                if score > 0:
                    scores.append({'username': row['username'], 'score': score, 'difficulty': diff})
            elif game == 'dodge':
                t = profile.get('dodgeBestTime', 0)
                diff = profile.get('dodgeBestDifficulty', 0)
                if t > 0:
                    scores.append({'username': row['username'], 'time': t, 'difficulty': diff})
            elif game == 'indovina':
                best = profile.get('migliorTentativi', 9999)
                if best < 9999:
                    scores.append({'username': row['username'], 'tentativi': best})
        except Exception:
            pass

    if game == 'snake':
        scores.sort(key=lambda x: x['score'], reverse=True)
    elif game == 'dodge':
        scores.sort(key=lambda x: x['time'], reverse=True)
    elif game == 'indovina':
        scores.sort(key=lambda x: x['tentativi'])

    return jsonify({'success': True, 'leaderboard': scores[:10]})

if __name__ == '__main__':
    init_db()
    print("=== VGC Backend Server ===")
    print(f"Database: {DB_PATH}")
    print("Server: http://localhost:8000")
    app.run(host='localhost', port=8000, debug=False)
