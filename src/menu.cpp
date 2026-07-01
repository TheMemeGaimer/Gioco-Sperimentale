// ============================================================
// VALENTINO GAME COLLECTION - Cross-Platform Edition
// Funziona su Windows, Linux e macOS
// Backend online: profili con email e password
// ============================================================
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <limits>
#include <cstring>
#include <chrono>
#include <thread>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
  #define CLEAR_SCREEN() system("cls")
  #define PLATFORM_SLEEP(ms) Sleep(ms)
  #define KEY_UP_CODE    72
  #define KEY_DOWN_CODE  80
  #define KEY_LEFT_CODE  75
  #define KEY_RIGHT_CODE 77
#else
  #include <termios.h>
  #include <unistd.h>
  #include <sys/select.h>
  #define CLEAR_SCREEN() printf("\033[2J\033[H"); fflush(stdout)
  #define PLATFORM_SLEEP(ms) std::this_thread::sleep_for(std::chrono::milliseconds(ms))
  #define KEY_UP_CODE    1001
  #define KEY_DOWN_CODE  1002
  #define KEY_LEFT_CODE  1003
  #define KEY_RIGHT_CODE 1004
#endif

using namespace std;
namespace fs = std::filesystem;

// ============================================================
// TERMINALE - Gestione cross-platform
// ============================================================
#ifndef _WIN32
static struct termios g_origTermios;
static bool g_rawMode = false;

void enableRawMode() {
    if (g_rawMode) return;
    tcgetattr(STDIN_FILENO, &g_origTermios);
    struct termios raw = g_origTermios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    g_rawMode = true;
}

void disableRawMode() {
    if (!g_rawMode) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_origTermios);
    g_rawMode = false;
}

int platformReadKey() {
    struct timeval tv = {0, 15000};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, nullptr, nullptr, &tv) <= 0) return 0;

    unsigned char buf[4] = {0};
    int n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return 0;

    if (n >= 3 && buf[0] == 27 && buf[1] == '[') {
        switch (buf[2]) {
            case 'A': return KEY_UP_CODE;
            case 'B': return KEY_DOWN_CODE;
            case 'C': return KEY_RIGHT_CODE;
            case 'D': return KEY_LEFT_CODE;
        }
    }
    if (n == 1) {
        if (buf[0] == 27) {
            struct timeval tv2 = {0, 50000};
            fd_set fds2;
            FD_ZERO(&fds2);
            FD_SET(STDIN_FILENO, &fds2);
            if (select(1, &fds2, nullptr, nullptr, &tv2) > 0) {
                unsigned char seq[3] = {0};
                int m = read(STDIN_FILENO, seq, sizeof(seq));
                if (m >= 2 && seq[0] == '[') {
                    switch(seq[1]) {
                        case 'A': return KEY_UP_CODE;
                        case 'B': return KEY_DOWN_CODE;
                        case 'C': return KEY_RIGHT_CODE;
                        case 'D': return KEY_LEFT_CODE;
                    }
                }
            }
            return 27;
        }
        return buf[0];
    }
    return buf[0];
}

bool platformKbhit() {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(1, &fds, nullptr, nullptr, &tv) > 0;
}
#else
int platformReadKey() {
    if (!_kbhit()) return 0;
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        switch (c2) {
            case 72: return KEY_UP_CODE;
            case 80: return KEY_DOWN_CODE;
            case 75: return KEY_LEFT_CODE;
            case 77: return KEY_RIGHT_CODE;
        }
        return 0;
    }
    return c;
}
bool platformKbhit() { return _kbhit() != 0; }
void enableRawMode() {}
void disableRawMode() {}
#endif

void hideCursor() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    if (GetConsoleCursorInfo(out, &ci)) { ci.bVisible = FALSE; SetConsoleCursorInfo(out, &ci); }
#else
    printf("\033[?25l"); fflush(stdout);
#endif
}

void showCursor() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    if (GetConsoleCursorInfo(out, &ci)) { ci.bVisible = TRUE; SetConsoleCursorInfo(out, &ci); }
#else
    printf("\033[?25h"); fflush(stdout);
#endif
}

void setColor(int color) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#else
    const char* codes[] = {
        "\033[0;30m", "\033[0;34m", "\033[0;32m", "\033[0;36m",
        "\033[0;31m", "\033[0;35m", "\033[0;33m", "\033[0;37m",
        "\033[0;90m", "\033[0;94m", "\033[0;92m", "\033[0;96m",
        "\033[0;91m", "\033[0;95m", "\033[0;93m", "\033[0;97m"
    };
    if (color >= 0 && color < 16) printf("%s", codes[color]);
#endif
}

void resetColor() {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
    printf("\033[0m");
#endif
}

void gotoXY(int x, int y) {
#ifdef _WIN32
    COORD c; c.X = x; c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
#else
    printf("\033[%d;%dH", y + 1, x + 1); fflush(stdout);
#endif
}

uint64_t getTickMs() {
    return (uint64_t)chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void waitKey(const string& msg = "Premi INVIO per continuare...") {
#ifndef _WIN32
    bool was = g_rawMode;
    if (was) disableRawMode();
#endif
    cout << msg << flush;
    string line;
    getline(cin, line);
#ifndef _WIN32
    if (was) enableRawMode();
#endif
    cout << "\n";
}

void slowPrint(const string& text, int delayMs = 30) {
    for (char c : text) {
        cout << c << flush;
        PLATFORM_SLEEP(delayMs);
    }
}

// ============================================================
// STATO GLOBALE
// ============================================================
const int WIDTH  = 80;
const int HEIGHT = 25;

string g_playerName;
string g_authToken;
bool   g_onlineMode = false;
string g_backendUrl = "http://localhost:8000";

int  g_migliorTempo        = 9999;
int  g_migliorTentativi    = 9999;
int  g_vittorieConsecutive = 0;
bool g_musicOn             = false;

int g_snakeBestScore       = 0;
int g_snakeBestDifficulty  = 0;
int g_dodgeBestTime        = 0;
int g_dodgeBestDifficulty  = 0;

// ============================================================
// ACHIEVEMENTS
// ============================================================
struct Achievement {
    string name;
    string description;
    bool   unlocked = false;
    int    difficulty;
};
vector<Achievement> achievements;

void addAchievement(const string& name, const string& desc, int diff) {
    achievements.push_back({name, desc, false, diff});
}

void initAchievements() {
    achievements.clear();
    addAchievement("THE ONE",             "Hai effettuato l'accesso come Neo.",           0);
    addAchievement("Connection Est.",     "Hai effettuato l'accesso come Trinity.",        0);
    addAchievement("Free Your Mind",      "Hai effettuato l'accesso come Morpheus.",       0);
    addAchievement("Dev God Mode",        "Il creatore e' entrato nel sistema.",           0);
    addAchievement("The Architect",       "Hai effettuato l'accesso come Architect.",      0);
    addAchievement("Red Awakening",       "Hai scelto la pillola rossa.",                  0);
    addAchievement("Blue Dream",          "Hai scelto la pillola blu.",                    0);
    addAchievement("First Login",         "Hai effettuato il primo accesso online.",       0);
    addAchievement("Deja-vu",            "Hai visto il gatto due volte.",                 1);
    addAchievement("Secret Door",         "Hai aperto la stanza segreta.",                 1);
    addAchievement("System Error",        "Hai trovato il codice 404 nascosto.",           1);
    addAchievement("Speedrunner",         "Hai vinto il mini-gioco in meno di 5 secondi.",2);
    addAchievement("Perfect Guess",       "Hai indovinato al primo tentativo.",            2);
    addAchievement("Hardcore Mode",       "Hai vinto in difficolta' massima.",             2);
    addAchievement("Persistence",         "Hai vinto tre volte di fila.",                  2);
    addAchievement("Snake Rookie",        "Hai raggiunto 10 punti in Snake.",              0);
    addAchievement("Snake Survivor",      "Hai raggiunto 30 punti in Snake.",              1);
    addAchievement("Snake Master",        "Hai raggiunto 50 punti in Snake.",              2);
    addAchievement("Dodger",              "Sopravvissuto 20 secondi in Dodge.",            0);
    addAchievement("Ultra Dodger",        "Sopravvissuto 40 secondi in Dodge.",            1);
    addAchievement("Untouchable",         "Sopravvissuto 60 secondi in Dodge.",            2);
    addAchievement("Cloud Saver",         "Hai salvato il profilo online.",                0);
}

void achievementPopup(const string& name) {
    disableRawMode();
    CLEAR_SCREEN();
    setColor(14);
    cout << "\n";
    cout << "  ╔══════════════════════════════════════╗\n";
    cout << "  ║      ACHIEVEMENT UNLOCKED!           ║\n";
    cout << "  ║                                      ║\n";
    setColor(10);
    // center the name
    int pad = max(0, (int)(36 - (int)name.size()) / 2);
    cout << "  ║" << string(pad, ' ') << name << string(36 - pad - (int)name.size(), ' ') << "║\n";
    setColor(14);
    cout << "  ║                                      ║\n";
    cout << "  ╚══════════════════════════════════════╝\n";
    resetColor();
    PLATFORM_SLEEP(1800);
    CLEAR_SCREEN();
}

void unlockAchievement(const string& name) {
    for (auto& a : achievements) {
        if (a.name == name && !a.unlocked) {
            a.unlocked = true;
            achievementPopup(a.name);
        }
    }
}

// ============================================================
// HTTP UTILS (tramite curl a riga di comando)
// ============================================================
string httpPost(const string& url, const string& json, const string& token = "") {
    string tmp = "/tmp/vgc_resp.json";
    string cmd = "curl -s -m 5 -X POST -H \"Content-Type: application/json\"";
    if (!token.empty())
        cmd += " -H \"Authorization: Bearer " + token + "\"";
    // escape single quotes in json
    string safe = json;
    cmd += " -d '" + safe + "'";
    cmd += " \"" + url + "\" > " + tmp + " 2>/dev/null";
    system(cmd.c_str());
    ifstream f(tmp);
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

string httpGet(const string& url, const string& token = "") {
    string tmp = "/tmp/vgc_resp.json";
    string cmd = "curl -s -m 5";
    if (!token.empty())
        cmd += " -H \"Authorization: Bearer " + token + "\"";
    cmd += " \"" + url + "\" > " + tmp + " 2>/dev/null";
    system(cmd.c_str());
    ifstream f(tmp);
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}

// Estrae valore stringa da JSON grezzo (senza libreria esterna)
string jsonStr(const string& json, const string& key) {
    string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    pos += search.size();
    size_t end = json.find('"', pos);
    if (end == string::npos) return "";
    return json.substr(pos, end - pos);
}

bool jsonBool(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return false;
    pos += search.size();
    return json.substr(pos, 4) == "true";
}

int jsonInt(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return 0;
    pos += search.size();
    size_t end = json.find_first_of(",}]", pos);
    if (end == string::npos) return 0;
    try { return stoi(json.substr(pos, end - pos)); }
    catch (...) { return 0; }
}

bool backendAvailable() {
    string resp = httpGet(g_backendUrl + "/api/ping");
    return resp.find("ok") != string::npos;
}

// ============================================================
// PROFILO - Salvataggio locale e online
// ============================================================
void ensureProfilesDir() {
    if (!fs::exists("profiles")) {
        error_code ec;
        fs::create_directory("profiles", ec);
    }
}

string profilePath(const string& name) {
    return "profiles/" + name + ".dat";
}

bool saveLocalProfile() {
    if (g_playerName.empty()) return false;
    ensureProfilesDir();
    ofstream out(profilePath(g_playerName), ios::binary);
    if (!out) return false;
    const char MAGIC[4] = {'V','G','C','2'};
    out.write(MAGIC, 4);
    uint32_t len = (uint32_t)g_playerName.size();
    out.write((char*)&len, 4);
    out.write(g_playerName.c_str(), len);
    out.write((char*)&g_migliorTempo,        sizeof(int));
    out.write((char*)&g_migliorTentativi,    sizeof(int));
    out.write((char*)&g_vittorieConsecutive, sizeof(int));
    out.write((char*)&g_musicOn,             sizeof(bool));
    uint32_t ac = (uint32_t)achievements.size();
    out.write((char*)&ac, 4);
    for (auto& a : achievements) out.write((char*)&a.unlocked, 1);
    out.write((char*)&g_snakeBestScore,      sizeof(int));
    out.write((char*)&g_snakeBestDifficulty, sizeof(int));
    out.write((char*)&g_dodgeBestTime,       sizeof(int));
    out.write((char*)&g_dodgeBestDifficulty, sizeof(int));
    out.close();
    return true;
}

bool loadLocalProfile(const string& name) {
    ifstream in(profilePath(name), ios::binary);
    if (!in) return false;
    char magic[4];
    if (!in.read(magic, 4)) return false;
    if (memcmp(magic, "VGC2", 4) != 0 && memcmp(magic, "VGC1", 4) != 0) return false;
    uint32_t len = 0;
    in.read((char*)&len, 4);
    g_playerName.resize(len);
    in.read(&g_playerName[0], len);
    in.read((char*)&g_migliorTempo,        sizeof(int));
    in.read((char*)&g_migliorTentativi,    sizeof(int));
    in.read((char*)&g_vittorieConsecutive, sizeof(int));
    in.read((char*)&g_musicOn,             sizeof(bool));
    uint32_t ac = 0;
    in.read((char*)&ac, 4);
    for (uint32_t i = 0; i < ac; i++) {
        bool st = false;
        in.read((char*)&st, 1);
        if (i < achievements.size()) achievements[i].unlocked = st;
    }
    in.read((char*)&g_snakeBestScore,      sizeof(int));
    in.read((char*)&g_snakeBestDifficulty, sizeof(int));
    in.read((char*)&g_dodgeBestTime,       sizeof(int));
    in.read((char*)&g_dodgeBestDifficulty, sizeof(int));
    return true;
}

vector<string> listLocalProfiles() {
    vector<string> names;
    if (!fs::exists("profiles")) return names;
    for (auto& e : fs::directory_iterator("profiles")) {
        if (e.is_regular_file() && e.path().extension() == ".dat")
            names.push_back(e.path().stem().string());
    }
    return names;
}

// Costruisce JSON del profilo corrente
string buildProfileJson() {
    string j = "{";
    j += "\"playerName\":\"" + g_playerName + "\",";
    j += "\"migliorTempo\":" + to_string(g_migliorTempo) + ",";
    j += "\"migliorTentativi\":" + to_string(g_migliorTentativi) + ",";
    j += "\"vittorieConsecutive\":" + to_string(g_vittorieConsecutive) + ",";
    j += "\"musicOn\":" + string(g_musicOn ? "true" : "false") + ",";
    j += "\"snakeBestScore\":" + to_string(g_snakeBestScore) + ",";
    j += "\"snakeBestDifficulty\":" + to_string(g_snakeBestDifficulty) + ",";
    j += "\"dodgeBestTime\":" + to_string(g_dodgeBestTime) + ",";
    j += "\"dodgeBestDifficulty\":" + to_string(g_dodgeBestDifficulty) + ",";
    j += "\"achievements\":[";
    for (size_t i = 0; i < achievements.size(); i++) {
        j += achievements[i].unlocked ? "true" : "false";
        if (i + 1 < achievements.size()) j += ",";
    }
    j += "]}";
    return j;
}

bool saveOnlineProfile() {
    if (g_authToken.empty()) return false;
    string json = buildProfileJson();
    string resp = httpPost(g_backendUrl + "/api/profile", json, g_authToken);
    return jsonBool(resp, "success");
}

bool loadOnlineProfile() {
    if (g_authToken.empty()) return false;
    string resp = httpGet(g_backendUrl + "/api/profile", g_authToken);
    if (!jsonBool(resp, "success")) return false;

    size_t pp = resp.find("\"profile\":");
    if (pp == string::npos) return false;
    string profilePart = resp.substr(pp + 10);

    int t = jsonInt(profilePart, "migliorTempo");
    if (t > 0) g_migliorTempo = t;
    int te = jsonInt(profilePart, "migliorTentativi");
    if (te > 0) g_migliorTentativi = te;
    g_vittorieConsecutive = jsonInt(profilePart, "vittorieConsecutive");
    g_snakeBestScore      = jsonInt(profilePart, "snakeBestScore");
    g_snakeBestDifficulty = jsonInt(profilePart, "snakeBestDifficulty");
    g_dodgeBestTime       = jsonInt(profilePart, "dodgeBestTime");
    g_dodgeBestDifficulty = jsonInt(profilePart, "dodgeBestDifficulty");

    // Parse achievements array
    size_t ap = profilePart.find("\"achievements\":[");
    if (ap != string::npos) {
        ap += 16;
        for (size_t i = 0; i < achievements.size(); i++) {
            size_t end = profilePart.find_first_of(",]", ap);
            if (end == string::npos) break;
            string val = profilePart.substr(ap, end - ap);
            achievements[i].unlocked = (val == "true");
            ap = end + 1;
        }
    }
    return true;
}

void saveProfile() {
    saveLocalProfile();
    if (g_onlineMode) {
        if (saveOnlineProfile())
            unlockAchievement("Cloud Saver");
    }
}

// ============================================================
// EFFETTI VISIVI
// ============================================================
void transitionGlitch() {
    srand((unsigned)time(nullptr));
    for (int i = 0; i < 8; i++) {
        CLEAR_SCREEN();
        setColor(15);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (rand() % 3 == 0) cout << (char)(33 + rand() % 94);
                else cout << ' ';
            }
            cout << '\n';
        }
        PLATFORM_SLEEP(40);
    }
    resetColor();
    CLEAR_SCREEN();
}

void transitionCRT() {
    for (int step = 0; step < WIDTH / 2; step++) {
        CLEAR_SCREEN();
        for (int y = 0; y < HEIGHT; y++) {
            cout << string(step, ' ');
            for (int x = step; x < WIDTH - step; x++) cout << ' ';
            cout << '\n';
        }
        PLATFORM_SLEEP(12);
    }
    CLEAR_SCREEN();
    for (int y = 0; y < HEIGHT; y++) {
        gotoXY(WIDTH / 2, y);
        setColor(10);
        cout << '|';
    }
    PLATFORM_SLEEP(200);
    resetColor();
    CLEAR_SCREEN();
}

void transitionDissolve() {
    for (int step = 0; step < HEIGHT; step++) {
        CLEAR_SCREEN();
        setColor(2);
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (y >= step) cout << (char)(33 + rand() % 94);
                else cout << ' ';
            }
            cout << '\n';
        }
        PLATFORM_SLEEP(18);
    }
    resetColor();
    CLEAR_SCREEN();
}

void playTransition(int mode, const string& name = "") {
    if (mode == 4) {
        if (name == "Neo")       { transitionDissolve(); return; }
        if (name == "Trinity")   { transitionGlitch();   return; }
        if (name == "Morpheus")  { transitionCRT();      return; }
        if (name == "Valentino") { int r = rand()%3; if(r==0) transitionGlitch(); else if(r==1) transitionCRT(); else transitionDissolve(); return; }
    }
    if (mode == 0) { int r = rand()%3; if(r==0) transitionGlitch(); else if(r==1) transitionCRT(); else transitionDissolve(); return; }
    if (mode == 1) transitionGlitch();
    if (mode == 2) transitionCRT();
    if (mode == 3) transitionDissolve();
}

// Matrix rain full-screen
void matrixRainPro(int durationMs) {
    const int cols = WIDTH;
    int headY[80] = {};
    int speed[80]  = {};
    for (int i = 0; i < cols; i++) {
        headY[i] = rand() % HEIGHT;
        speed[i] = 1 + rand() % 3;
    }
    uint64_t start = getTickMs();
    hideCursor();
    while ((int)(getTickMs() - start) < durationMs) {
        CLEAR_SCREEN();
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < cols; x++) {
                int h = headY[x];
                if (y == h) { setColor(10); cout << (char)(33 + rand()%94); }
                else if (y < h && y > h-6) { setColor(2); cout << (char)(33+rand()%94); }
                else cout << ' ';
            }
            cout << '\n';
        }
        for (int i = 0; i < cols; i++) {
            headY[i] += speed[i];
            if (headY[i] - 6 > HEIGHT + rand()%10) {
                headY[i] = -(rand()%HEIGHT);
                speed[i] = 1 + rand()%3;
            }
        }
        PLATFORM_SLEEP(70);
    }
    resetColor();
    CLEAR_SCREEN();
    showCursor();
}

// Frame singolo della pioggia (per il menu animato)
void matrixBgFrame(int headY[], int speed[], int w, int h) {
    for (int x = 0; x < w; x++) {
        int hh = headY[x];
        bool glitch = (rand()%40 == 0);
        if (hh >= 0 && hh < h) {
            gotoXY(x, hh);
            setColor(glitch ? 15 : 10);
            cout << (char)(33 + rand()%94);
        }
        for (int t = 1; t < 6; t++) {
            int y = hh - t;
            if (y >= 0 && y < h) { gotoXY(x, y); setColor(2); cout << (char)(33+rand()%94); }
        }
        int tail = hh - 6;
        if (tail >= 0 && tail < h) { gotoXY(x, tail); cout << ' '; }
        headY[x] += speed[x];
        if (headY[x] > h + rand()%20) {
            headY[x] = -(rand()%h);
            speed[x] = 1 + rand()%3;
        }
    }
    resetColor();
}

// ============================================================
// ACHIEVEMENTS SCREEN
// ============================================================
void showAchievements() {
    disableRawMode();
    CLEAR_SCREEN();
    setColor(10);
    cout << "========== ACHIEVEMENTS ==========\n\n";
    resetColor();

    int total = 0, unl = 0;
    for (auto& a : achievements) { total++; if (a.unlocked) unl++; }
    cout << "Profilo: " << g_playerName << "  |  " << unl << "/" << total
         << " (" << (total ? unl*100/total : 0) << "%)\n\n";

    const char* labels[] = {"[FACILI]", "[MEDI]", "[DIFFICILI]", "[SEGRETI]"};
    int cols[] = {10, 14, 12, 13};
    for (int d = 0; d <= 3; d++) {
        setColor(cols[d]);
        cout << labels[d] << "\n";
        resetColor();
        for (auto& a : achievements) {
            if (a.difficulty != d) continue;
            if (a.unlocked) { setColor(14); cout << " [v] " << a.name << " - " << a.description << "\n"; }
            else            { setColor(8);  cout << " [ ] " << a.name << "\n"; }
            resetColor();
        }
        cout << "\n";
    }
    waitKey();
}

// ============================================================
// LEADERBOARD ONLINE
// ============================================================
void showLeaderboard() {
    if (!g_onlineMode) {
        cout << "\nLeaderboard disponibile solo in modalita' online.\n";
        waitKey();
        return;
    }
    CLEAR_SCREEN();
    setColor(11);
    cout << "========== LEADERBOARD ONLINE ==========\n\n";
    resetColor();

    cout << "--- TOP SNAKE ---\n";
    string resp = httpGet(g_backendUrl + "/api/leaderboard/snake");
    size_t lbPos = resp.find("\"leaderboard\":[");
    if (lbPos != string::npos) {
        string lb = resp.substr(lbPos + 15);
        int rank = 1;
        size_t p = 0;
        while (p < lb.size() && lb[p] != ']' && rank <= 10) {
            size_t us = lb.find("\"username\":\"", p);
            size_t sc = lb.find("\"score\":", p);
            if (us == string::npos || sc == string::npos) break;
            us += 12;
            size_t ue = lb.find('"', us);
            sc += 8;
            size_t se = lb.find_first_of(",}", sc);
            if (ue == string::npos || se == string::npos) break;
            string user  = lb.substr(us, ue - us);
            string score = lb.substr(sc, se - sc);
            cout << "  " << rank++ << ". " << user << " - " << score << " punti\n";
            p = se;
        }
    }

    cout << "\n--- TOP DODGE ---\n";
    resp = httpGet(g_backendUrl + "/api/leaderboard/dodge");
    lbPos = resp.find("\"leaderboard\":[");
    if (lbPos != string::npos) {
        string lb = resp.substr(lbPos + 15);
        int rank = 1;
        size_t p = 0;
        while (p < lb.size() && lb[p] != ']' && rank <= 10) {
            size_t us = lb.find("\"username\":\"", p);
            size_t tc = lb.find("\"time\":", p);
            if (us == string::npos || tc == string::npos) break;
            us += 12;
            size_t ue = lb.find('"', us);
            tc += 7;
            size_t te = lb.find_first_of(",}", tc);
            if (ue == string::npos || te == string::npos) break;
            string user = lb.substr(us, ue - us);
            string time_s = lb.substr(tc, te - tc);
            cout << "  " << rank++ << ". " << user << " - " << time_s << " secondi\n";
            p = te;
        }
    }

    cout << "\n";
    waitKey();
}

// ============================================================
// GIOCO BOOT
// ============================================================
void gameBootScreen(const string& title) {
    disableRawMode();
    CLEAR_SCREEN();
    setColor(11);
    cout << "[ " << title << " SYSTEM BOOTING ]\n";
    resetColor();
    PLATFORM_SLEEP(200);
    slowPrint("Inizializzazione moduli...\n", 18);
    PLATFORM_SLEEP(100);
    slowPrint("Caricamento logica di gioco...\n", 18);
    PLATFORM_SLEEP(100);
    slowPrint("Calibrazione difficolta'...\n", 18);
    PLATFORM_SLEEP(100);
    slowPrint("Pronto.\n", 18);
    PLATFORM_SLEEP(300);
}

string difficultyName(int d) {
    if (d == 1) return "Facile";
    if (d == 2) return "Medio";
    if (d == 3) return "Difficile";
    return "-";
}

// ============================================================
// MINI-GIOCO: INDOVINA IL NUMERO
// ============================================================
void miniGiocoIndovina(const string& nomeUtente, int diff) {
    disableRawMode();
    playTransition(0, nomeUtente);
    CLEAR_SCREEN();
    setColor(11);
    cout << "=== INDOVINA IL NUMERO ===\n\n";
    resetColor();

    int maxRange = (diff == 1) ? 20 : (diff == 2) ? 50 : 100;
    int numeroSegreto = rand() % maxRange + 1;
    int tentativi = 0;
    time_t startTime = time(nullptr);

    cout << "Ho scelto un numero tra 1 e " << maxRange << ". Indovinalo!\n\n";

    int tentativo = 0;
    do {
        cout << "Il tuo tentativo: ";
        if (!(cin >> tentativo)) {
            cout << "Input non valido.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        tentativi++;
        if (tentativo < 1 || tentativo > maxRange) { cout << "Numero fuori range!\n"; continue; }
        if (tentativo > numeroSegreto)      { setColor(12); cout << "Troppo alto!\n";  resetColor(); }
        else if (tentativo < numeroSegreto) { setColor(14); cout << "Troppo basso!\n"; resetColor(); }
        else {
            int durata = (int)(time(nullptr) - startTime);
            setColor(10);
            cout << "\nHAI INDOVINATO in " << tentativi << " tentativi e " << durata << " secondi!\n";
            resetColor();
            if (durata < g_migliorTempo)     { g_migliorTempo = durata;     setColor(14); cout << "Nuovo record di tempo!\n";      resetColor(); }
            if (tentativi < g_migliorTentativi) { g_migliorTentativi = tentativi; setColor(14); cout << "Nuovo record di tentativi!\n"; resetColor(); }
            if (durata < 5)     unlockAchievement("Speedrunner");
            if (tentativi == 1) unlockAchievement("Perfect Guess");
            if (diff == 3)      unlockAchievement("Hardcore Mode");
            g_vittorieConsecutive++;
            if (g_vittorieConsecutive >= 3) unlockAchievement("Persistence");
            saveProfile();
        }
    } while (tentativo != numeroSegreto);

    waitKey();
    playTransition(0, nomeUtente);
    CLEAR_SCREEN();
}

// ============================================================
// GIOCO: SNAKE
// ============================================================
struct SnakeSeg { int x, y; };

void giocoSnake(const string& nomeUtente, int diff) {
    gameBootScreen("SNAKE");
    enableRawMode();
    hideCursor();

    const int W = 30, H = 20;
    int delayMs = (diff == 1) ? 150 : (diff == 2) ? 100 : 60;
    vector<SnakeSeg> snake;
    snake.push_back({W/2, H/2});
    int dirX = 1, dirY = 0;
    int foodX = rand()%W, foodY = rand()%H;
    int score = 0;
    bool over = false;

    while (!over) {
        int key = platformReadKey();
        if (key == KEY_UP_CODE    || key == 'w' || key == 'W') { if (dirY!=1) { dirX=0; dirY=-1; } }
        if (key == KEY_DOWN_CODE  || key == 's' || key == 'S') { if (dirY!=-1){ dirX=0; dirY=1;  } }
        if (key == KEY_LEFT_CODE  || key == 'a' || key == 'A') { if (dirX!=1) { dirX=-1;dirY=0;  } }
        if (key == KEY_RIGHT_CODE || key == 'd' || key == 'D') { if (dirX!=-1){ dirX=1; dirY=0;  } }
        if (key == 'q' || key == 'Q') break;

        SnakeSeg head = snake.front();
        SnakeSeg nh = {head.x+dirX, head.y+dirY};
        if (nh.x<0||nh.x>=W||nh.y<0||nh.y>=H) over=true;
        for (auto& s : snake) if (s.x==nh.x&&s.y==nh.y) { over=true; break; }
        if (over) break;

        snake.insert(snake.begin(), nh);
        if (nh.x==foodX && nh.y==foodY) {
            score++;
            bool valid=false;
            while(!valid) { foodX=rand()%W; foodY=rand()%H; valid=true; for(auto& s:snake) if(s.x==foodX&&s.y==foodY){valid=false;break;} }
        } else {
            snake.pop_back();
        }

        CLEAR_SCREEN();
        setColor(10); cout<<"=== SNAKE ===  Giocatore: "<<nomeUtente<<"  Punteggio: "<<score<<"  [Q=Esci]\n"; resetColor();
        for (int y=-1; y<=H; y++) {
            for (int x=-1; x<=W; x++) {
                if (y==-1||y==H||x==-1||x==W) { cout<<'#'; continue; }
                bool pr=false;
                if (x==foodX&&y==foodY) { setColor(12); cout<<'*'; setColor(7); pr=true; }
                else for (size_t i=0; i<snake.size(); i++) {
                    if (snake[i].x==x&&snake[i].y==y) {
                        setColor(i==0?10:2); cout<<(i==0?'O':'o'); resetColor(); pr=true; break;
                    }
                }
                if (!pr) cout<<' ';
            }
            cout<<'\n';
        }
        PLATFORM_SLEEP(delayMs);
    }

    showCursor();
    disableRawMode();
    CLEAR_SCREEN();
    setColor(12); cout<<"GAME OVER!\n\n"; resetColor();
    cout<<"Punteggio finale: "<<score<<"\n";
    if (score > g_snakeBestScore) {
        g_snakeBestScore=score; g_snakeBestDifficulty=diff;
        setColor(14); cout<<"Nuovo record personale!\n"; resetColor();
    } else {
        cout<<"Record attuale: "<<g_snakeBestScore<<" ("<<difficultyName(g_snakeBestDifficulty)<<")\n";
    }
    if (score>=10) unlockAchievement("Snake Rookie");
    if (score>=30) unlockAchievement("Snake Survivor");
    if (score>=50) unlockAchievement("Snake Master");
    saveProfile();
    waitKey();
}

// ============================================================
// GIOCO: DODGE
// ============================================================
struct Obstacle { int x, y; };

void giocoDodge(const string& nomeUtente, int diff) {
    gameBootScreen("DODGE GAME");
    enableRawMode();
    hideCursor();

    const int W=30, H=20;
    int delayMs = (diff==1)?120:(diff==2)?80:50;
    int playerX=W/2, playerY=H-2;
    vector<Obstacle> obs;
    bool over=false;
    uint64_t startT = getTickMs();
    int survived=0;

    while (!over) {
        int key = platformReadKey();
        if (key==KEY_LEFT_CODE  ||key=='a'||key=='A') { playerX--; if(playerX<0) playerX=0; }
        if (key==KEY_RIGHT_CODE ||key=='d'||key=='D') { playerX++; if(playerX>=W) playerX=W-1; }
        if (key=='q'||key=='Q') break;

        int spawnChance = (diff==1)?12:(diff==2)?8:5;
        if (rand()%spawnChance==0) obs.push_back({rand()%W, 0});

        for (auto& o : obs) o.y++;

        vector<Obstacle> nobs;
        for (auto& o : obs) {
            if (o.y==playerY&&o.x==playerX) over=true;
            if (o.y<H) nobs.push_back(o);
        }
        obs=nobs;

        survived=(int)((getTickMs()-startT)/1000);

        CLEAR_SCREEN();
        setColor(10);
        cout<<"=== DODGE ===  Giocatore: "<<nomeUtente<<"  Tempo: "<<survived<<"s  [Q=Esci]\n";
        resetColor();
        for (int y=0; y<H; y++) {
            for (int x=0; x<W; x++) {
                bool pr=false;
                if (y==playerY&&x==playerX) { setColor(14); cout<<'^'; resetColor(); pr=true; }
                else for (auto& o:obs) if(o.x==x&&o.y==y) { setColor(12); cout<<'*'; resetColor(); pr=true; break; }
                if (!pr) cout<<' ';
            }
            cout<<'\n';
        }
        PLATFORM_SLEEP(delayMs);
    }

    showCursor();
    disableRawMode();
    CLEAR_SCREEN();
    if (over) { setColor(12); cout<<"SEI STATO COLPITO!\n\n"; resetColor(); }
    cout<<"Tempo sopravvissuto: "<<survived<<" secondi\n";
    if (survived>g_dodgeBestTime) {
        g_dodgeBestTime=survived; g_dodgeBestDifficulty=diff;
        setColor(14); cout<<"Nuovo record personale!\n"; resetColor();
    } else {
        cout<<"Record attuale: "<<g_dodgeBestTime<<"s ("<<difficultyName(g_dodgeBestDifficulty)<<")\n";
    }
    if (survived>=20) unlockAchievement("Dodger");
    if (survived>=40) unlockAchievement("Ultra Dodger");
    if (survived>=60) unlockAchievement("Untouchable");
    saveProfile();
    waitKey();
}

// ============================================================
// MENU DIFFICOLTA'
// ============================================================
int scegliDifficolta() {
    enableRawMode();
    hideCursor();
    int sel=0;
    string opts[]={"Facile","Medio","Difficile","Torna indietro"};
    while (true) {
        CLEAR_SCREEN();
        setColor(10); cout<<"=== SCEGLI LA DIFFICOLTA' ===\n\n"; resetColor();
        cout<<"Usa W/S o frecce, INVIO per confermare.\n\n";
        for (int i=0; i<4; i++) {
            if (i==sel) { setColor(10); cout<<"> "<<opts[i]<<"\n"; resetColor(); }
            else { setColor(7); cout<<"  "<<opts[i]<<"\n"; resetColor(); }
        }
        int key = platformReadKey();
        PLATFORM_SLEEP(80);
        if (key==KEY_UP_CODE||key=='w'||key=='W') { sel--; if(sel<0) sel=3; }
        else if (key==KEY_DOWN_CODE||key=='s'||key=='S') { sel++; if(sel>3) sel=0; }
        else if (key=='\n'||key=='\r'||key==13) { disableRawMode(); showCursor(); return sel+1; }
    }
}

// ============================================================
// INSTALLATION WIZARD (prima esecuzione)
// ============================================================
bool isInstalled() { return fs::exists(".vgc_installed"); }

void runInstallWizard() {
    CLEAR_SCREEN();
    setColor(10);
    cout << R"(
  ██╗   ██╗ ██████╗  ██████╗
  ██║   ██║██╔════╝ ██╔════╝
  ██║   ██║██║  ███╗██║
  ╚██╗ ██╔╝██║   ██║██║
   ╚████╔╝ ╚██████╔╝╚██████╗
    ╚═══╝   ╚═════╝  ╚═════╝
)" << "\n";
    setColor(11);
    cout << "  VALENTINO GAME COLLECTION - Installer\n";
    setColor(7);
    cout << "  ======================================\n\n";
    PLATFORM_SLEEP(800);

    struct Step { const char* msg; int ms; };
    Step steps[] = {
        {"  [>>] Verifica requisiti di sistema...",   400},
        {"  [>>] Creazione cartelle di gioco...",     300},
        {"  [>>] Installazione core engine...",       500},
        {"  [>>] Configurazione grafica terminale...",400},
        {"  [>>] Caricamento sistema achievements...",300},
        {"  [>>] Configurazione profili...",          400},
        {"  [>>] Verifica connessione server...",     600},
        {"  [>>] Finalizzazione installazione...",    400},
    };
    for (auto& s : steps) {
        setColor(14);
        slowPrint(string(s.msg) + "\n", 12);
        resetColor();
        // progress bar
        cout << "  [";
        for (int i=0; i<30; i++) {
            cout << '#' << flush;
            PLATFORM_SLEEP(s.ms / 30);
        }
        cout << "] OK\n";
    }

    PLATFORM_SLEEP(400);
    CLEAR_SCREEN();
    setColor(10);
    cout << "\n  Installazione completata!\n\n";
    setColor(7);
    cout << "  Benvenuto in Valentino Game Collection.\n";
    cout << "  La tua avventura sta per iniziare...\n\n";
    PLATFORM_SLEEP(1200);

    ensureProfilesDir();
    ofstream f(".vgc_installed");
    f << "installed";
    f.close();
}

// ============================================================
// AUTENTICAZIONE ONLINE
// ============================================================
string promptHidden(const string& label) {
    cout << label;
    string pw;
#ifdef _WIN32
    char ch;
    while ((ch = _getch()) != '\r') {
        if (ch == '\b') { if (!pw.empty()) { pw.pop_back(); cout << "\b \b"; } }
        else { pw += ch; cout << '*'; }
    }
    cout << '\n';
#else
    struct termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    struct termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    getline(cin, pw);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << '\n';
#endif
    return pw;
}

bool doLogin() {
    CLEAR_SCREEN();
    setColor(11);
    cout << "=== LOGIN ===\n\n";
    resetColor();
    cout << "Email: ";
    string email; getline(cin, email);
    string pw = promptHidden("Password: ");

    string body = "{\"email\":\""+email+"\",\"password\":\""+pw+"\"}";
    string resp = httpPost(g_backendUrl + "/api/login", body);
    if (!jsonBool(resp, "success")) {
        setColor(12);
        string msg = jsonStr(resp, "message");
        cout << "\nErrore: " << (msg.empty() ? "impossibile connettersi al server" : msg) << "\n";
        resetColor();
        waitKey();
        return false;
    }
    g_authToken  = jsonStr(resp, "token");
    g_playerName = jsonStr(resp, "username");
    setColor(10);
    cout << "\nBenvenuto, " << g_playerName << "!\n";
    resetColor();
    PLATFORM_SLEEP(800);
    unlockAchievement("First Login");
    return true;
}

bool doRegister() {
    CLEAR_SCREEN();
    setColor(11);
    cout << "=== REGISTRAZIONE ===\n\n";
    resetColor();
    cout << "Username: ";
    string username; getline(cin, username);
    cout << "Email: ";
    string email; getline(cin, email);
    string pw  = promptHidden("Password (min 6 caratteri): ");
    string pw2 = promptHidden("Conferma password: ");
    if (pw != pw2) {
        setColor(12); cout << "\nLe password non corrispondono.\n"; resetColor();
        waitKey(); return false;
    }
    string body = "{\"username\":\""+username+"\",\"email\":\""+email+"\",\"password\":\""+pw+"\"}";
    string resp = httpPost(g_backendUrl + "/api/register", body);
    if (!jsonBool(resp, "success")) {
        setColor(12);
        string msg = jsonStr(resp, "message");
        cout << "\nErrore: " << (msg.empty() ? "impossibile connettersi al server" : msg) << "\n";
        resetColor();
        waitKey();
        return false;
    }
    g_authToken  = jsonStr(resp, "token");
    g_playerName = username;
    setColor(10);
    cout << "\nRegistrazione completata! Benvenuto, " << g_playerName << "!\n";
    resetColor();
    PLATFORM_SLEEP(800);
    return true;
}

// ============================================================
// MENU PROFILI (offline)
// ============================================================
bool offlineProfileMenu() {
    while (true) {
        CLEAR_SCREEN();
        cout<<"=====================================\n";
        cout<<"         PROFILI OFFLINE\n";
        cout<<"=====================================\n\n";
        cout<<"1) Nuovo profilo\n";
        cout<<"2) Seleziona profilo\n";
        cout<<"3) Elimina profilo\n";
        cout<<"4) Torna indietro\n\n";
        auto profs = listLocalProfiles();
        cout<<"Profili salvati:\n";
        if (profs.empty()) cout<<"  - Nessuno\n\n";
        else for (size_t i=0;i<profs.size();i++) cout<<"  "<<(i+1)<<") "<<profs[i]<<"\n";
        cout<<"\nScelta: ";
        int c; if(!(cin>>c)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
        if (c==1) {
            cout<<"Nome: "; string name; getline(cin,name);
            initAchievements(); g_playerName=name;
            g_migliorTempo=9999; g_migliorTentativi=9999; g_vittorieConsecutive=0;
            g_snakeBestScore=g_snakeBestDifficulty=g_dodgeBestTime=g_dodgeBestDifficulty=0;
            saveLocalProfile(); return true;
        } else if (c==2) {
            if (profs.empty()) { cout<<"Nessun profilo.\n"; waitKey(); continue; }
            cout<<"Numero: "; int idx; if(!(cin>>idx)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
            if (idx<1||idx>(int)profs.size()) { cout<<"Non valido.\n"; waitKey(); continue; }
            initAchievements();
            if (!loadLocalProfile(profs[idx-1])) { cout<<"Errore caricamento.\n"; waitKey(); continue; }
            return true;
        } else if (c==3) {
            if (profs.empty()) { cout<<"Nessun profilo.\n"; waitKey(); continue; }
            cout<<"Numero: "; int idx; if(!(cin>>idx)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
            if (idx<1||idx>(int)profs.size()) { cout<<"Non valido.\n"; waitKey(); continue; }
            error_code ec; fs::remove(profilePath(profs[idx-1]),ec);
            cout<<"Eliminato.\n"; waitKey();
        } else if (c==4) {
            return false;
        }
    }
}

// ============================================================
// SCHERMATA DI ACCESSO PRINCIPALE
// ============================================================
bool accessScreen() {
    while (true) {
        CLEAR_SCREEN();
        setColor(10);
        cout << "\n  =====================================\n";
        cout << "     VALENTINO GAME COLLECTION\n";
        cout << "  =====================================\n\n";
        resetColor();

        bool online = g_onlineMode;
        if (online) {
            setColor(14);
            cout << "  [ SERVER ONLINE DISPONIBILE ]\n\n";
            resetColor();
            cout << "  1) Login (email + password)\n";
            cout << "  2) Registrati\n";
            cout << "  3) Gioca offline\n";
            cout << "  4) Esci\n";
        } else {
            setColor(8);
            cout << "  [ SERVER OFFLINE - modalita' locale ]\n\n";
            resetColor();
            cout << "  1) Seleziona / crea profilo offline\n";
            cout << "  2) Esci\n";
        }

        cout << "\n  Scelta: ";
        int c; if(!(cin>>c)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');

        if (online) {
            if (c==1) { if(doLogin())   { g_onlineMode=true; loadOnlineProfile(); return true; } }
            else if (c==2) { if(doRegister()) { g_onlineMode=true; return true; } }
            else if (c==3) { g_onlineMode=false; if(offlineProfileMenu()) return true; }
            else if (c==4) return false;
        } else {
            if (c==1) { if(offlineProfileMenu()) return true; }
            else if (c==2) return false;
        }
    }
}

// ============================================================
// MENU PRINCIPALE ANIMATO
// ============================================================
int matrixMenu(const string& nome) {
    enableRawMode();
    hideCursor();

    int headY[80]={}, speed[80]={};
    for (int i=0;i<WIDTH;i++) { headY[i]=rand()%HEIGHT; speed[i]=1+rand()%3; }

    string secretInput, codeInput;
    int sel=0;
    const int N=7;
    string opts[N]={
        "Indovina il numero",
        "Snake",
        "Dodge Game",
        "Achievements",
        "Leaderboard",
        "Cambia modalita'",
        "Esci"
    };

    CLEAR_SCREEN();

    while (true) {
        matrixBgFrame(headY, speed, WIDTH, HEIGHT);

        // Menu overlay
        gotoXY(0, 2);
        setColor(10);
        cout << "  =====[ VALENTINO GAME COLLECTION ]=====\n";
        resetColor();
        gotoXY(2, 3);
        cout << "  Giocatore: ";
        setColor(14); cout << nome; resetColor();
        if (g_onlineMode) { setColor(11); cout << "  [ONLINE]"; resetColor(); }
        else              { setColor(8);  cout << "  [OFFLINE]"; resetColor(); }

        if (g_migliorTempo     < 9999) { gotoXY(2,4); cout<<"  Record Tempo: ";setColor(14);cout<<g_migliorTempo<<"s";resetColor(); }
        if (g_migliorTentativi < 9999) { gotoXY(2,5); cout<<"  Record Tent.: ";setColor(14);cout<<g_migliorTentativi;resetColor(); }
        if (g_snakeBestScore   >    0) { gotoXY(2,6); cout<<"  Record Snake: ";setColor(14);cout<<g_snakeBestScore;resetColor(); }
        if (g_dodgeBestTime    >    0) { gotoXY(2,7); cout<<"  Record Dodge: ";setColor(14);cout<<g_dodgeBestTime<<"s";resetColor(); }

        for (int i=0; i<N; i++) {
            gotoXY(4, 9+i);
            if (i==sel) { setColor(10); cout<<"> "<<opts[i]<<"   "; resetColor(); }
            else        { setColor(7);  cout<<"  "<<opts[i]<<"   "; resetColor(); }
        }
        gotoXY(0, 9+N+1);
        setColor(8); cout<<"  W/S o frecce: naviga  |  INVIO: seleziona  |  R/B: pillola"; resetColor();

        int key = platformReadKey();
        PLATFORM_SLEEP(60);

        if (key==KEY_UP_CODE  ||key=='w'||key=='W') { sel--; if(sel<0) sel=N-1; }
        else if (key==KEY_DOWN_CODE||key=='s'||key=='S') { sel++; if(sel>=N) sel=0; }
        else if (key=='\n'||key=='\r'||key==13) {
            disableRawMode(); showCursor();
            playTransition(4, nome);
            CLEAR_SCREEN();
            return sel+1;
        }
        else if (key=='r'||key=='R') { unlockAchievement("Red Awakening"); }
        else if (key=='b'||key=='B') { unlockAchievement("Blue Dream"); }

        // Easter egg codici
        if (key > 0 && key < 128) {
            secretInput += (char)key;
            codeInput   += (char)key;
            if (secretInput.size() > 40) secretInput.erase(0, secretInput.size()-40);
            if (codeInput.size()   > 10) codeInput.erase(0, codeInput.size()-10);
        }
        if (secretInput.find("opensesame") != string::npos) {
            unlockAchievement("Secret Door");
            secretInput.clear();
        }
        if (codeInput == "404") {
            unlockAchievement("System Error");
            codeInput.clear();
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    srand((unsigned)time(nullptr));

#ifndef _WIN32
    // Ripristina terminale all'uscita
    atexit([](){
        disableRawMode();
        showCursor();
        printf("\033[0m\n");
    });
#endif

    ensureProfilesDir();
    initAchievements();

    // Installazione (prima esecuzione)
    if (!isInstalled()) {
        runInstallWizard();
    }

    // Controlla disponibilita' backend
    setColor(8);
    cout << "\nConnessione al server...";
    fflush(stdout);
    g_onlineMode = backendAvailable();
    if (g_onlineMode) { setColor(10); cout << " OK\n"; }
    else              { setColor(8);  cout << " Non disponibile (modalita' offline)\n"; }
    resetColor();
    PLATFORM_SLEEP(600);

    // Schermata di accesso
    if (!accessScreen()) {
        cout << "\nArrivederci!\n";
        return 0;
    }

    // Easter egg nomi
    bool eNeo       = (g_playerName=="Neo");
    bool eTrinity   = (g_playerName=="Trinity");
    bool eMorpheus  = (g_playerName=="Morpheus");
    bool eValentino = (g_playerName=="Valentino");
    bool eArchitect = (g_playerName=="Architect");
    if (eNeo)       unlockAchievement("THE ONE");
    if (eTrinity)   unlockAchievement("Connection Est.");
    if (eMorpheus)  unlockAchievement("Free Your Mind");
    if (eValentino) unlockAchievement("Dev God Mode");
    if (eArchitect) unlockAchievement("The Architect");

    // Intro
    CLEAR_SCREEN();
    setColor(10);
    slowPrint("\n  =====================================\n", 4);
    setColor(10); slowPrint("      VALENTINO GAME COLLECTION\n", 35);
    setColor(10); slowPrint("  =====================================\n\n", 4);
    resetColor();
    slowPrint("  Caricamento", 35);
    for (int i=0;i<5;i++) { cout<<'.'; fflush(stdout); PLATFORM_SLEEP(300); }
    cout<<"\n\n";

    playTransition(0, g_playerName);
    matrixRainPro(2200);

    int scelta=0;
    do {
        scelta = matrixMenu(g_playerName);

        if (scelta==1) {
            int d = scegliDifficolta();
            if (d<=3) miniGiocoIndovina(g_playerName, d);
        }
        else if (scelta==2) {
            int d = scegliDifficolta();
            if (d<=3) giocoSnake(g_playerName, d);
        }
        else if (scelta==3) {
            int d = scegliDifficolta();
            if (d<=3) giocoDodge(g_playerName, d);
        }
        else if (scelta==4) { showAchievements(); }
        else if (scelta==5) { showLeaderboard(); }
        else if (scelta==6) {
            if (!g_onlineMode) {
                g_onlineMode = true;
                cout<<"\nVerifica connessione...\n";
                if (!backendAvailable()) {
                    setColor(12); cout<<"Server non raggiungibile.\n"; resetColor();
                    g_onlineMode=false;
                } else {
                    setColor(10); cout<<"Online! Effettua login.\n"; resetColor();
                    if (!doLogin()) g_onlineMode=false;
                    else { loadOnlineProfile(); }
                }
            } else {
                g_onlineMode=false;
                setColor(8); cout<<"\nModalita' offline attivata.\n"; resetColor();
            }
            PLATFORM_SLEEP(900);
        }
    } while (scelta != 7);

    saveProfile();
    CLEAR_SCREEN();
    setColor(10);
    cout << "\n  Progresso salvato. Arrivederci!\n\n";
    resetColor();
    return 0;
}
