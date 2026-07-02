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
        CREATE TABLE IF NOT EXISTS tournaments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            creator_id INTEGER NOT NULL,
            games TEXT NOT NULL,
            status TEXT DEFAULT 'open',
            deadline TEXT,
            winner_id INTEGER,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(creator_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS tournament_participants (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tournament_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            joined_at TEXT DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(tournament_id, user_id),
            FOREIGN KEY(tournament_id) REFERENCES tournaments(id),
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS tournament_scores (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tournament_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            game TEXT NOT NULL,
            score INTEGER NOT NULL,
            submitted_at TEXT DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(tournament_id, user_id, game),
            FOREIGN KEY(tournament_id) REFERENCES tournaments(id),
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

def calc_game_pts(game, score):
    """Converti raw score in punti torneo normalizzati."""
    if game == 'snake':
        return score
    elif game == 'dodge':
        return score
    elif game == 'indovina':
        return max(0, 11 - score)  # 1 tentativo = 10pt, 10 tentativi = 1pt
    return 0

def get_tournament_standings(conn, tournament_id):
    """Restituisce classifica completa di un torneo."""
    t = conn.execute('SELECT * FROM tournaments WHERE id=?', (tournament_id,)).fetchone()
    if not t:
        return []
    games = json.loads(t['games'])

    participants = conn.execute(
        'SELECT tp.user_id, u.username FROM tournament_participants tp JOIN users u ON u.id=tp.user_id WHERE tp.tournament_id=?',
        (tournament_id,)
    ).fetchall()

    standings = []
    for p in participants:
        uid = p['user_id']
        total = 0
        game_scores = {}
        for g in games:
            row = conn.execute(
                'SELECT score FROM tournament_scores WHERE tournament_id=? AND user_id=? AND game=?',
                (tournament_id, uid, g)
            ).fetchone()
            if row:
                raw = row['score']
                pts = calc_game_pts(g, raw)
                game_scores[g] = {'raw': raw, 'pts': pts}
                total += pts
            else:
                game_scores[g] = {'raw': None, 'pts': 0}
        standings.append({
            'user_id': uid,
            'username': p['username'],
            'total': total,
            'games': game_scores
        })

    standings.sort(key=lambda x: x['total'], reverse=True)
    return standings

# ============================================================
# PING
# ============================================================
@app.route('/api/ping', methods=['GET'])
def ping():
    return jsonify({'status': 'ok', 'version': '3.0'})

# ============================================================
# VERIFICA EMAIL
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
    action = data.get('action', '')

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
    game        = data.get('game', '').strip()
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
    action    = data.get('action', '')

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

# ============================================================
# TORNEI
# ============================================================
@app.route('/api/tournament/create', methods=['POST'])
def tournament_create():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data          = request.get_json(silent=True) or {}
    name          = data.get('name', '').strip()
    games         = data.get('games', [])
    deadline_h    = int(data.get('deadline_hours', 24))
    invite_names  = data.get('invites', [])  # lista di username amici

    if not name:
        return jsonify({'success': False, 'message': 'Nome torneo richiesto'}), 400
    valid_games = [g for g in games if g in ('snake', 'dodge', 'indovina')]
    if not valid_games:
        return jsonify({'success': False, 'message': 'Seleziona almeno un gioco'}), 400
    deadline_h = max(1, min(720, deadline_h))
    deadline   = (datetime.now() + timedelta(hours=deadline_h)).isoformat()

    conn = get_db()
    cursor = conn.execute(
        'INSERT INTO tournaments (name, creator_id, games, status, deadline) VALUES (?,?,?,?,?)',
        (name, user['id'], json.dumps(valid_games), 'active', deadline)
    )
    tid = cursor.lastrowid

    # Creator partecipa automaticamente
    conn.execute('INSERT INTO tournament_participants (tournament_id, user_id) VALUES (?,?)',
                 (tid, user['id']))

    # Invita gli amici
    invited = []
    for uname in invite_names:
        friend = conn.execute('SELECT id FROM users WHERE username=?', (uname,)).fetchone()
        if not friend:
            continue
        fs = conn.execute(
            "SELECT * FROM friendships WHERE status='accepted' AND ((requester_id=? AND addressee_id=?) OR (requester_id=? AND addressee_id=?))",
            (user['id'], friend['id'], friend['id'], user['id'])
        ).fetchone()
        if fs:
            try:
                conn.execute('INSERT INTO tournament_participants (tournament_id, user_id) VALUES (?,?)',
                             (tid, friend['id']))
                games_str = ', '.join(g.upper() for g in valid_games)
                add_notification(conn, friend['id'],
                    f'{user["username"]} ti ha invitato al torneo "{name}" [{games_str}]! Scade tra {deadline_h}h.',
                    'tournament')
                invited.append(uname)
            except Exception:
                pass

    conn.commit()
    conn.close()
    return jsonify({
        'success': True,
        'tournament_id': tid,
        'message': f'Torneo "{name}" creato! Invitati: {len(invited)} amici.',
        'invited': invited
    })

@app.route('/api/tournament/join', methods=['POST'])
def tournament_join():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data = request.get_json(silent=True) or {}
    tid  = data.get('tournament_id')

    conn = get_db()
    t = conn.execute(
        "SELECT * FROM tournaments WHERE id=? AND status='active' AND deadline > ?",
        (tid, datetime.now().isoformat())
    ).fetchone()
    if not t:
        conn.close()
        return jsonify({'success': False, 'message': 'Torneo non trovato o scaduto'}), 404

    try:
        conn.execute('INSERT INTO tournament_participants (tournament_id, user_id) VALUES (?,?)',
                     (tid, user['id']))
        creator = conn.execute('SELECT username FROM users WHERE id=?', (t['creator_id'],)).fetchone()
        add_notification(conn, t['creator_id'],
            f'{user["username"]} si e\' unito al tuo torneo "{t["name"]}"!', 'tournament')
        conn.commit()
        conn.close()
        return jsonify({'success': True, 'message': f'Sei entrato nel torneo "{t["name"]}"!'})
    except sqlite3.IntegrityError:
        conn.close()
        return jsonify({'success': False, 'message': 'Sei gia in questo torneo'}), 400

@app.route('/api/tournament/list', methods=['GET'])
def tournament_list():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    rows = conn.execute('''
        SELECT t.*, u.username AS creator_name,
               (SELECT COUNT(*) FROM tournament_participants WHERE tournament_id=t.id) AS participant_count
        FROM tournaments t
        JOIN users u ON u.id = t.creator_id
        JOIN tournament_participants tp ON tp.tournament_id = t.id AND tp.user_id = ?
        WHERE t.status IN ('active','completed')
        ORDER BY t.created_at DESC
        LIMIT 10
    ''', (user['id'],)).fetchall()

    result = []
    for r in rows:
        # Punteggio personale
        games = json.loads(r['games'])
        my_scores = {}
        for g in games:
            sr = conn.execute(
                'SELECT score FROM tournament_scores WHERE tournament_id=? AND user_id=? AND game=?',
                (r['id'], user['id'], g)
            ).fetchone()
            my_scores[g] = sr['score'] if sr else None

        # Posizione in classifica
        standings = get_tournament_standings(conn, r['id'])
        my_rank = next((i+1 for i,s in enumerate(standings) if s['user_id']==user['id']), 0)

        result.append({
            'id': r['id'],
            'name': r['name'],
            'creator': r['creator_name'],
            'is_creator': r['creator_id'] == user['id'],
            'games': games,
            'status': r['status'],
            'deadline': r['deadline'],
            'participant_count': r['participant_count'],
            'my_scores': my_scores,
            'my_rank': my_rank,
            'total_players': len(standings)
        })

    conn.close()
    return jsonify({'success': True, 'tournaments': result})

@app.route('/api/tournament/submit', methods=['POST'])
def tournament_submit():
    """Sottomette uno score ad un torneo specifico (o a tutti se tournament_id=0)."""
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data  = request.get_json(silent=True) or {}
    tid   = data.get('tournament_id', 0)
    game  = data.get('game', '').strip()
    score = int(data.get('score', 0))

    if game not in ('snake', 'dodge', 'indovina') or score < 0:
        return jsonify({'success': False, 'message': 'Dati non validi'}), 400

    conn  = get_db()
    now   = datetime.now().isoformat()
    updated = 0

    if tid == 0:
        # Sottometti a tutti i tornei attivi dell'utente che includono questo gioco
        rows = conn.execute('''
            SELECT t.id, t.games FROM tournaments t
            JOIN tournament_participants tp ON tp.tournament_id=t.id AND tp.user_id=?
            WHERE t.status='active' AND t.deadline > ?
        ''', (user['id'], now)).fetchall()
        targets = [(r['id'], json.loads(r['games'])) for r in rows if game in json.loads(r['games'])]
    else:
        row = conn.execute(
            "SELECT t.id, t.games FROM tournaments t JOIN tournament_participants tp ON tp.tournament_id=t.id AND tp.user_id=? WHERE t.id=? AND t.status='active' AND t.deadline>?",
            (user['id'], tid, now)
        ).fetchone()
        targets = [(row['id'], json.loads(row['games'])) for row in [row] if row and game in json.loads(row['games'])]

    for t_id, _ in targets:
        existing = conn.execute(
            'SELECT score FROM tournament_scores WHERE tournament_id=? AND user_id=? AND game=?',
            (t_id, user['id'], game)
        ).fetchone()
        if existing:
            # Aggiorna solo se migliore
            is_better = False
            if game in ('snake', 'dodge') and score > existing['score']:
                is_better = True
            elif game == 'indovina' and score < existing['score']:
                is_better = True
            if is_better:
                conn.execute(
                    'UPDATE tournament_scores SET score=?, submitted_at=? WHERE tournament_id=? AND user_id=? AND game=?',
                    (score, now, t_id, user['id'], game)
                )
                updated += 1
        else:
            conn.execute(
                'INSERT INTO tournament_scores (tournament_id, user_id, game, score, submitted_at) VALUES (?,?,?,?,?)',
                (t_id, user['id'], game, score, now)
            )
            updated += 1

    conn.commit()
    conn.close()
    return jsonify({'success': True, 'updated_tournaments': updated,
                    'message': f'Score inviato a {updated} torneo/i.'})

@app.route('/api/tournament/<int:tid>/standings', methods=['GET'])
def tournament_standings(tid):
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    conn = get_db()
    t = conn.execute('SELECT * FROM tournaments WHERE id=?', (tid,)).fetchone()
    if not t:
        conn.close()
        return jsonify({'success': False, 'message': 'Torneo non trovato'}), 404

    # Verifica partecipazione
    part = conn.execute(
        'SELECT id FROM tournament_participants WHERE tournament_id=? AND user_id=?',
        (tid, user['id'])
    ).fetchone()
    if not part:
        conn.close()
        return jsonify({'success': False, 'message': 'Non partecipi a questo torneo'}), 403

    standings = get_tournament_standings(conn, tid)
    games     = json.loads(t['games'])

    # Calcola scadenza rimanente
    try:
        deadline_dt = datetime.fromisoformat(t['deadline'])
        remaining   = max(0, int((deadline_dt - datetime.now()).total_seconds()))
        hours_left  = remaining // 3600
        mins_left   = (remaining % 3600) // 60
    except Exception:
        hours_left  = 0
        mins_left   = 0

    conn.close()
    return jsonify({
        'success': True,
        'tournament': {
            'id': t['id'],
            'name': t['name'],
            'games': games,
            'status': t['status'],
            'deadline': t['deadline'],
            'hours_left': hours_left,
            'mins_left': mins_left,
        },
        'standings': standings
    })

@app.route('/api/tournament/close', methods=['POST'])
def tournament_close():
    token = request.headers.get('Authorization', '').replace('Bearer ', '').strip()
    user  = get_user_by_token(token)
    if not user:
        return jsonify({'success': False, 'message': 'Non autorizzato'}), 401

    data = request.get_json(silent=True) or {}
    tid  = data.get('tournament_id')

    conn = get_db()
    t = conn.execute(
        "SELECT * FROM tournaments WHERE id=? AND creator_id=? AND status='active'",
        (tid, user['id'])
    ).fetchone()
    if not t:
        conn.close()
        return jsonify({'success': False, 'message': 'Torneo non trovato o non sei il creatore'}), 404

    standings = get_tournament_standings(conn, tid)
    winner_id = None
    winner_name = 'Nessuno'
    if standings:
        winner_id   = standings[0]['user_id']
        winner_name = standings[0]['username']

    conn.execute("UPDATE tournaments SET status='completed', winner_id=? WHERE id=?",
                 (winner_id, tid))

    # Notifica tutti i partecipanti
    parts = conn.execute(
        'SELECT user_id FROM tournament_participants WHERE tournament_id=?', (tid,)
    ).fetchall()
    for p in parts:
        if p['user_id'] == winner_id:
            add_notification(conn, p['user_id'],
                f'Hai VINTO il torneo "{t["name"]}"! Congratulazioni!', 'tournament_win')
        else:
            add_notification(conn, p['user_id'],
                f'Il torneo "{t["name"]}" e\' terminato. Vincitore: {winner_name}!', 'tournament_end')

    conn.commit()
    conn.close()
    return jsonify({
        'success': True,
        'winner': winner_name,
        'standings': standings,
        'message': f'Torneo concluso! Vincitore: {winner_name}'
    })

if __name__ == '__main__':
    init_db()
    print("=== VGC Backend Server v3.0 ===")
    print(f"Database: {DB_PATH}")
    print(f"Email: {EMAIL_USER or '(non configurata)'}")
    print("Server: http://localhost:8000")
    app.run(host='localhost', port=8000, debug=False)
