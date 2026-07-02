# 🎮 Valentino Game Collection

<div align="center">

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue?style=flat-square)](https://github.com/TheMemeGaimer/Gioco-Sperimentale)
[![Language](https://img.shields.io/badge/language-C%2B%2B17-orange?style=flat-square)](https://en.cppreference.com/w/cpp/17)
[![Backend](https://img.shields.io/badge/backend-Python%20Flask-green?style=flat-square)](https://flask.palletsprojects.com/)
[![License](https://img.shields.io/badge/license-TMG%20Studio%202026-purple?style=flat-square)](LICENSE)
[![itch.io](https://img.shields.io/badge/download-itch.io-FA5C5C?style=flat-square&logo=itch.io)](https://thememeGaimer.itch.io)

**Raccolta di mini-giochi in stile Matrix per terminale — cross-platform, con backend online, tornei e molto altro.**

*by [The_Meme_Gaimer](https://github.com/TheMemeGaimer) — © TMG Studio 2026*

</div>

---

## 🖥️ Anteprima

```
  ╔══════════════════════════════════════════════════════╗
  ║         VALENTINO GAME COLLECTION v2.0               ║
  ║                  by The_Meme_Gaimer                  ║
  ╚══════════════════════════════════════════════════════╝

  [ SERVER ONLINE DISPONIBILE ]    ● GiocatoreX   [ONLINE]

    1)  Indovina il numero
    2)  Snake
    3)  Dodge Game
    4)  Achievements
    5)  Leaderboard
    6)  Amici
    7)  Sfide
    8)  Notifiche
    9)  Statistiche
   10)  Tornei
   11)  Impostazioni
   12)  Esci

  W/S o frecce: naviga   INVIO: seleziona
```

---

## 🎮 Giochi

### 🔢 Indovina il Numero
> Trova il numero segreto nel minor numero di tentativi possibile!

```
=== Indovina il Numero ===

  1 - 100 ?

  # 50    →  TOO HIGH
  # 25    →  TOO LOW
  # 37    →  TOO HIGH
  # 31    →  GOT IT in 4 t., 12s!  ★ Record!
```

- **3 difficoltà**: Facile (1–20), Medio (1–50), Difficile (1–100)
- Record di tempo e di tentativi salvati nel cloud
- Achievement segreti per le vittorie rapide

---

### 🐍 Snake
> Il classico Snake ASCII — più mangi, più acceleri!

```
┌──────────────────────────────────┐
│  ·  ·  ·  ·  ·  ·  ·  ·  ·  · │
│  ·  ·  ·  O  ·  ·  ·  ·  ·  · │
│  ·  ·  ·  #  ·  ·  ·  ·  ·  · │
│  ·  ·  ·  ###  ·  ·  ·  ·  ·  │
│  ·  ·  ·  ·  ###  ·  *  ·  ·  │
│  ·  ·  ·  ·  ·  ###  ·  ·  ·  │
│  ·  ·  ·  ·  ·  ·  ·  ·  ·  · │
└──────────────────────────────────┘
   Score: 24   Best: 50   [MEDIUM]
```

- Controlli: WASD o frecce direzionali
- 3 difficoltà (velocità crescente)
- Leaderboard globale online

---

### 🎯 Dodge Game
> Sopravvivi il più a lungo agli ostacoli in caduta!

```
┌─────────────────────────────────┐
│  ·    #    ·    #    ·    ·    │
│  ·    ·    #    ·    #    ·    │
│  ·    ·    ·    ·    #    ·    │
│  ·    ·    ·    ·    ·    ·    │
│  ·    ·    ·    ^    ·    ·    │
└─────────────────────────────────┘
   Tempo: 23s   Best: 47s
```

- Controlli: A/D o frecce sinistra/destra
- Punteggio = secondi sopravvissuti
- Velocità degli ostacoli cresce nel tempo

---

## ✨ Funzionalità

| Feature | Descrizione |
|---------|-------------|
| 🌐 **Profili Online** | Registrazione con verifica email + login sicuro |
| 🏆 **Leaderboard** | Classifica globale live per ogni gioco |
| 🤝 **Sistema Amici** | Aggiungi amici e monitora i loro punteggi |
| ⚔️ **Sfide** | Manda sfide dirette ad altri giocatori |
| 🔔 **Notifiche** | Ricevi avvisi per sfide, tornei e novità |
| 🥇 **Tornei** | Bracket-style con punteggio cumulativo cross-game |
| 🏅 **21 Achievement** | Da "First Login" a "Untouchable" |
| 📊 **Statistiche** | Grafici ASCII degli ultimi 10 punteggi |
| 🌍 **Multi-lingua** | Italiano · English · Français |
| 🎓 **Tutorial** | Guida interattiva saltabile per i nuovi giocatori |
| 💾 **Cloud Save** | Progressi sincronizzati ovunque |
| 🔌 **Offline Mode** | Tutto funziona anche senza internet |

---

## 🥇 Sistema Tornei

```
====== TORNEI ======

  Nome:    Torneo Estivo 2026
  Giochi:  Snake + Dodge + Indovina
  Host:    Valentino
  Scade:   5h 23m

  Classifica:
  ┌─────────────────────────────────────────────┐
  │  #   Giocatore     Snake   Dodge   Indov.   │
  ├─────────────────────────────────────────────┤
  │  1   Valentino      48      35      9        │
  │  2   Marco          31      41      7        │
  │  3   Luigi  ← TU   22      28      8        │
  └─────────────────────────────────────────────┘
```

- Crea tornei con nome, giochi scelti e durata personalizzata
- Invita amici direttamente per username
- Punteggi inviati **automaticamente** dopo ogni partita
- Vincitore annunciato a tutti i partecipanti alla chiusura

---

## 🎓 Tutorial Interattivo

```
====== TUTORIAL ======  (pagina 1/3)

  Benvenuto in Valentino Game Collection!

  Navigazione:
    W / S  o  Frecce su/giu  →  Sposta il cursore
    INVIO                    →  Seleziona voce
    ESC                      →  Torna indietro

  In ogni schermata trovi:
    - In alto:  titolo e stato online
    - Al centro: le opzioni del menu
    - In basso:  la legenda dei tasti

  [S] Salta tutorial        INVIO: pagina successiva
```

Disponibile in **Italiano**, **English** e **Français** — saltabile con `S` in qualsiasi momento.

---

## 📥 Download e Installazione

### Via itch.io (consigliato)
1. Scarica l'ultima versione da **[itch.io →](https://thememeGaimer.itch.io)**
2. Estrai l'archivio
3. Esegui `START.sh` (Linux/macOS) oppure `START.bat` (Windows)

### Da sorgente (GitHub)

```bash
# Clona il repository
git clone https://github.com/TheMemeGaimer/Gioco-Sperimentale.git
cd Gioco-Sperimentale

# Installazione automatica
chmod +x install.sh && ./install.sh

# Oppure manuale:
# Terminale 1 — Backend
python3 backend/app.py

# Terminale 2 — Gioco
make && ./gioco
```

### Requisiti

| Componente | Versione minima | Note |
|-----------|-----------------|------|
| `g++` | C++17 (GCC 7+) | Obbligatorio |
| `python3` | 3.8+ | Solo per funzionalità online |
| `flask` | 2.0+ | `pip install flask` |
| `curl` | qualsiasi | Per HTTP dal gioco |

---

## 📦 Struttura del Progetto

```
Gioco-Sperimentale/
├── src/
│   └── menu.cpp            # Tutto il gioco C++ (cross-platform)
├── backend/
│   ├── app.py              # Server Flask REST API v3.0
│   └── vgc.db              # Database SQLite (generato)
├── profiles/               # Profili locali offline (generati)
├── Makefile
├── install.sh              # Installazione automatica
├── launch.sh               # Avvio rapido
└── package_itch.sh         # Build + zip per itch.io
```

---

## 🏅 Achievement

<details>
<summary>Mostra tutti i 21 achievement</summary>

| Achievement | Come sbloccare |
|-------------|----------------|
| Verified | Verifica l'email alla registrazione |
| First Login | Primo accesso online |
| Speedrunner | Indovina in meno di 5 secondi |
| Perfect Guess | Indovina al primo tentativo |
| Hardcore Mode | Vinci in difficoltà Hard |
| Persistence | 3 vittorie consecutive |
| Snake Rookie | Raggiungi 10 punti a Snake |
| Snake Survivor | Raggiungi 30 punti a Snake |
| Snake Master | Raggiungi 50 punti a Snake |
| Dodger | Sopravvivi 20 secondi a Dodge |
| Ultra Dodger | Sopravvivi 40 secondi a Dodge |
| Untouchable | Sopravvivi 60 secondi a Dodge |
| Social | Aggiungi il tuo primo amico |
| Challenger | Invia la tua prima sfida |
| Accepted | Accetta una sfida |
| Champion | Vinci una sfida |
| Statistician | Guarda le tue statistiche |
| Multilingual | Cambia la lingua del gioco |
| Saver | Salva manualmente il profilo |
| Notified | Controlla le notifiche |
| Tournament | Partecipa al tuo primo torneo |

</details>

---

## 🌐 API Backend (v3.0)

<details>
<summary>Mostra tutti gli endpoint</summary>

```
GET  /api/ping                        Status server
POST /api/send-verification           Invia codice email
POST /api/register                    Registrazione utente
POST /api/login                       Login utente
POST /api/score                       Invia punteggio
GET  /api/leaderboard                 Classifica globale
POST /api/friends/add                 Aggiungi amico
GET  /api/friends                     Lista amici
POST /api/challenge/send              Invia sfida
GET  /api/challenges                  Lista sfide ricevute
GET  /api/notifications               Notifiche utente
POST /api/tournament/create           Crea torneo
POST /api/tournament/join             Unisciti a torneo
GET  /api/tournament/list             Lista tornei attivi
POST /api/tournament/submit           Invia punteggio torneo
GET  /api/tournament/<id>/standings   Classifica torneo
POST /api/tournament/close            Chiudi torneo
```

</details>

---

## 👥 Credits

| Ruolo | Nome |
|-------|------|
| **Founder & Developer** | Valentino Ingrao |
| **Studio** | TMG Studio |

**Ringraziamenti speciali:**
*Alberto · Francesco · Luca · Matteo · Andrea · Simone · Marco · Giulia*

---

## 📄 Licenza

© TMG Studio 2026 — Valentino Ingrao. Tutti i Diritti Riservati.

---

<div align="center">

**Se ti piace il progetto, lascia una stella su GitHub!**

[Gioca su itch.io](https://thememeGaimer.itch.io) · [Segnala un Bug](https://github.com/TheMemeGaimer/Gioco-Sperimentale/issues) · [Discussioni](https://github.com/TheMemeGaimer/Gioco-Sperimentale/discussions)

</div>
