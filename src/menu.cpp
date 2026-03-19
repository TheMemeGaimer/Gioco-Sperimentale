#include <iostream>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include <string>
#include <vector>
#include <mmsystem.h>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <limits>
#include <cstring>

#pragma comment(lib, "winmm.lib")

using namespace std;
namespace fs = std::filesystem;

// prevent windows.h macro collisions with std::min/std::max
#undef max
#undef min

// Profile file format
static const char PROFILE_MAGIC[4] = { 'V','G','C','1' };
static const uint32_t PROFILE_VERSION = 2;

const int WIDTH = 80;
const int HEIGHT = 25;

int g_speedBoost = 0;
bool g_musicOn = true;

string g_playerName;
int g_migliorTempo = 9999;
int g_migliorTentativi = 9999;
int g_vittorieConsecutive = 0;

// RECORD NUOVI PER GIOCHI
int g_snakeBestScore = 0;
int g_snakeBestDifficulty = 0;   // 1=Facile,2=Medio,3=Difficile
int g_dodgeBestTime = 0;         // in secondi
int g_dodgeBestDifficulty = 0;

// ------------------------------------------------------------
// SISTEMA ACHIEVEMENTS AVANZATO
// ------------------------------------------------------------
struct Achievement {
    string name;
    string description;
    bool unlocked = false;
    int difficulty;
    // 0 = facile
    // 1 = medio
    // 2 = difficile
    // 3 = segreto
};

vector<Achievement> achievements;

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void gotoXY(int x, int y) {
    COORD c;
    c.X = x;
    c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void achievementPopup(const string& name) {
    for (int i = 0; i < 8; i++) {
        system("cls");
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (rand() % 4 == 0)
                    cout << (char)(33 + rand() % 94);
                else
                    cout << " ";
            }
            cout << "\n";
        }
        Beep(900 + rand() % 400, 20);
        Sleep(40);
    }

    system("cls");

    setColor(14);
    gotoXY(20, 10);
    cout << "=== ACHIEVEMENT UNLOCKED ===";

    setColor(10);
    gotoXY(25, 12);
    cout << name;

    Beep(1200, 150);
    Beep(1500, 150);
    Sleep(800);

    for (int step = 0; step < HEIGHT; step++) {
        system("cls");
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (y < step)
                    cout << " ";
                else
                    cout << (char)(33 + rand() % 94);
            }
            cout << "\n";
        }
        Sleep(20);
    }

    system("cls");
    setColor(7);
}

void addAchievement(string name, string description, int difficulty) {
    achievements.push_back({ name, description, false, difficulty });
}

void unlockAchievement(const string& name) {
    for (auto& a : achievements) {
        if (a.name == name && !a.unlocked) {
            a.unlocked = true;
            achievementPopup(a.name);
        }
    }
}

void initAchievements() {
    achievements.clear();

    // Facili (0)
    addAchievement("THE ONE", "Hai effettuato l'accesso come Neo.", 0);
    addAchievement("Connection Established", "Hai effettuato l'accesso come Trinity.", 0);
    addAchievement("Free Your Mind", "Hai effettuato l'accesso come Morpheus.", 0);
    addAchievement("Dev God Mode", "Il creatore e' entrato nel sistema.", 0);
    addAchievement("The Architect", "Hai effettuato l'accesso come Architect.", 0);
    addAchievement("Sound of the Matrix", "Hai attivato la musica di sottofondo.", 0);
    addAchievement("Red Awakening", "Hai scelto la pillola rossa.", 0);
    addAchievement("Blue Dream", "Hai scelto la pillola blu.", 0);

    // Medi (1)
    addAchievement("Déjà-vu", "Hai visto il gatto due volte.", 1);
    addAchievement("Secret Door", "Hai aperto la stanza segreta.", 1);
    addAchievement("System Error", "Hai trovato il codice 404 nascosto.", 1);

    // Difficili (2)
    addAchievement("Speedrunner", "Hai vinto il mini-gioco in meno di 5 secondi.", 2);
    addAchievement("Perfect Guess", "Hai indovinato al primo tentativo.", 2);
    addAchievement("Hardcore Mode", "Hai vinto in difficolta' massima.", 2);
    addAchievement("Persistence", "Hai vinto tre volte di fila.", 2);

    // SNAKE
    addAchievement("Snake Rookie", "Hai raggiunto 10 punti in Snake.", 0);
    addAchievement("Snake Survivor", "Hai raggiunto 30 punti in Snake.", 1);
    addAchievement("Snake Master", "Hai raggiunto 50 punti in Snake.", 2);

    // DODGE GAME
    addAchievement("Dodger", "Hai sopravvissuto 20 secondi in Dodge Game.", 0);
    addAchievement("Ultra Dodger", "Hai sopravvissuto 40 secondi in Dodge Game.", 1);
    addAchievement("Untouchable", "Hai sopravvissuto 60 secondi in Dodge Game.", 2);

    // Segreti (3) - spazio per aggiungerne altri
}

// ------------------------------------------------------------
// SALVATAGGIO PROFILO
// ------------------------------------------------------------
void ensureProfilesDir() {
    if (!fs::exists("profiles")) {
        std::error_code ec;
        fs::create_directory("profiles", ec);
        // ignore error but do not throw
    }
}

string profilePath(const string& name) {
    return string("profiles/") + name + ".dat";
}

bool saveCurrentProfile() {
    if (g_playerName.empty()) return false;
    ofstream out(profilePath(g_playerName), ios::binary);
    if (!out) return false;

    // write magic + version
    out.write(PROFILE_MAGIC, sizeof(PROFILE_MAGIC));
    uint32_t ver = PROFILE_VERSION;
    out.write((char*)&ver, sizeof(ver));

    // write name (uint32_t length)
    uint32_t len = static_cast<uint32_t>(g_playerName.size());
    out.write((char*)&len, sizeof(len));
    out.write(g_playerName.c_str(), len);

    out.write((char*)&g_migliorTempo, sizeof(int));
    out.write((char*)&g_migliorTentativi, sizeof(int));
    out.write((char*)&g_vittorieConsecutive, sizeof(int));
    out.write((char*)&g_musicOn, sizeof(bool));

    // achievements: write count then states
    uint32_t achCount = static_cast<uint32_t>(achievements.size());
    out.write((char*)&achCount, sizeof(achCount));
    for (auto& a : achievements) {
        out.write((char*)&a.unlocked, sizeof(bool));
    }

    // nuovi record giochi
    out.write((char*)&g_snakeBestScore, sizeof(int));
    out.write((char*)&g_snakeBestDifficulty, sizeof(int));
    out.write((char*)&g_dodgeBestTime, sizeof(int));
    out.write((char*)&g_dodgeBestDifficulty, sizeof(int));

    out.close();
    return true;
}

bool loadProfile(const string& name) {
    ifstream in(profilePath(name), ios::binary);
    if (!in) return false;
    // try to read magic first
    char magic[4];
    if (!in.read(magic, sizeof(magic))) {
        in.close();
        return false;
    }

    if (memcmp(magic, PROFILE_MAGIC, sizeof(magic)) == 0) {
        // new format
        uint32_t ver = 0;
        if (!in.read((char*)&ver, sizeof(ver))) { in.close(); return false; }

        uint32_t len = 0;
        if (!in.read((char*)&len, sizeof(len))) { in.close(); return false; }
        g_playerName.resize(len);
        if (!in.read(&g_playerName[0], len)) { in.close(); return false; }

        in.read((char*)&g_migliorTempo, sizeof(int));
        in.read((char*)&g_migliorTentativi, sizeof(int));
        in.read((char*)&g_vittorieConsecutive, sizeof(int));
        in.read((char*)&g_musicOn, sizeof(bool));

        uint32_t achCount = 0;
        if (!in.read((char*)&achCount, sizeof(achCount))) achCount = 0;

        for (uint32_t i = 0; i < achCount; ++i) {
            bool state = false;
            if (!in.read((char*)&state, sizeof(bool))) break;
            if (i < achievements.size()) achievements[i].unlocked = state;
        }

        // if file has fewer achievements than current, ensure others are false
        for (size_t i = achCount; i < achievements.size(); ++i) achievements[i].unlocked = false;

        // read new game records
        if (!in.read((char*)&g_snakeBestScore, sizeof(int))) {
            g_snakeBestScore = 0;
            g_snakeBestDifficulty = 0;
            g_dodgeBestTime = 0;
            g_dodgeBestDifficulty = 0;
        }
        else {
            in.read((char*)&g_snakeBestDifficulty, sizeof(int));
            in.read((char*)&g_dodgeBestTime, sizeof(int));
            in.read((char*)&g_dodgeBestDifficulty, sizeof(int));
        }

        in.close();
        return true;
    }
    else {
        // legacy format: reset to start and parse old layout
        in.clear();
        in.seekg(0, ios::beg);

        size_t len = 0;
        in.read((char*)&len, sizeof(len));
        if (!in) { in.close(); return false; }

        g_playerName.resize(len);
        in.read(&g_playerName[0], len);

        in.read((char*)&g_migliorTempo, sizeof(int));
        in.read((char*)&g_migliorTentativi, sizeof(int));
        in.read((char*)&g_vittorieConsecutive, sizeof(int));
        in.read((char*)&g_musicOn, sizeof(bool));

        for (auto& a : achievements) {
            bool state = false;
            if (!in.read((char*)&state, sizeof(bool))) break;
            a.unlocked = state;
        }

        // try to read the newer records (if present)
        if (in.read((char*)&g_snakeBestScore, sizeof(int))) {
            in.read((char*)&g_snakeBestDifficulty, sizeof(int));
            in.read((char*)&g_dodgeBestTime, sizeof(int));
            in.read((char*)&g_dodgeBestDifficulty, sizeof(int));
        }
        else {
            g_snakeBestScore = 0;
            g_snakeBestDifficulty = 0;
            g_dodgeBestTime = 0;
            g_dodgeBestDifficulty = 0;
        }

        in.close();
        return true;
    }
}

vector<string> listProfiles() {
    vector<string> names;
    if (!fs::exists("profiles")) return names;

    for (auto& entry : fs::directory_iterator("profiles")) {
        if (entry.is_regular_file()) {
            auto p = entry.path();
            if (p.extension() == ".dat") {
                names.push_back(p.stem().string());
            }
        }
    }
    return names;
}

// ------------------------------------------------------------
// UTILITY
// ------------------------------------------------------------
void hideCursor() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    if (GetConsoleCursorInfo(out, &cursorInfo)) {
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(out, &cursorInfo);
    }
}

// Prompt helper that keeps asking until a valid integer is entered
int promptInt(const string& prompt) {
    while (true) {
        cout << prompt;
        int v;
        if (cin >> v) {
            return v;
        }
        cout << "Input non valido. Riprova.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

void slowPrint(string text, int delay = 30) {
    for (char c : text) {
        cout << c;
        Sleep(delay);
    }
}

void blinkText(string text, int times = 3, int delay = 200) {
    for (int i = 0; i < times; i++) {
        setColor(14);
        cout << text;
        Sleep(delay);
        cout << "\r";
        setColor(7);
        cout << string(text.size(), ' ');
        cout << "\r";
        Sleep(delay);
    }
    cout << text << "\n";
}

void startBackgroundMusic() {
    if (g_musicOn) {
        std::error_code ec;
        if (fs::exists("music.wav", ec)) {
            PlaySound(TEXT("music.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
            unlockAchievement("Sound of the Matrix");
        }
        else {
            // silent fail if no music file
        }
    }
}

void stopBackgroundMusic() {
    PlaySound(NULL, 0, 0);
}

string difficultyName(int d) {
    if (d == 1) return "Facile";
    if (d == 2) return "Medio";
    if (d == 3) return "Difficile";
    return "-";
}

// ------------------------------------------------------------
// TRANSIZIONI
// ------------------------------------------------------------
void transitionGlitch() {
    for (int i = 0; i < 12; i++) {
        system("cls");
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (rand() % 3 == 0) {
                    setColor(15);
                    cout << (char)(33 + rand() % 94);
                }
                else {
                    cout << " ";
                }
            }
            cout << "\n";
        }
        Beep(600 + rand() % 400, 20);
        Sleep(40);
    }
    system("cls");
}

void transitionCRT() {
    for (int step = 0; step < WIDTH / 2; step++) {
        system("cls");
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = step; x < WIDTH - step; x++) {
                cout << " ";
            }
            cout << "\n";
        }
        Sleep(15);
    }

    system("cls");
    for (int y = 0; y < HEIGHT; y++) {
        gotoXY(WIDTH / 2, y);
        cout << "|";
    }
    Sleep(200);

    system("cls");
    Sleep(100);
}

void transitionMatrixDissolve() {
    vector<string> screen(HEIGHT, string(WIDTH, ' '));

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            gotoXY(x, y);
            CHAR_INFO ci;
            // ensure ci is zeroed
            memset(&ci, 0, sizeof(ci));
            SMALL_RECT rect = { (SHORT)x, (SHORT)y, (SHORT)x, (SHORT)y };
            COORD bufSize = { 1, 1 };
            COORD bufCoord = { 0, 0 };
            BOOL ok = ReadConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE), &ci, bufSize, bufCoord, &rect);
            if (ok) {
                screen[y][x] = ci.Char.AsciiChar;
            }
            else {
                screen[y][x] = ' ';
            }
        }
    }

    for (int step = 0; step < HEIGHT; step++) {
        system("cls");
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (y < step)
                    cout << " ";
                else
                    cout << screen[y][x];
            }
            cout << "\n";
        }
        Sleep(20);
    }

    system("cls");
}

void playTransition(int mode, const string& nomeUtente = "") {
    if (mode == 4) {
        if (nomeUtente == "Neo") {
            transitionMatrixDissolve();
            return;
        }
        if (nomeUtente == "Trinity") {
            transitionGlitch();
            return;
        }
        if (nomeUtente == "Morpheus") {
            transitionCRT();
            return;
        }
        if (nomeUtente == "Architect") {
            transitionCRT();
            return;
        }
        if (nomeUtente == "Valentino") {
            int r = rand() % 3;
            if (r == 0) transitionGlitch();
            if (r == 1) transitionCRT();
            if (r == 2) transitionMatrixDissolve();
            return;
        }
    }

    if (mode == 0) {
        int r = rand() % 3;
        if (r == 0) transitionGlitch();
        if (r == 1) transitionCRT();
        if (r == 2) transitionMatrixDissolve();
        return;
    }

    if (mode == 1) transitionGlitch();
    if (mode == 2) transitionCRT();
    if (mode == 3) transitionMatrixDissolve();
}

// ------------------------------------------------------------
// MATRIX RAIN
// ------------------------------------------------------------
void matrixRainPro(int durationMs = 3000) {
    const int cols = WIDTH;
    int headY[cols];
    int speed[cols];

    for (int i = 0; i < cols; i++) {
        headY[i] = rand() % HEIGHT;
        speed[i] = 1 + rand() % 3;
    }

    DWORD start = GetTickCount();

    while (GetTickCount() - start < (DWORD)durationMs) {
        system("cls");

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < cols; x++) {
                int h = headY[x];

                if (y == h) {
                    setColor(10);
                    cout << (char)(33 + rand() % 94);
                }
                else if (y < h && y > h - 6) {
                    setColor(2);
                    cout << (char)(33 + rand() % 94);
                }
                else {
                    cout << " ";
                }
            }
            cout << "\n";
        }

        for (int i = 0; i < cols; i++) {
            headY[i] += speed[i];
            if (headY[i] - 6 > HEIGHT + rand() % 10) {
                headY[i] = -(rand() % HEIGHT);
                speed[i] = 1 + rand() % 3;
            }
        }

        Sleep(70);
    }

    setColor(7);
    system("cls");
}

void matrixBackgroundFrame(int width, int height, int headY[], int speed[]) {
    for (int x = 0; x < width; x++) {

        bool glitch = (rand() % 40 == 0);

        int h = headY[x];

        if (h >= 0 && h < height) {
            gotoXY(x, h);
            setColor(glitch ? 15 : 10);
            cout << (char)(33 + rand() % 94);
        }

        for (int t = 1; t < 6; t++) {
            int y = h - t;
            if (y >= 0 && y < height) {
                gotoXY(x, y);
                setColor(2);
                cout << (char)(33 + rand() % 94);
            }
        }

        int tail = h - 6;
        if (tail >= 0 && tail < height) {
            gotoXY(x, tail);
            cout << " ";
        }

        int step = speed[x] + g_speedBoost;
        if (step < 1) step = 1;
        headY[x] += step;
        if (headY[x] > height + rand() % 20) {
            headY[x] = -(rand() % height);
            speed[x] = 1 + rand() % 3;
        }
    }

    setColor(7);
}

// ------------------------------------------------------------
// SCHERMATA ACHIEVEMENTS
// ------------------------------------------------------------
void exportAchievementsTxt() {
    ofstream out("achievements_export.txt");
    if (!out) return;

    int total = achievements.size();
    int unlockedCount = 0;
    for (auto& a : achievements) if (a.unlocked) unlockedCount++;

    out << "VALENTINO GAME COLLECTION - ACHIEVEMENTS\n\n";
    out << "Profilo: " << g_playerName << "\n";
    out << "Progress: " << unlockedCount << " / " << total << " ("
        << (total ? (unlockedCount * 100 / total) : 0) << "%)\n\n";

    for (auto& a : achievements) {
        out << (a.unlocked ? "[UNLOCKED] " : "[LOCKED]   ");
        out << a.name << " - " << a.description << "\n";
    }

    out.close();
}

void showAchievements() {
    system("cls");
    setColor(10);
    cout << "=== ACHIEVEMENTS ===\n\n";
    setColor(7);

    int total = achievements.size();
    int unlockedCount = 0;
    for (auto& a : achievements) if (a.unlocked) unlockedCount++;

    cout << "Profilo: " << g_playerName << "\n";
    cout << "Progress: " << unlockedCount << " / " << total << " ("
        << (total ? (unlockedCount * 100 / total) : 0) << "%)\n\n";

    for (int d = 0; d <= 3; d++) {
        if (d == 0) { setColor(10); cout << "[FACILI]\n"; }
        if (d == 1) { setColor(14); cout << "\n[MEDI]\n"; }
        if (d == 2) { setColor(12); cout << "\n[DIFFICILI]\n"; }
        if (d == 3) { setColor(13); cout << "\n[SEGRETI]\n"; }
        setColor(7);

        for (auto& a : achievements) {
            if (a.difficulty != d) continue;

            if (a.unlocked) {
                setColor(14);
                cout << "[✓] " << a.name << " - " << a.description << "\n";
            }
            else {
                setColor(8);
                cout << "[ ] " << a.name << "\n";
            }
            setColor(7);
        }
    }

    cout << "\nPremi E per esportare in achievements_export.txt, oppure INVIO per tornare al menu.\n";
    while (true) {
        if (GetAsyncKeyState('E') & 0x8000) {
            exportAchievementsTxt();
            cout << "\nEsportazione completata.\n";
            Sleep(500);
            break;
        }
        if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
            break;
        }
        Sleep(50);
    }

    system("pause");
}

// ------------------------------------------------------------
// ANIMAZIONE AVVIO GIOCO
// ------------------------------------------------------------
void gameBootScreen(const string& title) {
    system("cls");
    setColor(11);
    cout << "[ " << title << " SYSTEM BOOTING ]\n";
    setColor(7);
    Sleep(200);
    slowPrint("Initializing modules...\n", 20);
    Sleep(150);
    slowPrint("Loading core logic...\n", 20);
    Sleep(150);
    slowPrint("Calibrating difficulty...\n", 20);
    Sleep(150);
    slowPrint("Ready.\n", 20);
    Beep(900, 120);
    Sleep(300);
}

// ------------------------------------------------------------
// MENU GIOCHI E DIFFICOLTA'
// ------------------------------------------------------------
int scegliDifficolta() {
    int selected = 0;
    const int numOptions = 4;
    string options[numOptions] = {
        "Facile",
        "Medio",
        "Difficile",
        "Torna indietro"
    };

    hideCursor();

    while (true) {
        system("cls");
        setColor(10);
        cout << "=== SCEGLI LA DIFFICOLTA' ===\n\n";
        setColor(7);
        cout << "Usa W/S o Frecce per muoverti, INVIO per confermare.\n\n";

        for (int i = 0; i < numOptions; i++) {
            if (i == selected) {
                setColor(10);
                cout << "> " << options[i] << "\n";
            }
            else {
                setColor(7);
                cout << "  " << options[i] << "\n";
            }
        }

        SHORT up = GetAsyncKeyState(VK_UP);
        SHORT down = GetAsyncKeyState(VK_DOWN);
        SHORT wKey = GetAsyncKeyState('W');
        SHORT sKey = GetAsyncKeyState('S');
        SHORT enterKey = GetAsyncKeyState(VK_RETURN);

        if ((up & 0x8000) || (wKey & 0x8000)) {
            selected--;
            if (selected < 0) selected = numOptions - 1;
            Beep(700, 60);
            Sleep(150);
        }
        else if ((down & 0x8000) || (sKey & 0x8000)) {
            selected++;
            if (selected >= numOptions) selected = 0;
            Beep(500, 60);
            Sleep(150);
        }
        else if (enterKey & 0x8000) {
            if (selected == 3) return 0; // torna indietro
            return selected + 1; // 1..3
        }

        Sleep(80);
    }
}

// ------------------------------------------------------------
// MENU GIOCHI (HUB)
// ------------------------------------------------------------
int giochiMenu() {
    int selected = 0;
    const int numOptions = 4;
    string options[numOptions] = {
        "Indovina il Numero",
        "Snake",
        "Dodge Game",
        "Torna al menu"
    };

    hideCursor();

    while (true) {
        system("cls");
        setColor(10);
        cout << "==============================\n";
        cout << "         GIOCHI DISPONIBILI   \n";
        cout << "==============================\n\n";
        setColor(7);

        // Indovina il Numero
        if (selected == 0) setColor(10); else setColor(7);
        cout << (selected == 0 ? "> " : "  ") << "Indovina il Numero\n";
        setColor(8);
        cout << "    Miglior tempo: " << (g_migliorTempo == 9999 ? 0 : g_migliorTempo) << " s\n";
        cout << "    Migliori tentativi: " << (g_migliorTentativi == 9999 ? 0 : g_migliorTentativi) << "\n\n";

        // Snake
        if (selected == 1) setColor(10); else setColor(7);
        cout << (selected == 1 ? "> " : "  ") << "Snake\n";
        setColor(8);
        cout << "    Record: " << g_snakeBestScore << " punti\n";
        cout << "    Difficolta max: " << difficultyName(g_snakeBestDifficulty) << "\n\n";

        // Dodge Game
        if (selected == 2) setColor(10); else setColor(7);
        cout << (selected == 2 ? "> " : "  ") << "Dodge Game\n";
        setColor(8);
        cout << "    Record: " << g_dodgeBestTime << " secondi\n";
        cout << "    Difficolta max: " << difficultyName(g_dodgeBestDifficulty) << "\n\n";

        // Torna
        if (selected == 3) setColor(10); else setColor(7);
        cout << (selected == 3 ? "> " : "  ") << "Torna al menu\n";

        setColor(7);
        cout << "\nUsa W/S o Frecce per muoverti, INVIO per scegliere.\n";

        SHORT up = GetAsyncKeyState(VK_UP);
        SHORT down = GetAsyncKeyState(VK_DOWN);
        SHORT wKey = GetAsyncKeyState('W');
        SHORT sKey = GetAsyncKeyState('S');
        SHORT enterKey = GetAsyncKeyState(VK_RETURN);

        if ((up & 0x8000) || (wKey & 0x8000)) {
            selected--;
            if (selected < 0) selected = numOptions - 1;
            Beep(700, 60);
            Sleep(150);
        }
        else if ((down & 0x8000) || (sKey & 0x8000)) {
            selected++;
            if (selected >= numOptions) selected = 0;
            Beep(500, 60);
            Sleep(150);
        }
        else if (enterKey & 0x8000) {
            if (selected == 3) return 0; // torna al menu
            return selected + 1; // 1..3
        }

        Sleep(80);
    }
}

// ------------------------------------------------------------
// MENU PRINCIPALE
// ------------------------------------------------------------
int matrixMenu(const string& nomeUtente,
    int migliorTempo,
    int migliorTentativi,
    bool easterNeo,
    bool easterTrinity,
    bool easterMorpheus,
    bool easterValentino,
    bool easterArchitect) {

    const int numOptions = 5;
    string options[numOptions] = {
        "Saluta",
        "Mostra un numero",
        "Giochi",
        "Achievements",
        "Esci"
    };

    int headY[WIDTH];
    int speed[WIDTH];

    for (int i = 0; i < WIDTH; i++) {
        headY[i] = rand() % HEIGHT;
        speed[i] = 1 + rand() % 3;
    }

    hideCursor();

    int selected = 0;

    string secretInput = "";
    string codeInput = "";
    int catCounter = 0;

    while (true) {

        matrixBackgroundFrame(WIDTH, HEIGHT, headY, speed);

        bool menuGlitch = (rand() % 50 == 0);

        int panelX = 18;
        int panelY = 3;

        setColor(0);
        for (int y = 0; y < 16; y++) {
            gotoXY(panelX, panelY + y);
            cout << "                              ";
        }

        if (easterValentino) {
            gotoXY(panelX, panelY - 2);
            setColor(14);
            cout << "[DEV GOD MODE ENABLED]        ";
        }

        if (easterNeo) {
            gotoXY(panelX, panelY - 1);
            setColor(15);
            cout << "      THE ONE HAS LOGGED IN      ";
        }

        if (easterTrinity) {
            gotoXY(panelX, panelY - 1);
            setColor(11);
            cout << "Connection established, Trinity. ";
        }

        if (easterArchitect) {
            gotoXY(panelX - 8, panelY - 1);
            setColor(15);
            cout << "Welcome, Architect.                         ";
        }

        setColor(menuGlitch ? 15 : 10);
        gotoXY(panelX, panelY);
        cout << "==============================";
        gotoXY(panelX, panelY + 1);
        cout << "      VALENTINO CONSOLE      ";
        gotoXY(panelX, panelY + 2);
        cout << "==============================";

        setColor(10);
        gotoXY(panelX, panelY + 4);
        cout << "Utente: " << nomeUtente << "          ";
        setColor(7);
        gotoXY(panelX, panelY + 5);
        cout << "Record tempo: " << migliorTempo << " s   ";
        gotoXY(panelX, panelY + 6);
        cout << "Record tentativi: " << migliorTentativi << "   ";

        setColor(2);
        gotoXY(panelX, panelY + 8);
        cout << "W/S o Frecce, INVIO per scegliere   ";
        setColor(7);

        for (int i = 0; i < numOptions; i++) {
            gotoXY(panelX + 2, panelY + 10 + i);
            if (i == selected) {
                setColor(menuGlitch ? 15 : 10);
                cout << "> [" << options[i] << "]           ";
            }
            else {
                setColor(7);
                cout << "  " << options[i] << "              ";
            }
        }

        if (easterMorpheus && rand() % 120 == 0) {
            gotoXY(2, HEIGHT - 2);
            setColor(13);
            string phrases[3] = {
                "What is real?",
                "Free your mind.",
                "I can only show you the door."
            };
            cout << phrases[rand() % 3] << "                ";
        }

        if (rand() % 200 == 0) {
            gotoXY(10, 12);
            setColor(15);
            cout << "SYSTEM BREACH DETECTED      ";
        }

        if (GetAsyncKeyState('C') & 0x8000) {
            catCounter++;
            Sleep(150);
        }
        if (catCounter >= 2) {
            gotoXY(5, HEIGHT - 3);
            setColor(14);
            cout << "=^.^=  (déjà-vu?)           ";
            catCounter = 0;
            unlockAchievement("Déjà-vu");
        }

        if (GetAsyncKeyState('R') & 0x8000) {
            g_speedBoost = 2;
            gotoXY(0, 0);
            setColor(12);
            cout << "RED PILL: MATRIX FULL POWER        ";
            unlockAchievement("Red Awakening");
        }
        if (GetAsyncKeyState('B') & 0x8000) {
            g_speedBoost = 0;
            gotoXY(0, 0);
            setColor(9);
            cout << "BLUE PILL: NORMAL MODE             ";
            unlockAchievement("Blue Dream");
        }
        if (GetAsyncKeyState('G') & 0x8000) {
            gotoXY(0, 1);
            setColor(15);
            cout << "!!! SYSTEM GLITCH BOOST !!!        ";
        }

        if (GetAsyncKeyState('O') & 0x8000) { secretInput += 'o'; Sleep(120); }
        if (GetAsyncKeyState('P') & 0x8000) { secretInput += 'p'; Sleep(120); }
        if (GetAsyncKeyState('E') & 0x8000) { secretInput += 'e'; Sleep(120); }
        if (GetAsyncKeyState('N') & 0x8000) { secretInput += 'n'; Sleep(120); }
        if (GetAsyncKeyState('S') & 0x8000) { secretInput += 's'; Sleep(120); }
        if (GetAsyncKeyState('A') & 0x8000) { secretInput += 'a'; Sleep(120); }
        if (GetAsyncKeyState('M') & 0x8000) { secretInput += 'm'; Sleep(120); }

        if (secretInput.size() > 40)
            secretInput.erase(0, secretInput.size() - 40);

        if (secretInput.find("opensesame") != string::npos) {
            unlockAchievement("Secret Door");
            playTransition(1, nomeUtente);
            system("cls");
            setColor(10);
            cout << "ACCESSO ALLA STANZA SEGRETA...\n\n";
            Sleep(1000);
            cout << "Qui potresti mettere un mini-gioco segreto,\n";
            cout << "un messaggio cifrato o un codice da decifrare.\n\n";
            system("pause");
            secretInput.clear();
        }

        if (GetAsyncKeyState('4') & 0x8000) { codeInput += '4'; Sleep(120); }
        if (GetAsyncKeyState('0') & 0x8000) { codeInput += '0'; Sleep(120); }

        if (codeInput.size() > 10)
            codeInput.erase(0, codeInput.size() - 10);

        if (codeInput == "404") {
            unlockAchievement("System Error");
            playTransition(1, nomeUtente);
            system("cls");
            setColor(12);
            cout << "ERROR HANDLER MODE\n\n";
            cout << "Correggi gli errori prima che esplodano! (placeholder)\n\n";
            system("pause");
            codeInput.clear();
        }

        SHORT up = GetAsyncKeyState(VK_UP);
        SHORT down = GetAsyncKeyState(VK_DOWN);
        SHORT wKey = GetAsyncKeyState('W');
        SHORT sKey = GetAsyncKeyState('S');
        SHORT enterKey = GetAsyncKeyState(VK_RETURN);

        if ((up & 0x8000) || (wKey & 0x8000)) {
            selected--;
            if (selected < 0) selected = numOptions - 1;
            Beep(700, 60);
            Sleep(150);
        }
        else if ((down & 0x8000) || (sKey & 0x8000)) {
            selected++;
            if (selected >= numOptions) selected = 0;
            Beep(500, 60);
            Sleep(150);
        }
        else if (enterKey & 0x8000) {
            playTransition(4, nomeUtente);
            system("cls");
            return selected + 1;
        }

        Sleep(80);
    }

    return 3;
}

// ------------------------------------------------------------
// MINI-GIOCO: INDOVINA IL NUMERO (con difficolta esterna)
// ------------------------------------------------------------
void miniGiocoIndovina(const string& nomeUtente, int difficolta) {

    playTransition(0, nomeUtente);

    system("cls");
    cout << "\nCaricamento";
    for (int i = 0; i < 3; i++) {
        cout << ".";
        Beep(600, 120);
        Sleep(300);
    }
    cout << "\n\n";

    setColor(11);
    slowPrint("=== INDOVINA IL NUMERO ===\n", 25);
    setColor(7);

    int maxRange;

    switch (difficolta) {
    case 1: maxRange = 20; break;
    case 2: maxRange = 50; break;
    case 3: maxRange = 100; break;
    default:
        setColor(12);
        cout << "Difficolta non valida, imposto difficolta media.\n";
        Beep(400, 300);
        setColor(7);
        maxRange = 50;
        difficolta = 2;
        break;
    }

    int numeroSegreto = rand() % maxRange + 1;
    int tentativo = 0;
    int tentativi = 0;

    time_t startTime = time(nullptr);

    cout << "\nHo scelto un numero tra 1 e " << maxRange << ". Prova a indovinarlo!\n";

    do {
        cout << "Il tuo tentativo: ";
        if (!(cin >> tentativo)) {
            cout << "Input non valido. Inserisci un numero.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        tentativi++;
        if (tentativo < 1 || tentativo > maxRange) {

            cout << "Il numero deve essere tra 1 e " << maxRange << ".\n";
            continue;
        }

        if (tentativo > numeroSegreto) {
            setColor(12);
            cout << "Troppo alto!\n";
            Beep(500, 150);
            setColor(7);
        }
        else if (tentativo < numeroSegreto) {
            setColor(14);
            cout << "Troppo basso!\n";
            Beep(500, 150);
            setColor(7);
        }
        else {
            time_t endTime = time(nullptr);
            int durata = (int)(endTime - startTime);

            setColor(10);
            blinkText("\nHAI INDOVINATO!", 5, 120);

            slowPrint("Bravo " + nomeUtente + "! ", 40);
            slowPrint("Hai indovinato in ", 30);
            cout << tentativi << " tentativi e " << durata << " secondi!\n";

            Beep(900, 150);
            Beep(1100, 150);
            Beep(1300, 200);

            bool nuovoRecord = false;

            if (durata < g_migliorTempo) {
                g_migliorTempo = durata;
                setColor(14);
                slowPrint("\nNuovo record di tempo! ", 40);
                cout << "(" << durata << " secondi)\n";
                nuovoRecord = true;
            }

            if (tentativi < g_migliorTentativi) {
                g_migliorTentativi = tentativi;
                setColor(14);
                slowPrint("Nuovo record di tentativi! ", 40);
                cout << "(" << tentativi << " tentativi)\n";
                nuovoRecord = true;
            }

            if (!nuovoRecord) {
                setColor(11);
                slowPrint("\nNon hai battuto nessun record stavolta... ma ci sei andato vicino!\n", 30);
            }

            if (durata < 5) unlockAchievement("Speedrunner");
            if (tentativi == 1) unlockAchievement("Perfect Guess");
            if (difficolta == 3) unlockAchievement("Hardcore Mode");

            g_vittorieConsecutive++;
            if (g_vittorieConsecutive >= 3) unlockAchievement("Persistence");

            setColor(7);
        }

    } while (tentativo != numeroSegreto);

    system("pause");

    playTransition(0, nomeUtente);
    system("cls");
}

// ------------------------------------------------------------
// SNAKE
// ------------------------------------------------------------
struct SnakeSegment {
    int x, y;
};

void giocoSnake(const string& nomeUtente, int difficolta) {
    gameBootScreen("SNAKE");

    const int width = 30;
    const int height = 20;

    int delayMs;
    if (difficolta == 1) delayMs = 150;
    else if (difficolta == 2) delayMs = 100;
    else delayMs = 60;

    vector<SnakeSegment> snake;
    snake.push_back({ width / 2, height / 2 });

    int dirX = 1, dirY = 0;

    int foodX = rand() % width;
    int foodY = rand() % height;

    int score = 0;
    bool gameOver = false;

    hideCursor();

    while (!gameOver) {
        // input
        if (GetAsyncKeyState(VK_UP) & 0x8000 || GetAsyncKeyState('W') & 0x8000) {
            if (dirY != 1) { dirX = 0; dirY = -1; }
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x8000 || GetAsyncKeyState('S') & 0x8000) {
            if (dirY != -1) { dirX = 0; dirY = 1; }
        }
        if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000) {
            if (dirX != 1) { dirX = -1; dirY = 0; }
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
            if (dirX != -1) { dirX = 1; dirY = 0; }
        }

        // move
        SnakeSegment head = snake.front();
        SnakeSegment newHead = { head.x + dirX, head.y + dirY };

        // collision with walls
        if (newHead.x < 0 || newHead.x >= width || newHead.y < 0 || newHead.y >= height) {
            gameOver = true;
        }

        // collision with self
        for (auto& seg : snake) {
            if (seg.x == newHead.x && seg.y == newHead.y) {
                gameOver = true;
                break;
            }
        }

        if (gameOver) break;

        snake.insert(snake.begin(), newHead);

        // food
        if (newHead.x == foodX && newHead.y == foodY) {
            score++;
            Beep(900, 80);
            // new food
            bool valid = false;
            while (!valid) {
                foodX = rand() % width;
                foodY = rand() % height;
                valid = true;
                for (auto& seg : snake) {
                    if (seg.x == foodX && seg.y == foodY) {
                        valid = false;
                        break;
                    }
                }
            }
        }
        else {
            snake.pop_back();
        }

        // draw
        system("cls");
        setColor(10);
        cout << "=== SNAKE ===   Giocatore: " << nomeUtente << "   Punteggio: " << score << "\n";
        setColor(7);

        for (int y = -1; y <= height; y++) {
            for (int x = -1; x <= width; x++) {
                if (y == -1 || y == height || x == -1 || x == width) {
                    cout << "#";
                }
                else {
                    bool printed = false;
                    if (x == foodX && y == foodY) {
                        setColor(12);
                        cout << "*";
                        setColor(7);
                        printed = true;
                    }
                    else {
                        for (size_t i = 0; i < snake.size(); i++) {
                            if (snake[i].x == x && snake[i].y == y) {
                                if (i == 0) {
                                    setColor(10);
                                    cout << "O";
                                }
                                else {
                                    setColor(2);
                                    cout << "o";
                                }
                                setColor(7);
                                printed = true;
                                break;
                            }
                        }
                    }
                    if (!printed) cout << " ";
                }
            }
            cout << "\n";
        }

        Sleep(delayMs);
    }

    system("cls");
    setColor(12);
    cout << "GAME OVER!\n\n";
    setColor(7);
    cout << "Punteggio finale: " << score << "\n";

    bool nuovoRecord = false;
    if (score > g_snakeBestScore) {
        g_snakeBestScore = score;
        g_snakeBestDifficulty = difficolta;
        nuovoRecord = true;
        setColor(14);
        cout << "Nuovo record personale in Snake!\n";
        setColor(7);
    }

    if (score >= 10) unlockAchievement("Snake Rookie");
    if (score >= 30) unlockAchievement("Snake Survivor");
    if (score >= 50) unlockAchievement("Snake Master");

    if (!nuovoRecord) {
        cout << "Record attuale: " << g_snakeBestScore
            << " (difficolta: " << difficultyName(g_snakeBestDifficulty) << ")\n";
    }

    cout << "\n";
    system("pause");
}

// ------------------------------------------------------------
// DODGE GAME
// ------------------------------------------------------------
struct Obstacle {
    int x, y;
};

void giocoDodge(const string& nomeUtente, int difficolta) {
    gameBootScreen("DODGE GAME");

    const int width = 30;
    const int height = 20;

    int delayMs;
    if (difficolta == 1) delayMs = 120;
    else if (difficolta == 2) delayMs = 80;
    else delayMs = 50;

    int playerX = width / 2;
    int playerY = height - 2;

    vector<Obstacle> obstacles;
    bool gameOver = false;

    DWORD startTime = GetTickCount();
    int survivedSeconds = 0;

    hideCursor();

    while (!gameOver) {
        // input
        if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000) {
            playerX--;
            if (playerX < 0) playerX = 0;
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
            playerX++;
            if (playerX >= width) playerX = width - 1;
        }

        // spawn ostacoli
        int spawnChance;
        if (difficolta == 1) spawnChance = 12;
        else if (difficolta == 2) spawnChance = 8;
        else spawnChance = 5;

        if (rand() % spawnChance == 0) {
            Obstacle o;
            o.x = rand() % width;
            o.y = 0;
            obstacles.push_back(o);
        }

        // move ostacoli
        for (auto& o : obstacles) {
            o.y++;
        }

        // collisioni e pulizia
        vector<Obstacle> newObs;
        for (auto& o : obstacles) {
            if (o.y == playerY && o.x == playerX) {
                gameOver = true;
            }
            if (o.y < height) {
                newObs.push_back(o);
            }
        }
        obstacles.swap(newObs);

        // tempo
        DWORD now = GetTickCount();
        survivedSeconds = (int)((now - startTime) / 1000);

        // draw
        system("cls");
        setColor(10);
        cout << "=== DODGE GAME ===   Giocatore: " << nomeUtente
            << "   Tempo: " << survivedSeconds << " s\n";
        setColor(7);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool printed = false;

                if (y == playerY && x == playerX) {
                    setColor(14);
                    cout << "▲";
                    setColor(7);
                    printed = true;
                }
                else {
                    for (auto& o : obstacles) {
                        if (o.x == x && o.y == y) {
                            setColor(12);
                            cout << "█";
                            setColor(7);
                            printed = true;
                            break;
                        }
                    }
                }

                if (!printed) cout << " ";
            }
            cout << "\n";
        }

        Sleep(delayMs);
    }

    system("cls");
    setColor(12);
    cout << "SEI STATO COLPITO!\n\n";
    setColor(7);
    cout << "Tempo sopravvissuto: " << survivedSeconds << " secondi\n";

    bool nuovoRecord = false;
    if (survivedSeconds > g_dodgeBestTime) {
        g_dodgeBestTime = survivedSeconds;
        g_dodgeBestDifficulty = difficolta;
        nuovoRecord = true;
        setColor(14);
        cout << "Nuovo record personale in Dodge Game!\n";
        setColor(7);
    }

    if (survivedSeconds >= 20) unlockAchievement("Dodger");
    if (survivedSeconds >= 40) unlockAchievement("Ultra Dodger");
    if (survivedSeconds >= 60) unlockAchievement("Untouchable");

    if (!nuovoRecord) {
        cout << "Record attuale: " << g_dodgeBestTime
            << " s (difficolta: " << difficultyName(g_dodgeBestDifficulty) << ")\n";
    }

    cout << "\n";
    system("pause");
}

// ------------------------------------------------------------
// SPLASH SCREEN
// ------------------------------------------------------------
void splashScreen() {
    setColor(11);
    slowPrint("\n=====================================\n", 5);
    Sleep(150);

    setColor(10);
    slowPrint("      VALENTINO GAME COLLECTION\n", 40);

    setColor(11);
    slowPrint("=====================================\n\n", 5);
    setColor(7);

    slowPrint("Caricamento", 40);
    for (int i = 0; i < 5; i++) {
        cout << ".";
        Beep(600, 120);
        Sleep(300);
    }
    cout << "\n\n";
}

// ------------------------------------------------------------
// MENU PROFILI
// ------------------------------------------------------------
bool profileMenu() {
    while (true) {
        system("cls");
        cout << "=====================================\n";
        cout << "           PROFILI DI GIOCO          \n";
        cout << "=====================================\n\n";

        cout << "1) Nuovo profilo\n";
        cout << "2) Seleziona profilo\n";
        cout << "3) Rinomina profilo\n";
        cout << "4) Elimina profilo\n";
        cout << "5) Esci\n\n";

        auto profiles = listProfiles();
        cout << "Profili disponibili:\n";
        if (profiles.empty()) {
            cout << "- Nessun profilo trovato.\n\n";
        }
        else {
            for (size_t i = 0; i < profiles.size(); ++i) {
                cout << "  " << (i + 1) << ") " << profiles[i] << "\n";
            }
            cout << "\n";
        }

        int scelta = promptInt("Scelta: ");

        if (scelta == 1) {
            cout << "\nInserisci il nome del nuovo profilo: ";
            string name;
            cin >> name;

            initAchievements();
            g_playerName = name;
            g_migliorTempo = 9999;
            g_migliorTentativi = 9999;
            g_vittorieConsecutive = 0;
            g_musicOn = true;
            g_snakeBestScore = 0;
            g_snakeBestDifficulty = 0;
            g_dodgeBestTime = 0;
            g_dodgeBestDifficulty = 0;

            saveCurrentProfile();
            return true;
        }
        else if (scelta == 2) {
            if (profiles.empty()) {
                cout << "\nNessun profilo da selezionare.\n";
                system("pause");
                continue;
            }
            int idx = promptInt("Seleziona il numero del profilo: ");
            if (idx < 1 || idx >(int)profiles.size()) {
                cout << "Scelta non valida.\n";
                system("pause");
                continue;
            }
            initAchievements();
            if (!loadProfile(profiles[idx - 1])) {
                cout << "Errore nel caricamento del profilo.\n";
                system("pause");
                continue;
            }
            return true;
        }
        else if (scelta == 3) {
            if (profiles.empty()) {
                cout << "\nNessun profilo da rinominare.\n";
                system("pause");
                continue;
            }
            int idx = promptInt("Seleziona il numero del profilo da rinominare: ");
            if (idx < 1 || idx >(int)profiles.size()) {
                cout << "Scelta non valida.\n";
                system("pause");
                continue;
            }
            string oldName = profiles[idx - 1];
            cout << "Nuovo nome: ";
            string newName;
            cin >> newName;
            std::error_code ec;
            fs::rename(profilePath(oldName), profilePath(newName), ec);
            if (ec) cout << "Errore rinominando: " << ec.message() << "\n";
            else cout << "Profilo rinominato.\n";
            system("pause");
        }
        else if (scelta == 4) {
            if (profiles.empty()) {
                cout << "\nNessun profilo da eliminare.\n";
                system("pause");
                continue;
            }
            int idx = promptInt("Seleziona il numero del profilo da eliminare: ");
            if (idx < 1 || idx >(int)profiles.size()) {
                cout << "Scelta non valida.\n";
                system("pause");
                continue;
            }
            string name = profiles[idx - 1];
            cout << "Sei sicuro di voler eliminare il profilo '" << name << "'? (1 = Si, 2 = No): ";
            int conf = promptInt("");
            if (conf == 1) {
                std::error_code ec;
                fs::remove(profilePath(name), ec);
                if (ec) cout << "Errore eliminando: " << ec.message() << "\n";
                else cout << "Profilo eliminato.\n";
            }
            else {
                cout << "Operazione annullata.\n";
            }
            system("pause");
        }
        else if (scelta == 5) {
            return false;
        }
        else {
            cout << "Scelta non valida.\n";
            system("pause");
        }
    }
}

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
int main() {
    srand((unsigned)time(0));

    ensureProfilesDir();
    initAchievements();

    if (!profileMenu()) {
        return 0;
    }

    // Easter egg sui nomi dopo scelta profilo
    bool easterNeo = (g_playerName == "Neo");
    bool easterTrinity = (g_playerName == "Trinity");
    bool easterMorpheus = (g_playerName == "Morpheus");
    bool easterValentino = (g_playerName == "Valentino");
    bool easterArchitect = (g_playerName == "Architect");

    if (easterNeo) unlockAchievement("THE ONE");
    if (easterTrinity) unlockAchievement("Connection Established");
    if (easterMorpheus) unlockAchievement("Free Your Mind");
    if (easterValentino) unlockAchievement("Dev God Mode");
    if (easterArchitect) unlockAchievement("The Architect");

    startBackgroundMusic();

    splashScreen();
    playTransition(0, g_playerName);
    matrixRainPro(2500);

    int scelta = 0;

    do {
        playTransition(4, g_playerName);
        scelta = matrixMenu(g_playerName,
            g_migliorTempo,
            g_migliorTentativi,
            easterNeo,
            easterTrinity,
            easterMorpheus,
            easterValentino,
            easterArchitect);

        switch (scelta) {

        case 1: // Saluta
            playTransition(0, g_playerName);
            setColor(10);
            slowPrint("\nCiao " + g_playerName + "! Grande che stai imparando il C++!\n", 30);
            setColor(7);
            system("pause");
            break;

        case 2: // Mostra un numero
            playTransition(0, g_playerName);
            setColor(14);
            slowPrint("\nIl numero magico di oggi è: 42\n", 30);
            setColor(7);
            system("pause");
            break;

        case 3: { // Giochi
            int sceltaGioco = giochiMenu();
            if (sceltaGioco == 0) {
                // torna al menu
                break;
            }

            int diff = scegliDifficolta();
            if (diff == 0) {
                // torna indietro
                break;
            }

            if (sceltaGioco == 1) {
                miniGiocoIndovina(g_playerName, diff);
            }
            else if (sceltaGioco == 2) {
                giocoSnake(g_playerName, diff);
            }
            else if (sceltaGioco == 3) {
                giocoDodge(g_playerName, diff);
            }
            break;
        }

        case 4: // Achievements
            showAchievements();
            break;

        case 5: // Esci
            playTransition(2, g_playerName);
            setColor(12);
            slowPrint("\nUscita dal programma...\n", 30);
            setColor(7);
            break;

        default:
            playTransition(1, g_playerName);
            setColor(12);
            cout << "\nScelta non valida. Riprova.\n";
            Beep(400, 250);
            setColor(7);
            system("pause");
            break;
        }

        saveCurrentProfile();

    } while (scelta != 5);

    stopBackgroundMusic();
    return 0;
}
