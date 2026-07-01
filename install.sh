#!/bin/bash
# ============================================================
# VALENTINO GAME COLLECTION - Script di Installazione
# ============================================================
set -e

GREEN='\033[0;92m'
CYAN='\033[0;96m'
YELLOW='\033[0;93m'
RED='\033[0;91m'
RESET='\033[0m'

echo -e "${CYAN}"
echo "  ============================================"
echo "     VALENTINO GAME COLLECTION - Installer"
echo "  ============================================"
echo -e "${RESET}"
sleep 0.5

# Requisiti
echo -e "${YELLOW}[1/5] Verifica requisiti...${RESET}"
command -v g++ >/dev/null 2>&1 || { echo -e "${RED}g++ non trovato. Installa build-essential.${RESET}"; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo -e "${RED}python3 non trovato.${RESET}"; exit 1; }
command -v curl >/dev/null 2>&1 || { echo -e "${RED}curl non trovato.${RESET}"; exit 1; }
echo -e "${GREEN}  OK${RESET}"

# Dipendenze Python
echo -e "${YELLOW}[2/5] Installazione dipendenze backend...${RESET}"
pip3 install -r backend/requirements.txt -q
echo -e "${GREEN}  OK${RESET}"

# Compilazione
echo -e "${YELLOW}[3/5] Compilazione gioco C++...${RESET}"
make clean
make all
echo -e "${GREEN}  OK${RESET}"

# Directory dati
echo -e "${YELLOW}[4/5] Creazione directory dati...${RESET}"
mkdir -p profiles
echo -e "${GREEN}  OK${RESET}"

# Launcher
echo -e "${YELLOW}[5/5] Creazione launcher...${RESET}"
cat > launch.sh << 'EOF'
#!/bin/bash
# Avvia il backend in background
python3 backend/app.py &
BACKEND_PID=$!
sleep 1
# Avvia il gioco
./gioco
# Ferma il backend all'uscita
kill $BACKEND_PID 2>/dev/null
EOF
chmod +x launch.sh
echo -e "${GREEN}  OK${RESET}"

echo ""
echo -e "${GREEN}  ============================================"
echo "     Installazione completata!"
echo "  ============================================${RESET}"
echo ""
echo "  Per avviare:  ./launch.sh"
echo "  Solo gioco:   ./gioco"
echo "  Solo backend: python3 backend/app.py"
echo ""
