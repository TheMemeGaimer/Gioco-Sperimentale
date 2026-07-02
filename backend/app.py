from flask import Flask, request, jsonify
import sqlite3
import hashlib
import secrets
import json
import os
import smtplib
import random
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from datetime import datetime, timedelta

app = Flask(__name__)
DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'vgc.db')

EMAIL_USER     = os.environ.get('EMAIL_USER', '')
EMAIL_PASSWORD = os.environ.get('EMAIL_APP_PASSWORD', '')

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
            verified INTEGER DEFAULT 0,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS verification_codes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            email TEXT NOT NULL,
            code TEXT NOT NULL,
            expires_at TEXT NOT NULL,
            used INTEGER DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS profiles (
            user_id INTEGER PRIMARY KEY,
            profile_data TEXT,
            updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS friendships (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            requester_id INTEGER NOT NULL,
            addressee_id INTEGER NOT NULL,
            status TEXT DEFAULT 'pending',
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(requester_id) REFERENCES users(id),
            FOREIGN KEY(addressee_id) REFERENCES users(id),
            UNIQUE(requester_id, addressee_id)
        );
        CREATE TABLE IF NOT EXISTS challenges (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            challenger_id INTEGER NOT NULL,
            challenged_id INTEGER NOT NULL,
            game TEXT NOT NULL,
            target_score INTEGER NOT NULL,
            status TEXT DEFAULT 'pending',
            winner_id INTEGER,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            expires_at TEXT NOT NULL,
            FOREIGN KEY(challenger_id) REFERENCES users(id),
            FOREIGN KEY(challenged_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS notifications (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            message TEXT NOT NULL,
            type TEXT DEFAULT 'info',
            read INTEGER DEFAULT 0,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    ''')
    conn.commit()
    conn.close()

def hash_password(password):
    return hashlib.sha256(password.encode('utf-8')).hexdigest()

def get_user_by_token(token):
    if not token:
        return None
    conn = get_db()
    row = conn.execute('SELECT * FROM users WHERE token = ?', (token,)).fetchone()
    conn.close()
    return dict(row) if row else None

def add_notification(conn, user_id, message, ntype='info'):
    conn.execute(
        'INSERT INTO notifications (user_id, message, type) VALUES (?, ?, ?)',
        (user_id, message, ntype)
    )

def send_email(to_addr, subject, body_text, body_html=None):
    if not EMAIL_USER or not EMAIL_PASSWORD:
        print(f"[EMAIL SKIP] To={to_addr} Subject={subject} Body={body_text}")
        return True
    try:
        msg = MIMEMultipart('alternative')
        msg['Subject'] = subject
        msg['From']    = f"Valentino Game Collection <{EMAIL_USER}>"
        msg['To']      = to_addr
        msg.attach(MIMEText(body_text, 'plain'))
        if body_html:
            msg.attach(MIMEText(body_html, 'html'))
        with smtplib.SMTP_SSL('smtp.gmail.com', 465) as server:
            server.login(EMAIL_USER, EMAIL_PASSWORD)
            server.sendmail(EMAIL_USER, to_addr, msg.as_string())
        return True
    except Exception as e:
        print(f"[EMAIL ERROR] {e}")
        return False

# ============================================================
# PING
# ============================================================
@app.route('/api/ping', methods=['GET'])
def ping():
    return jsonify({'status': 'ok', 'version': '2.0'})

# ============================================================
# VERIFICA EMAIL — Invia codice
# ============================================================
@app.route('/api/send-verification', methods=['POST'])
def send_verification():
    data  = request.get_json(silent=True) or {}
    email = data.get('email', '').strip().lower()
    if not email or '@' not in email:
        return jsonify({'success': False, 'message': 'Email non valida'}), 400

    conn = get_db()
    existing = conn.execute('SELECT id FROM users WHERE email = ?', (email,)).fetchone()
    conn.close()
    if existing:
        return jsonify({'success': False, 'message': 'Email gia registrata'}), 400

    code = str(random.randint(100000, 999999))
    expires = (datetime.now() + timedelta(minutes=15)).isoformat()

    conn = get_db()
    conn.execute('DELETE FROM verification_codes WHERE email = ?', (email,))
    conn.execute(
        'INSERT INTO verification_codes (email, code, expires_at) VALUES (?, ?, ?)',
        (email, code, expires)
    )
    conn.commit()
    conn.close()

    plain = f"""Valentino Game Collection — Verifica Email

Il tuo codice di verifica e': {code}

Valido per 15 minuti.
Se non hai richiesto la registrazione, ignora questa email.
"""
    html = f"""
<div style="font-family:monospace;background:#0a0a0a;color:#00ff41;padding:30px;max-width:500px">
  <h2 style="color:#00ff41;border-bottom:1px solid #00ff41;padding-bottom:10px">
    &#x2588; VALENTINO GAME COLLECTION
  </h2>
  <p style="color:#ccc">Benvenuto! Il tuo codice di verifica e':</p>
  <div style="background:#111;border:2px solid #00ff41;padding:20px;text-align:center;
              font-size:36px;letter-spacing:12px;color:#00ff41;margin:20px 0">
    {code}
  </div>
  <p style="color:#888;font-size:12px">Valido per 15 minuti.<br>
  Se non hai richiesto la registrazione, ignora questa email.</p>
</div>
"""
    ok = send_email(email, 'VGC — Codice di verifica', plain, html)
    if not ok:
        return jsonify({'success': False, 'message': 'Errore invio email'}), 500
    return jsonify({'success': True, 'message': 'Codice inviato!'})

# ============================================================
# REGISTRAZIONE
# ============================================================
@app.route('/api/register', methods=['POST'])
def register():
    data     = request.get_json(silent=True) or {}
    username = data.get('username', '').strip()
    email    = data.get('email', '').strip().lower()
    password = data.get('password', '')
    code     = data.get('code', '').strip()

    if not username or not email or not password or not code:
        return jsonify({'success': False, 'message': 'Dati mancanti'}), 400
    if len(password) < 6:
        return jsonify({'success': False, 'message': 'Password troppo corta (min 6 caratteri)'}), 400
    if '@' not in email:
        return jsonify({'success': False, 'message': 'Email non valida'}), 400

    conn = get_db()
    vc = conn.execute(
        'SELECT * FROM verification_codes WHERE email = ? AND used = 0 ORDER BY id DESC LIMIT 1',
        (email,)
    ).fetchone()

    if not vc:
        conn.close()
        return jsonify({'success': False, 'message': 'Nessun codice trovato. Richiedi un nuovo codice.'}), 400

    if datetime.fromisoformat(vc['expires_at']) < datetime.now():
        conn.close()
        return jsonify({'success': False, 'message': 'Codice scaduto. Richiedine uno nuovo.'}), 400

    if vc['code'] != code:
        conn.close()
        return jsonify({'success': False, 'message': 'Codice errato'}), 400

    token   = secrets.token_hex(32)
    pw_hash = hash_password(password)
    try:
        conn.execute(
            'INSERT INTO users (username, email, password_hash, token, verified) VALUES (?, ?, ?, ?, 1)',
            (username, email, pw_hash, token)
        )
        conn.execute('UPDATE verification_codes SET used = 1 WHERE id = ?', (vc['id'],))
        user_id = conn.execute('SELECT id FROM users WHERE email = ?', (email,)).fetchone()['id']
        add_notification(conn, user_id,
            f'Benvenuto in Valentino Game Collection, {username}! Buon divertimento!', 'welcome')
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'token': token, 'username': username,
                        'message': 'Registrazione completata!'})
    except sqlite3.IntegrityError as e:
        conn.close()
        msg = 'Email gia registrata' if 'email' in str(e) else 'Username gia in uso'
        return jsonify({'success': False, 'message': msg}), 400

# ============================================================
# LOGIN
# ============================================================
@app.route('/api/login', methods=['POST'])
def login():
    data     = request.get_json(silent=True) or {}
    email    = data.get('email', '').strip().lower()
    password = data.get('password', '')
    pw_hash  = hash_password(password)

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
    return jsonify({'success': True, 'token': token, 'username': user['username'],
                    'verified': bool(user['verified'])})

# ============================================================
# PROFILO
# ============================================================
@app.route('/api/profile', methods=['GET'])
def get_profile():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    row  = conn.execute('SELECT profile_data FROM profiles WHERE user_id = ?', (user['id'],)).fetchone()
    conn.close()
    profile = json.loads(row['profile_data']) if row and row['profile_data'] else {}
    return jsonify({'success': True, 'profile': profile})

@app.route('/api/profile', methods=['POST'])
def save_profile():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data = request.get_json(silent=True) or {}
    conn = get_db()
    conn.execute(
        'INSERT OR REPLACE INTO profiles (user_id, profile_data, updated_at) VALUES (?, ?, ?)',
        (user['id'], json.dumps(data), datetime.now().isoformat())
    )

    # Controlla sfide completate
    snake_score = data.get('snakeBestScore', 0)
    dodge_time  = data.get('dodgeBestTime', 0)
    challenges  = conn.execute(
        "SELECT * FROM challenges WHERE challenged_id = ? AND status = 'accepted'",
        (user['id'],)
    ).fetchall()
    for ch in challenges:
        game   = ch['game']
        target = ch['target_score']
        beat   = False
        if game == 'snake' and snake_score >= target:
            beat = True
        elif game == 'dodge' and dodge_time >= target:
            beat = True
        if beat:
            conn.execute(
                "UPDATE challenges SET status = 'won_by_challenged', winner_id = ? WHERE id = ?",
                (user['id'], ch['id'])
            )
            challenger_name = conn.execute(
                'SELECT username FROM users WHERE id = ?', (ch['challenger_id'],)
            ).fetchone()['username']
            add_notification(conn, user['id'],
                f'Hai completato la sfida di {challenger_name} in {game.upper()}!', 'challenge_win')
            add_notification(conn, ch['challenger_id'],
                f'{user["username"]} ha vinto la tua sfida in {game.upper()} (target: {target})!', 'challenge_lose')

    conn.commit()
    conn.close()
    return jsonify({'success': True})

# ============================================================
# LEADERBOARD
# ============================================================
@app.route('/api/leaderboard/<game>', methods=['GET'])
def leaderboard(game):
    conn = get_db()
    rows = conn.execute(
        'SELECT u.username, p.profile_data FROM profiles p JOIN users u ON p.user_id = u.id WHERE p.profile_data IS NOT NULL'
    ).fetchall()
    conn.close()

    scores = []
    for row in rows:
        try:
            profile = json.loads(row['profile_data'])
            if game == 'snake':
                s = profile.get('snakeBestScore', 0)
                if s > 0:
                    scores.append({'username': row['username'], 'score': s,
                                   'difficulty': profile.get('snakeBestDifficulty', 0)})
            elif game == 'dodge':
                t = profile.get('dodgeBestTime', 0)
                if t > 0:
                    scores.append({'username': row['username'], 'time': t,
                                   'difficulty': profile.get('dodgeBestDifficulty', 0)})
            elif game == 'indovina':
                b = profile.get('migliorTentativi', 9999)
                if b < 9999:
                    scores.append({'username': row['username'], 'tentativi': b})
        except Exception:
            pass

    if game == 'snake':
        scores.sort(key=lambda x: x['score'], reverse=True)
    elif game == 'dodge':
        scores.sort(key=lambda x: x['time'], reverse=True)
    elif game == 'indovina':
        scores.sort(key=lambda x: x['tentativi'])

    return jsonify({'success': True, 'leaderboard': scores[:10]})

# ============================================================
# AMICIZIE
# ============================================================
@app.route('/api/friends/request', methods=['POST'])
def friend_request():
    token    = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user     = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data         = request.get_json(silent=True) or {}
    friend_name  = data.get('username', '').strip()

    conn = get_db()
    friend = conn.execute('SELECT * FROM users WHERE username = ?', (friend_name,)).fetchone()
    if not friend:
        conn.close()
        return jsonify({'success': False, 'message': 'Utente non trovato'}), 404
    if friend['id'] == user['id']:
        conn.close()
        return jsonify({'success': False, 'message': 'Non puoi aggiungerti da solo'}), 400

    existing = conn.execute(
        'SELECT * FROM friendships WHERE (requester_id=? AND addressee_id=?) OR (requester_id=? AND addressee_id=?)',
        (user['id'], friend['id'], friend['id'], user['id'])
    ).fetchone()
    if existing:
        conn.close()
        status = existing['status']
        if status == 'accepted':
            return jsonify({'success': False, 'message': 'Siete gia amici'}), 400
        return jsonify({'success': False, 'message': 'Richiesta gia inviata'}), 400

    try:
        conn.execute(
            'INSERT INTO friendships (requester_id, addressee_id) VALUES (?, ?)',
            (user['id'], friend['id'])
        )
        add_notification(conn, friend['id'],
            f'{user["username"]} ti ha inviato una richiesta di amicizia!', 'friend_request')
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': f'Richiesta inviata a {friend_name}!'})
    except Exception as e:
        conn.close()
        return jsonify({'success': False, 'message': str(e)}), 500

@app.route('/api/friends/respond', methods=['POST'])
def friend_respond():
    token  = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user   = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data   = request.get_json(silent=True) or {}
    req_id = data.get('friendship_id')
    action = data.get('action', '')  # 'accept' o 'decline'

    conn = get_db()
    fs   = conn.execute(
        'SELECT * FROM friendships WHERE id = ? AND addressee_id = ?', (req_id, user['id'])
    ).fetchone()
    if not fs:
        conn.close()
        return jsonify({'success': False, 'message': 'Richiesta non trovata'}), 404

    if action == 'accept':
        conn.execute("UPDATE friendships SET status = 'accepted' WHERE id = ?", (req_id,))
        requester = conn.execute('SELECT username FROM users WHERE id=?', (fs['requester_id'],)).fetchone()
        add_notification(conn, fs['requester_id'],
            f'{user["username"]} ha accettato la tua richiesta di amicizia!', 'friend_accept')
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': f'Amicizia con {requester["username"]} accettata!'})
    elif action == 'decline':
        conn.execute('DELETE FROM friendships WHERE id = ?', (req_id,))
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': 'Richiesta rifiutata.'})
    else:
        conn.close()
        return jsonify({'success': False, 'message': 'Azione non valida'}), 400

@app.route('/api/friends/list', methods=['GET'])
def friends_list():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    rows = conn.execute('''
        SELECT f.id, f.status,
               CASE WHEN f.requester_id = ? THEN u2.username ELSE u1.username END AS friend_name,
               CASE WHEN f.requester_id = ? THEN f.addressee_id ELSE f.requester_id END AS friend_id,
               CASE WHEN f.requester_id = ? THEN 'sent' ELSE 'received' END AS direction
        FROM friendships f
        JOIN users u1 ON u1.id = f.requester_id
        JOIN users u2 ON u2.id = f.addressee_id
        WHERE f.requester_id = ? OR f.addressee_id = ?
    ''', (user['id'], user['id'], user['id'], user['id'], user['id'])).fetchall()

    friends  = []
    pending  = []
    for r in rows:
        if r['status'] == 'accepted':
            # Ottieni il miglior score dell'amico
            prow = conn.execute('SELECT profile_data FROM profiles WHERE user_id=?', (r['friend_id'],)).fetchone()
            friend_data = {}
            if prow and prow['profile_data']:
                try:
                    pd = json.loads(prow['profile_data'])
                    friend_data = {'snakeBestScore': pd.get('snakeBestScore',0),
                                   'dodgeBestTime':  pd.get('dodgeBestTime',0)}
                except Exception:
                    pass
            friends.append({'id': r['id'], 'username': r['friend_name'],
                            'friend_id': r['friend_id'], **friend_data})
        elif r['status'] == 'pending' and r['direction'] == 'received':
            pending.append({'id': r['id'], 'username': r['friend_name']})

    conn.close()
    return jsonify({'success': True, 'friends': friends, 'pending': pending})

# ============================================================
# SFIDE
# ============================================================
@app.route('/api/challenges/send', methods=['POST'])
def send_challenge():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data        = request.get_json(silent=True) or {}
    friend_name = data.get('username', '').strip()
    game        = data.get('game', '').strip()       # snake / dodge
    target      = data.get('target', 0)

    if game not in ('snake', 'dodge') or target <= 0:
        return jsonify({'success': False, 'message': 'Dati sfida non validi'}), 400

    conn = get_db()
    friend = conn.execute('SELECT * FROM users WHERE username = ?', (friend_name,)).fetchone()
    if not friend:
        conn.close()
        return jsonify({'success': False, 'message': 'Utente non trovato'}), 404

    fs = conn.execute(
        "SELECT * FROM friendships WHERE status='accepted' AND ((requester_id=? AND addressee_id=?) OR (requester_id=? AND addressee_id=?))",
        (user['id'], friend['id'], friend['id'], user['id'])
    ).fetchone()
    if not fs:
        conn.close()
        return jsonify({'success': False, 'message': 'Non sei amico di questo utente'}), 400

    expires = (datetime.now() + timedelta(days=7)).isoformat()
    conn.execute(
        'INSERT INTO challenges (challenger_id, challenged_id, game, target_score, expires_at) VALUES (?,?,?,?,?)',
        (user['id'], friend['id'], game, target, expires)
    )
    add_notification(conn, friend['id'],
        f'{user["username"]} ti ha sfidato in {game.upper()}! Target: {target}. Scade tra 7 giorni.', 'challenge')
    conn.commit()
    conn.close()
    return jsonify({'success': True, 'message': f'Sfida inviata a {friend_name}!'})

@app.route('/api/challenges/respond', methods=['POST'])
def respond_challenge():
    token  = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user   = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data      = request.get_json(silent=True) or {}
    ch_id     = data.get('challenge_id')
    action    = data.get('action', '')  # 'accept' o 'decline'

    conn = get_db()
    ch   = conn.execute(
        "SELECT * FROM challenges WHERE id = ? AND challenged_id = ? AND status = 'pending'",
        (ch_id, user['id'])
    ).fetchone()
    if not ch:
        conn.close()
        return jsonify({'success': False, 'message': 'Sfida non trovata'}), 404

    if action == 'accept':
        conn.execute("UPDATE challenges SET status = 'accepted' WHERE id = ?", (ch_id,))
        challenger = conn.execute('SELECT username FROM users WHERE id=?', (ch['challenger_id'],)).fetchone()
        add_notification(conn, ch['challenger_id'],
            f'{user["username"]} ha accettato la tua sfida in {ch["game"].upper()}!', 'challenge_accept')
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': 'Sfida accettata! Batti il target!'})
    elif action == 'decline':
        conn.execute("UPDATE challenges SET status = 'declined' WHERE id = ?", (ch_id,))
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': 'Sfida rifiutata.'})
    conn.close()
    return jsonify({'success': False, 'message': 'Azione non valida'}), 400

@app.route('/api/challenges/list', methods=['GET'])
def list_challenges():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    rows = conn.execute('''
        SELECT c.*,
               u1.username AS challenger_name,
               u2.username AS challenged_name
        FROM challenges c
        JOIN users u1 ON u1.id = c.challenger_id
        JOIN users u2 ON u2.id = c.challenged_id
        WHERE (c.challenger_id = ? OR c.challenged_id = ?)
          AND c.expires_at > ?
        ORDER BY c.created_at DESC
        LIMIT 20
    ''', (user['id'], user['id'], datetime.now().isoformat())).fetchall()
    conn.close()

    result = []
    for r in rows:
        result.append({
            'id': r['id'],
            'game': r['game'],
            'target': r['target_score'],
            'status': r['status'],
            'challenger': r['challenger_name'],
            'challenged': r['challenged_name'],
            'direction': 'sent' if r['challenger_id'] == user['id'] else 'received',
            'expires_at': r['expires_at']
        })
    return jsonify({'success': True, 'challenges': result})

# ============================================================
# NOTIFICHE
# ============================================================
@app.route('/api/notifications', methods=['GET'])
def get_notifications():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    rows = conn.execute(
        'SELECT * FROM notifications WHERE user_id = ? ORDER BY created_at DESC LIMIT 20',
        (user['id'],)
    ).fetchall()
    unread = conn.execute(
        'SELECT COUNT(*) AS c FROM notifications WHERE user_id = ? AND read = 0',
        (user['id'],)
    ).fetchone()['c']
    conn.execute('UPDATE notifications SET read = 1 WHERE user_id = ?', (user['id'],))
    conn.commit()
    conn.close()

    notifs = [{'id': r['id'], 'message': r['message'], 'type': r['type'],
               'read': bool(r['read']), 'created_at': r['created_at']} for r in rows]
    return jsonify({'success': True, 'notifications': notifs, 'unread': unread})

if __name__ == '__main__':
    init_db()
    print("=== VGC Backend Server v2.0 ===")
    print(f"Database: {DB_PATH}")
    print(f"Email: {EMAIL_USER or '(non configurata)'}")
    print("Server: http://localhost:8000")
    app.run(host='localhost', port=8000, debug=False)
