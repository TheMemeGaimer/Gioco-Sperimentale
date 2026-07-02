#!/usr/bin/env bash
# ============================================================
# Valentino Game Collection — itch.io Package Builder
# Genera un archivio .zip pronto per il caricamento su itch.io
# ============================================================
set -e

GAME_NAME="ValentinoGameCollection"
VERSION="2.0"
PLATFORM=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)
DIST_DIR="dist/${GAME_NAME}-${VERSION}-${PLATFORM}-${ARCH}"
ZIP_NAME="dist/${GAME_NAME}-${VERSION}-${PLATFORM}-${ARCH}.zip"

echo ""
echo "  ╔══════════════════════════════════════════════╗"
echo "  ║   Valentino Game Collection — itch.io Build  ║"
echo "  ╚══════════════════════════════════════════════╝"
echo ""

# 1. Compila il gioco
echo "  >> Compilazione in corso..."
make clean 2>/dev/null || true
make
echo "  [OK] Binario compilato: ./gioco"

# 2. Crea struttura dist
rm -rf "${DIST_DIR}"
mkdir -p "${DIST_DIR}/backend"

# 3. Copia file necessari
cp gioco             "${DIST_DIR}/"
cp backend/app.py    "${DIST_DIR}/backend/"
cp Makefile          "${DIST_DIR}/"
cp install.sh        "${DIST_DIR}/" 2>/dev/null || true
cp launch.sh         "${DIST_DIR}/" 2>/dev/null || true

# Copia sorgente per ricompilazione su target
mkdir -p "${DIST_DIR}/src"
cp src/menu.cpp "${DIST_DIR}/src/"

# 4. Crea script di avvio per l'utente finale
cat > "${DIST_DIR}/START.sh" << 'EOF'
#!/usr/bin/env bash
# ============================================================
# Valentino Game Collection — Avvio
# ============================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo ""
echo "  Valentino Game Collection v2.0"
echo "  ================================"
echo ""

# Avvia backend se python3 è disponibile
if command -v python3 &>/dev/null; then
    echo "  [*] Avvio backend server..."
    python3 backend/app.py &
    BACKEND_PID=$!
    sleep 1
    echo "  [OK] Backend attivo (PID $BACKEND_PID)"
else
    echo "  [!] python3 non trovato — modalità offline"
fi

# Avvia il gioco
./gioco

# Ferma il backend alla chiusura
if [ -n "$BACKEND_PID" ]; then
    kill "$BACKEND_PID" 2>/dev/null || true
fi
EOF
chmod +x "${DIST_DIR}/START.sh"

# Script Windows
cat > "${DIST_DIR}/START.bat" << 'EOF'
@echo off
title Valentino Game Collection
cd /d "%~dp0"
echo.
echo   Valentino Game Collection v2.0
echo   ================================
echo.
start /b python backend\app.py
timeout /t 2 /nobreak >nul
gioco.exe
EOF

# 5. README per itch.io
cat > "${DIST_DIR}/README.txt" << 'EOF'
==============================================
  VALENTINO GAME COLLECTION v2.0
  by The_Meme_Gaimer — TMG Studio 2026
==============================================

REQUISITI:
  - Terminal / Prompt dei comandi
  - g++ con C++17 (se ricompilazione necessaria)
  - Python 3 + Flask (per funzionalità online)
  - curl (per connessione backend)

AVVIO RAPIDO (Linux/macOS):
  chmod +x START.sh
  ./START.sh

AVVIO RAPIDO (Windows):
  Doppio click su START.bat
  (oppure gioco.exe da terminale)

AVVIO MANUALE:
  Terminale 1: python3 backend/app.py
  Terminale 2: ./gioco

MODALITÀ OFFLINE:
  Il gioco funziona anche senza backend.
  Seleziona "Gioca offline" nella schermata iniziale.

GIOCHI INCLUSI:
  1. Indovina il Numero — 3 difficoltà
  2. Snake — classico con record online
  3. Dodge Game — schiva gli ostacoli

FUNZIONALITÀ:
  - Profili online con email verificata
  - Leaderboard globale
  - Sistema achievements (21+)
  - Sfide tra amici
  - Sistema tornei bracket-style
  - Multi-lingua (IT/EN/FR)
  - Tutorial interattivo

SUPPORTO:
  github.com/TheMemeGaimer/Gioco-Sperimentale

(C) TMG Studio 2026. Tutti i Diritti Riservati.
EOF

# 6. Crea il file backend requirements
cat > "${DIST_DIR}/backend/requirements.txt" << 'EOF'
flask>=2.0
EOF

# 7. Zip finale
mkdir -p dist
cd "$(dirname "${DIST_DIR}")"
zip -r "../${ZIP_NAME#dist/}" "${GAME_NAME}-${VERSION}-${PLATFORM}-${ARCH}/" > /dev/null
cd ..

echo ""
echo "  ╔══════════════════════════════════════╗"
echo "  ║  Build completata con successo!       ║"
echo "  ╚══════════════════════════════════════╝"
echo ""
echo "  File: ${ZIP_NAME}"
echo "  Dim.: $(du -sh "${ZIP_NAME}" | cut -f1)"
echo ""
echo "  Come caricare su itch.io:"
echo "  1. Vai su https://itch.io/dashboard"
echo "  2. Crea un nuovo progetto ('New project')"
echo "  3. Carica ${ZIP_NAME} come upload"
echo "  4. Imposta 'Kind of project': Downloadable"
echo "  5. Pubblica!"
echo ""
