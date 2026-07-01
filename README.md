# Valentino Game Collection

Raccolta di mini-giochi con tema Matrix per terminale. Cross-platform (Windows, Linux, macOS).

## Giochi
- **Indovina il Numero** — 3 difficoltà, record di tempo e tentativi
- **Snake** — classico Snake con difficoltà variabile
- **Dodge Game** — schiva gli ostacoli che cadono

## Funzionalità moderne
- **Installazione automatica** al primo avvio (wizard)
- **Profili online** con email e password (backend Flask)
- **Leaderboard globale** online
- **Sistema achievements** (21 achievement)
- **Salvataggio cloud** dei progressi
- **Modalità offline** con profili locali

## Requisiti
- `g++` con C++17
- `python3` + `flask`
- `curl` (per HTTP dal gioco)

## Installazione rapida
```bash
chmod +x install.sh
./install.sh
./launch.sh
```

## Avvio manuale
```bash
# Terminale 1 — Backend
python3 backend/app.py

# Terminale 2 — Gioco
make && ./gioco
```

## Struttura
```
├── src/menu.cpp        # Gioco C++ cross-platform
├── backend/app.py      # Server Flask REST API
├── backend/vgc.db      # Database SQLite (generato)
├── profiles/           # Profili locali (generati)
├── Makefile
└── install.sh
```
