// ============================================================
// VALENTINO GAME COLLECTION - Cross-Platform Edition v2.0
// by Valentino — The_Meme_Gaimer
// Funziona su Windows, Linux e macOS
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
#include <map>

#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
  #define CLEAR_SCREEN() system("cls")
  #define PLATFORM_SLEEP(ms) Sleep(ms)
  #define KEY_UP_CODE    1001
  #define KEY_DOWN_CODE  1002
  #define KEY_LEFT_CODE  1003
  #define KEY_RIGHT_CODE 1004
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
// TERMINALE cross-platform
// ============================================================
#ifndef _WIN32
static struct termios g_origTermios;
static bool g_rawMode = false;

void enableRawMode() {
    if (g_rawMode) return;
    tcgetattr(STDIN_FILENO, &g_origTermios);
    struct termios raw = g_origTermios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
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
    fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
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
            fd_set fds2; FD_ZERO(&fds2); FD_SET(STDIN_FILENO, &fds2);
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
#else
void enableRawMode()  {}
void disableRawMode() {}
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
#endif

void hideCursor() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(out,&ci); ci.bVisible=FALSE; SetConsoleCursorInfo(out,&ci);
#else
    printf("\033[?25l"); fflush(stdout);
#endif
}
void showCursor() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci; GetConsoleCursorInfo(out,&ci); ci.bVisible=TRUE; SetConsoleCursorInfo(out,&ci);
#else
    printf("\033[?25h"); fflush(stdout);
#endif
}
void setColor(int c) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
#else
    const char* codes[] = {
        "\033[0;30m","\033[0;34m","\033[0;32m","\033[0;36m",
        "\033[0;31m","\033[0;35m","\033[0;33m","\033[0;37m",
        "\033[0;90m","\033[0;94m","\033[0;92m","\033[0;96m",
        "\033[0;91m","\033[0;95m","\033[0;93m","\033[0;97m"
    };
    if (c >= 0 && c < 16) printf("%s", codes[c]);
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
    COORD c; c.X=x; c.Y=y; SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),c);
#else
    printf("\033[%d;%dH", y+1, x+1); fflush(stdout);
#endif
}
uint64_t getTickMs() {
    return (uint64_t)chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now().time_since_epoch()).count();
}
void slowPrint(const string& text, int delayMs = 30) {
    for (char c : text) { cout << c << flush; PLATFORM_SLEEP(delayMs); }
}

// ============================================================
// SISTEMA DI TRADUZIONE (IT=0, EN=1, FR=2)
// ============================================================
int g_language = 0;

enum TKey {
    T_PRESS_ENTER,
    T_DIFFICULTY, T_EASY, T_MEDIUM, T_HARD, T_BACK,
    T_GUESS_GAME, T_SNAKE, T_DODGE, T_ACHIEVEMENTS, T_LEADERBOARD,
    T_FRIENDS, T_CHALLENGES, T_NOTIFICATIONS, T_STATS, T_SETTINGS, T_QUIT,
    T_VOLUME, T_LANGUAGE_LABEL, T_PROFILE, T_CREDITS,
    T_CREDITS_FOUNDER, T_CREDITS_THANKS, T_CREDITS_RIGHTS,
    T_CREDITS_TITLE,
    T_SETTINGS_TITLE,
    T_STATS_TITLE, T_STATS_SNAKE_HIST, T_STATS_DODGE_HIST,
    T_STATS_INDOVINA_HIST, T_STATS_NO_DATA, T_STATS_SCORE, T_STATS_TIME,
    T_STATS_TRIES, T_STATS_BEST_RECORDS,
    T_ONLINE, T_OFFLINE,
    T_NAVIGATE, T_SELECT,
    T_FRIENDS_TITLE, T_CHALLENGES_TITLE, T_NOTIFS_TITLE,
    T_LEADERBOARD_TITLE, T_ACHIEVE_TITLE,
    T_ONLINE_ONLY,
    T_ADD_FRIEND, T_SEND_CHALLENGE,
    T_SAVE_PROFILE, T_SAVED,
    T_LANG_CHANGED,
    T_CREDITS_ROLE_FOUNDER,
    T_COUNT
};

const char* TRANS[T_COUNT][3] = {
    // IT                                    EN                                    FR
    {"Premi INVIO per continuare...",        "Press ENTER to continue...",         "Appuyez sur ENTREE pour continuer..."}, // T_PRESS_ENTER
    {"DIFFICOLTA'",                          "DIFFICULTY",                         "DIFFICULTE"},                           // T_DIFFICULTY
    {"Facile",                               "Easy",                               "Facile"},                               // T_EASY
    {"Medio",                                "Medium",                             "Moyen"},                                // T_MEDIUM
    {"Difficile",                            "Hard",                               "Difficile"},                            // T_HARD
    {"Torna indietro",                       "Go back",                            "Retour"},                               // T_BACK
    {"Indovina il numero",                   "Guess the number",                   "Devinez le nombre"},                    // T_GUESS_GAME
    {"Snake",                                "Snake",                              "Snake"},                                // T_SNAKE
    {"Dodge Game",                           "Dodge Game",                         "Dodge Game"},                           // T_DODGE
    {"Achievements",                         "Achievements",                       "Succes"},                               // T_ACHIEVEMENTS
    {"Leaderboard",                          "Leaderboard",                        "Classement"},                           // T_LEADERBOARD
    {"Amici",                                "Friends",                            "Amis"},                                 // T_FRIENDS
    {"Sfide",                                "Challenges",                         "Defis"},                                // T_CHALLENGES
    {"Notifiche",                            "Notifications",                      "Notifications"},                        // T_NOTIFICATIONS
    {"Statistiche",                          "Statistics",                         "Statistiques"},                         // T_STATS
    {"Impostazioni",                         "Settings",                           "Parametres"},                           // T_SETTINGS
    {"Esci",                                 "Quit",                               "Quitter"},                              // T_QUIT
    {"Volume",                               "Volume",                             "Volume"},                               // T_VOLUME
    {"Lingua",                               "Language",                           "Langue"},                               // T_LANGUAGE_LABEL
    {"Profilo",                              "Profile",                            "Profil"},                               // T_PROFILE
    {"Crediti",                              "Credits",                            "Credits"},                              // T_CREDITS
    {"Valentino Ingrao",                     "Valentino Ingrao",                   "Valentino Ingrao"},                     // T_CREDITS_FOUNDER
    {"Ringraziamenti",                       "Special Thanks",                     "Remerciements"},                        // T_CREDITS_THANKS
    {"(C) TMG Studio 2026. Tutti i Diritti Riservati.", "(C) TMG Studio 2026. All Rights Reserved.", "(C) TMG Studio 2026. Tous Droits Reserves."}, // T_CREDITS_RIGHTS
    {"====== CREDITI ======",                "====== CREDITS ======",              "====== CREDITS ======"},                // T_CREDITS_TITLE
    {"====== IMPOSTAZIONI ======",           "====== SETTINGS ======",             "====== PARAMETRES ======"},             // T_SETTINGS_TITLE
    {"====== STATISTICHE ======",            "====== STATISTICS ======",           "====== STATISTIQUES ======"},           // T_STATS_TITLE
    {"Storico Snake (ultimi 10)",            "Snake History (last 10)",            "Historique Snake (10 derniers)"},       // T_STATS_SNAKE_HIST
    {"Storico Dodge (ultimi 10)",            "Dodge History (last 10)",            "Historique Dodge (10 derniers)"},       // T_STATS_DODGE_HIST
    {"Storico Indovina (ultimi 10)",         "Guess History (last 10)",            "Historique Devinette (10 derniers)"},   // T_STATS_INDOVINA_HIST
    {"Nessun dato registrato.",              "No data recorded yet.",              "Aucune donnee enregistree."},           // T_STATS_NO_DATA
    {"Punti",                                "Points",                             "Points"},                               // T_STATS_SCORE
    {"Secondi",                              "Seconds",                            "Secondes"},                             // T_STATS_TIME
    {"Tentativi",                            "Tries",                              "Essais"},                               // T_STATS_TRIES
    {"--- RECORD MIGLIORI ---",              "--- BEST RECORDS ---",               "--- MEILLEURS RECORDS ---"},            // T_STATS_BEST_RECORDS
    {"ONLINE",                               "ONLINE",                             "EN LIGNE"},                             // T_ONLINE
    {"OFFLINE",                              "OFFLINE",                            "HORS LIGNE"},                           // T_OFFLINE
    {"W/S o frecce: naviga",                 "W/S or arrows: navigate",            "W/S ou fleches: naviguer"},             // T_NAVIGATE
    {"INVIO: seleziona",                     "ENTER: select",                      "ENTREE: selectionner"},                 // T_SELECT
    {"====== AMICI ======",                  "====== FRIENDS ======",              "====== AMIS ======"},                   // T_FRIENDS_TITLE
    {"====== SFIDE ======",                  "====== CHALLENGES ======",           "====== DEFIS ======"},                  // T_CHALLENGES_TITLE
    {"====== NOTIFICHE ======",              "====== NOTIFICATIONS ======",        "====== NOTIFICATIONS ======"},          // T_NOTIFS_TITLE
    {"====== LEADERBOARD ONLINE ======",     "====== ONLINE LEADERBOARD ======",   "====== CLASSEMENT EN LIGNE ======"},    // T_LEADERBOARD_TITLE
    {"====== ACHIEVEMENTS ======",           "====== ACHIEVEMENTS ======",         "====== SUCCES ======"},                 // T_ACHIEVE_TITLE
    {"Disponibile solo online.",             "Available in online mode only.",     "Disponible en mode en ligne seulement."}, // T_ONLINE_ONLY
    {"Aggiungi amico",                       "Add friend",                         "Ajouter un ami"},                       // T_ADD_FRIEND
    {"Invia nuova sfida a un amico",         "Send a new challenge to a friend",   "Envoyer un nouveau defi a un ami"},     // T_SEND_CHALLENGE
    {"Salva profilo",                        "Save profile",                       "Sauvegarder le profil"},                // T_SAVE_PROFILE
    {"Profilo salvato!",                     "Profile saved!",                     "Profil sauvegarde!"},                   // T_SAVED
    {"Lingua cambiata.",                     "Language changed.",                  "Langue modifiee."},                     // T_LANG_CHANGED
    {"Founder",                              "Founder",                            "Fondateur"},                            // T_CREDITS_ROLE_FOUNDER
};

string tr(TKey k) {
    int l = (g_language >= 0 && g_language < 3) ? g_language : 0;
    return string(TRANS[k][l]);
}

void waitKey(const string& msg = "") {
    string m = msg.empty() ? tr(T_PRESS_ENTER) : msg;
#ifndef _WIN32
    bool was = g_rawMode; if (was) disableRawMode();
#endif
    cout << m << flush;
    string line; getline(cin, line);
#ifndef _WIN32
    if (was) enableRawMode();
#endif
    cout << "\n";
}

// ============================================================
// STATO GLOBALE
// ============================================================
const int WIDTH = 80, HEIGHT = 25;

string g_playerName;
string g_authToken;
bool   g_onlineMode = false;
string g_backendUrl = "http://localhost:8000";

int  g_migliorTempo        = 9999;
int  g_migliorTentativi    = 9999;
int  g_vittorieConsecutive = 0;
bool g_musicOn             = false;
int  g_snakeBestScore      = 0;
int  g_snakeBestDifficulty = 0;
int  g_dodgeBestTime       = 0;
int  g_dodgeBestDifficulty = 0;
int  g_unreadNotifs        = 0;
int  g_volume              = 75;

vector<int> g_snakeHistory;
vector<int> g_dodgeHistory;
vector<int> g_indovinaHistory;

void addToHistory(vector<int>& hist, int val) {
    hist.push_back(val);
    if ((int)hist.size() > 10) hist.erase(hist.begin());
}

// ============================================================
// ACHIEVEMENTS
// ============================================================
struct Achievement { string name, description; bool unlocked=false; int difficulty; };
vector<Achievement> achievements;

void addAchievement(const string& n, const string& d, int diff) {
    achievements.push_back({n,d,false,diff});
}
void initAchievements() {
    achievements.clear();
    addAchievement("THE ONE",          "Hai effettuato l'accesso come Neo.",            0);
    addAchievement("Connection Est.",  "Hai effettuato l'accesso come Trinity.",         0);
    addAchievement("Free Your Mind",   "Hai effettuato l'accesso come Morpheus.",        0);
    addAchievement("Dev God Mode",     "Il creatore e' entrato nel sistema.",            0);
    addAchievement("The Architect",    "Hai effettuato l'accesso come Architect.",       0);
    addAchievement("Red Awakening",    "Hai scelto la pillola rossa.",                   0);
    addAchievement("Blue Dream",       "Hai scelto la pillola blu.",                     0);
    addAchievement("First Login",      "Hai effettuato il primo accesso online.",        0);
    addAchievement("Verified",         "Hai verificato la tua email.",                   0);
    addAchievement("Social Gamer",     "Hai aggiunto il tuo primo amico.",               0);
    addAchievement("Challenger",       "Hai inviato la tua prima sfida.",                1);
    addAchievement("Challenge Won",    "Hai vinto una sfida!",                           2);
    addAchievement("Deja-vu",          "Hai visto il gatto due volte.",                  1);
    addAchievement("Secret Door",      "Hai aperto la stanza segreta.",                  1);
    addAchievement("System Error",     "Hai trovato il codice 404 nascosto.",            1);
    addAchievement("Speedrunner",      "Hai vinto il mini-gioco in meno di 5 secondi.", 2);
    addAchievement("Perfect Guess",    "Hai indovinato al primo tentativo.",             2);
    addAchievement("Hardcore Mode",    "Hai vinto in difficolta' massima.",              2);
    addAchievement("Persistence",      "Hai vinto tre volte di fila.",                   2);
    addAchievement("Snake Rookie",     "Hai raggiunto 10 punti in Snake.",               0);
    addAchievement("Snake Survivor",   "Hai raggiunto 30 punti in Snake.",               1);
    addAchievement("Snake Master",     "Hai raggiunto 50 punti in Snake.",               2);
    addAchievement("Dodger",           "Sopravvissuto 20 secondi in Dodge.",             0);
    addAchievement("Ultra Dodger",     "Sopravvissuto 40 secondi in Dodge.",             1);
    addAchievement("Untouchable",      "Sopravvissuto 60 secondi in Dodge.",             2);
    addAchievement("Cloud Saver",      "Hai salvato il profilo online.",                 0);
}

void achievementPopup(const string& name) {
    disableRawMode();
    CLEAR_SCREEN();
    setColor(14);
    cout << "\n";
    cout << "  +======================================+\n";
    cout << "  |       ACHIEVEMENT UNLOCKED!          |\n";
    cout << "  |                                      |\n";
    int pad = max(0, (int)(36-(int)name.size())/2);
    cout << "  |" << string(pad,' ') << name << string(36-pad-(int)name.size(),' ') << "|\n";
    cout << "  |                                      |\n";
    cout << "  +======================================+\n";
    resetColor();
    PLATFORM_SLEEP(1800);
    CLEAR_SCREEN();
}

void unlockAchievement(const string& name) {
    for (auto& a : achievements)
        if (a.name == name && !a.unlocked) { a.unlocked=true; achievementPopup(a.name); }
}

// ============================================================
// HTTP UTILS (curl)
// ============================================================
string httpPost(const string& url, const string& json, const string& token="") {
    string tmp = "/tmp/vgc_resp.json";
    string cmd = "curl -s -m 8 -X POST -H \"Content-Type: application/json\"";
    if (!token.empty()) cmd += " -H \"Authorization: Bearer "+token+"\"";
    cmd += " -d '"+json+"' \""+url+"\" > "+tmp+" 2>/dev/null";
    system(cmd.c_str());
    ifstream f(tmp);
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}
string httpGet(const string& url, const string& token="") {
    string tmp = "/tmp/vgc_resp.json";
    string cmd = "curl -s -m 8";
    if (!token.empty()) cmd += " -H \"Authorization: Bearer "+token+"\"";
    cmd += " \""+url+"\" > "+tmp+" 2>/dev/null";
    system(cmd.c_str());
    ifstream f(tmp);
    return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
}
string jsonStr(const string& j, const string& key) {
    string s = "\""+key+"\":\"";
    size_t p = j.find(s);
    if (p==string::npos) return "";
    p += s.size();
    size_t e = j.find('"',p);
    return (e==string::npos)?"":j.substr(p,e-p);
}
bool jsonBool(const string& j, const string& key) {
    string s = "\""+key+"\":";
    size_t p = j.find(s);
    if (p==string::npos) return false;
    return j.substr(p+s.size(),4)=="true";
}
int jsonInt(const string& j, const string& key) {
    string s = "\""+key+"\":";
    size_t p = j.find(s);
    if (p==string::npos) return 0;
    p += s.size();
    size_t e = j.find_first_of(",}]",p);
    try { return stoi(j.substr(p,e-p)); } catch(...){ return 0; }
}
bool backendAvailable() {
    return httpGet(g_backendUrl+"/api/ping").find("ok")!=string::npos;
}

// ============================================================
// PROFILO locale + online
// ============================================================
void ensureProfilesDir() {
    if (!fs::exists("profiles")) { error_code ec; fs::create_directory("profiles",ec); }
}
string profilePath(const string& n) { return "profiles/"+n+".dat"; }

bool saveLocalProfile() {
    if (g_playerName.empty()) return false;
    ensureProfilesDir();
    ofstream out(profilePath(g_playerName),ios::binary);
    if (!out) return false;
    const char MAGIC[4]={'V','G','C','3'};
    out.write(MAGIC,4);
    uint32_t len=(uint32_t)g_playerName.size();
    out.write((char*)&len,4); out.write(g_playerName.c_str(),len);
    out.write((char*)&g_migliorTempo,sizeof(int));
    out.write((char*)&g_migliorTentativi,sizeof(int));
    out.write((char*)&g_vittorieConsecutive,sizeof(int));
    out.write((char*)&g_musicOn,sizeof(bool));
    uint32_t ac=(uint32_t)achievements.size();
    out.write((char*)&ac,4);
    for (auto& a:achievements) out.write((char*)&a.unlocked,1);
    out.write((char*)&g_snakeBestScore,sizeof(int));
    out.write((char*)&g_snakeBestDifficulty,sizeof(int));
    out.write((char*)&g_dodgeBestTime,sizeof(int));
    out.write((char*)&g_dodgeBestDifficulty,sizeof(int));
    // Nuovi campi v3
    out.write((char*)&g_language,sizeof(int));
    out.write((char*)&g_volume,sizeof(int));
    // Storico punteggi
    auto writeHist=[&](const vector<int>& h){
        uint32_t hs=(uint32_t)h.size(); out.write((char*)&hs,4);
        for(int v:h) out.write((char*)&v,sizeof(int));
    };
    writeHist(g_snakeHistory);
    writeHist(g_dodgeHistory);
    writeHist(g_indovinaHistory);
    return true;
}
bool loadLocalProfile(const string& name) {
    ifstream in(profilePath(name),ios::binary);
    if (!in) return false;
    char magic[4]; if (!in.read(magic,4)) return false;
    bool isV3=(memcmp(magic,"VGC3",4)==0);
    bool isOld=(memcmp(magic,"VGC2",4)==0||memcmp(magic,"VGC1",4)==0);
    if (!isV3 && !isOld) return false;
    uint32_t len=0; in.read((char*)&len,4);
    g_playerName.resize(len); in.read(&g_playerName[0],len);
    in.read((char*)&g_migliorTempo,sizeof(int));
    in.read((char*)&g_migliorTentativi,sizeof(int));
    in.read((char*)&g_vittorieConsecutive,sizeof(int));
    in.read((char*)&g_musicOn,sizeof(bool));
    uint32_t ac=0; in.read((char*)&ac,4);
    for (uint32_t i=0;i<ac;i++) {
        bool st=false; in.read((char*)&st,1);
        if (i<achievements.size()) achievements[i].unlocked=st;
    }
    in.read((char*)&g_snakeBestScore,sizeof(int));
    in.read((char*)&g_snakeBestDifficulty,sizeof(int));
    in.read((char*)&g_dodgeBestTime,sizeof(int));
    in.read((char*)&g_dodgeBestDifficulty,sizeof(int));
    if (isV3) {
        in.read((char*)&g_language,sizeof(int));
        in.read((char*)&g_volume,sizeof(int));
        auto readHist=[&](vector<int>& h){
            uint32_t hs=0; in.read((char*)&hs,4);
            h.clear(); for(uint32_t i=0;i<hs&&i<10;i++){int v=0;in.read((char*)&v,sizeof(int));h.push_back(v);}
        };
        readHist(g_snakeHistory);
        readHist(g_dodgeHistory);
        readHist(g_indovinaHistory);
    }
    return true;
}
vector<string> listLocalProfiles() {
    vector<string> v;
    if (!fs::exists("profiles")) return v;
    for (auto& e:fs::directory_iterator("profiles"))
        if (e.is_regular_file()&&e.path().extension()==".dat")
            v.push_back(e.path().stem().string());
    return v;
}
string buildProfileJson() {
    string j="{";
    j+="\"playerName\":\""+g_playerName+"\",";
    j+="\"migliorTempo\":"+to_string(g_migliorTempo)+",";
    j+="\"migliorTentativi\":"+to_string(g_migliorTentativi)+",";
    j+="\"vittorieConsecutive\":"+to_string(g_vittorieConsecutive)+",";
    j+="\"musicOn\":"; j+=(g_musicOn?"true":"false"); j+=",";
    j+="\"snakeBestScore\":"+to_string(g_snakeBestScore)+",";
    j+="\"snakeBestDifficulty\":"+to_string(g_snakeBestDifficulty)+",";
    j+="\"dodgeBestTime\":"+to_string(g_dodgeBestTime)+",";
    j+="\"dodgeBestDifficulty\":"+to_string(g_dodgeBestDifficulty)+",";
    j+="\"achievements\":[";
    for (size_t i=0;i<achievements.size();i++) {
        j+=achievements[i].unlocked?"true":"false";
        if (i+1<achievements.size()) j+=",";
    }
    j+="]}";
    return j;
}
bool saveOnlineProfile() {
    if (g_authToken.empty()) return false;
    string resp=httpPost(g_backendUrl+"/api/profile",buildProfileJson(),g_authToken);
    return jsonBool(resp,"success");
}
bool loadOnlineProfile() {
    if (g_authToken.empty()) return false;
    string resp=httpGet(g_backendUrl+"/api/profile",g_authToken);
    if (!jsonBool(resp,"success")) return false;
    size_t pp=resp.find("\"profile\":");
    if (pp==string::npos) return false;
    string pd=resp.substr(pp+10);
    int t=jsonInt(pd,"migliorTempo"); if(t>0&&t<9999) g_migliorTempo=t;
    int te=jsonInt(pd,"migliorTentativi"); if(te>0&&te<9999) g_migliorTentativi=te;
    g_vittorieConsecutive=jsonInt(pd,"vittorieConsecutive");
    g_snakeBestScore=jsonInt(pd,"snakeBestScore");
    g_snakeBestDifficulty=jsonInt(pd,"snakeBestDifficulty");
    g_dodgeBestTime=jsonInt(pd,"dodgeBestTime");
    g_dodgeBestDifficulty=jsonInt(pd,"dodgeBestDifficulty");
    size_t ap=pd.find("\"achievements\":[");
    if (ap!=string::npos) {
        ap+=16;
        for (size_t i=0;i<achievements.size();i++) {
            size_t end=pd.find_first_of(",]",ap);
            if (end==string::npos) break;
            achievements[i].unlocked=(pd.substr(ap,end-ap)=="true");
            ap=end+1;
        }
    }
    return true;
}
void saveProfile() {
    saveLocalProfile();
    if (g_onlineMode) { if(saveOnlineProfile()) unlockAchievement("Cloud Saver"); }
}

// ============================================================
// NOTIFICHE
// ============================================================
void checkNotifications() {
    if (!g_onlineMode||g_authToken.empty()) return;
    string resp=httpGet(g_backendUrl+"/api/notifications",g_authToken);
    g_unreadNotifs=jsonInt(resp,"unread");
}

void showNotifications() {
    disableRawMode();
    CLEAR_SCREEN();
    setColor(11);
    cout<<tr(T_NOTIFS_TITLE)<<"\n\n";
    resetColor();
    if (!g_onlineMode) { cout<<tr(T_ONLINE_ONLY)<<"\n"; waitKey(); return; }
    string resp=httpGet(g_backendUrl+"/api/notifications",g_authToken);
    size_t np=resp.find("\"notifications\":[");
    if (np==string::npos||resp.find("[]")!=string::npos) {
        setColor(8); cout<<"  ---\n"; resetColor();
    } else {
        string nb=resp.substr(np+17);
        size_t pos=0;
        int cnt=0;
        while (pos<nb.size()&&nb[pos]!=']'&&cnt<20) {
            size_t mp=nb.find("\"message\":\"",pos);
            if (mp==string::npos) break;
            mp+=11; size_t me=nb.find('"',mp);
            string m=nb.substr(mp,me-mp);
            size_t tp=nb.find("\"type\":\"",pos);
            string t="info";
            if (tp!=string::npos) { tp+=8; size_t te=nb.find('"',tp); t=nb.substr(tp,te-tp); }
            if      (t=="challenge"||t=="challenge_win") setColor(14);
            else if (t=="friend_request"||t=="friend_accept") setColor(11);
            else if (t=="welcome") setColor(10);
            else setColor(7);
            cout<<"  * "<<m<<"\n";
            resetColor();
            size_t next=nb.find("\"id\":",me);
            if (next==string::npos) break;
            nb=nb.substr(next-1); pos=0; cnt++;
        }
    }
    g_unreadNotifs=0;
    cout<<"\n";
    waitKey();
}

// ============================================================
// EFFETTI VISIVI
// ============================================================
void transitionGlitch() {
    for (int i=0;i<8;i++) {
        CLEAR_SCREEN(); setColor(15);
        for (int y=0;y<HEIGHT;y++) { for(int x=0;x<WIDTH;x++) cout<<(rand()%3==0?(char)(33+rand()%94):' '); cout<<'\n'; }
        PLATFORM_SLEEP(40);
    }
    resetColor(); CLEAR_SCREEN();
}
void transitionCRT() {
    for (int step=0;step<WIDTH/2;step++) {
        CLEAR_SCREEN();
        for (int y=0;y<HEIGHT;y++) { cout<<string(step,' '); for(int x=step;x<WIDTH-step;x++) cout<<' '; cout<<'\n'; }
        PLATFORM_SLEEP(12);
    }
    CLEAR_SCREEN();
    for (int y=0;y<HEIGHT;y++) { gotoXY(WIDTH/2,y); setColor(10); cout<<'|'; }
    PLATFORM_SLEEP(200); resetColor(); CLEAR_SCREEN();
}
void transitionDissolve() {
    for (int step=0;step<HEIGHT;step++) {
        CLEAR_SCREEN(); setColor(2);
        for (int y=0;y<HEIGHT;y++) { for(int x=0;x<WIDTH;x++) cout<<(y>=step?(char)(33+rand()%94):' '); cout<<'\n'; }
        PLATFORM_SLEEP(18);
    }
    resetColor(); CLEAR_SCREEN();
}
void playTransition(int mode, const string& name="") {
    if (mode==4) {
        if(name=="Neo")       { transitionDissolve(); return; }
        if(name=="Trinity")   { transitionGlitch();   return; }
        if(name=="Morpheus")  { transitionCRT();      return; }
        if(name=="Valentino") { int r=rand()%3; if(r==0)transitionGlitch(); else if(r==1)transitionCRT(); else transitionDissolve(); return; }
    }
    if (mode==0) { int r=rand()%3; if(r==0)transitionGlitch(); else if(r==1)transitionCRT(); else transitionDissolve(); return; }
    if (mode==1) transitionGlitch();
    if (mode==2) transitionCRT();
    if (mode==3) transitionDissolve();
}
void matrixRainPro(int ms) {
    int headY[80]={},speed[80]={};
    for(int i=0;i<WIDTH;i++){headY[i]=rand()%HEIGHT;speed[i]=1+rand()%3;}
    uint64_t start=getTickMs(); hideCursor();
    while ((int)(getTickMs()-start)<ms) {
        CLEAR_SCREEN();
        for (int y=0;y<HEIGHT;y++) {
            for (int x=0;x<WIDTH;x++) {
                int h=headY[x];
                if(y==h){setColor(10);cout<<(char)(33+rand()%94);}
                else if(y<h&&y>h-6){setColor(2);cout<<(char)(33+rand()%94);}
                else cout<<' ';
            }
            cout<<'\n';
        }
        for (int i=0;i<WIDTH;i++) { headY[i]+=speed[i]; if(headY[i]-6>HEIGHT+rand()%10){headY[i]=-(rand()%HEIGHT);speed[i]=1+rand()%3;} }
        PLATFORM_SLEEP(70);
    }
    resetColor(); CLEAR_SCREEN(); showCursor();
}
void matrixBgFrame(int headY[], int speed[], int w, int h) {
    for (int x=0;x<w;x++) {
        int hh=headY[x]; bool gl=(rand()%40==0);
        if(hh>=0&&hh<h){gotoXY(x,hh);setColor(gl?15:10);cout<<(char)(33+rand()%94);}
        for(int t=1;t<6;t++){int y=hh-t;if(y>=0&&y<h){gotoXY(x,y);setColor(2);cout<<(char)(33+rand()%94);}}
        int tail=hh-6; if(tail>=0&&tail<h){gotoXY(x,tail);cout<<' ';}
        headY[x]+=speed[x]; if(headY[x]>h+rand()%20){headY[x]=-(rand()%h);speed[x]=1+rand()%3;}
    }
    resetColor();
}

// ============================================================
// ACHIEVEMENTS SCREEN
// ============================================================
void showAchievements() {
    disableRawMode(); CLEAR_SCREEN();
    setColor(10); cout<<tr(T_ACHIEVE_TITLE)<<"\n\n"; resetColor();
    int total=0,unl=0;
    for(auto& a:achievements){total++;if(a.unlocked)unl++;}
    cout<<tr(T_PROFILE)<<": "<<g_playerName<<"  |  "<<unl<<"/"<<total<<" ("<<(total?unl*100/total:0)<<"%)\n\n";
    const char* labels[4]={"[I]","[II]","[III]","[?]"};
    int cols[]={10,14,12,13};
    for (int d=0;d<=3;d++) {
        setColor(cols[d]); cout<<labels[d]<<"\n"; resetColor();
        for (auto& a:achievements) {
            if (a.difficulty!=d) continue;
            if (a.unlocked){setColor(14);cout<<" [v] "<<a.name<<" - "<<a.description<<"\n";}
            else           {setColor(8); cout<<" [ ] "<<a.name<<"\n";}
            resetColor();
        }
        cout<<"\n";
    }
    waitKey();
}

// ============================================================
// LEADERBOARD
// ============================================================
void showLeaderboard() {
    disableRawMode(); CLEAR_SCREEN();
    if (!g_onlineMode) { cout<<"\n"<<tr(T_ONLINE_ONLY)<<"\n"; waitKey(); return; }
    setColor(11); cout<<tr(T_LEADERBOARD_TITLE)<<"\n\n"; resetColor();
    auto printBoard=[&](const string& game, const string& label, const string& scoreKey, const string& unit){
        setColor(14); cout<<"--- TOP 10 "<<label<<" ---\n"; resetColor();
        string resp=httpGet(g_backendUrl+"/api/leaderboard/"+game);
        size_t lp=resp.find("\"leaderboard\":[");
        if (lp==string::npos){setColor(8);cout<<"  "<<tr(T_STATS_NO_DATA)<<"\n";resetColor();cout<<"\n";return;}
        string lb=resp.substr(lp+15);
        int rank=1; size_t p=0;
        while(p<lb.size()&&lb[p]!=']'&&rank<=10) {
            size_t us=lb.find("\"username\":\"",p);
            size_t sc=lb.find("\""+scoreKey+"\":",p);
            if(us==string::npos||sc==string::npos) break;
            us+=12; size_t ue=lb.find('"',us);
            sc+=scoreKey.size()+3; size_t se=lb.find_first_of(",}]",sc);
            if(ue==string::npos||se==string::npos) break;
            string user=lb.substr(us,ue-us);
            string score=lb.substr(sc,se-sc);
            if(rank==1) setColor(14);
            else if(rank==2) setColor(7);
            else if(rank==3) setColor(6);
            else setColor(8);
            cout<<"  "<<rank++<<". "<<user<<" — "<<score<<unit<<"\n";
            resetColor();
            p=se;
        }
        cout<<"\n";
    };
    printBoard("snake",   tr(T_SNAKE),       "score",    " pt");
    printBoard("dodge",   tr(T_DODGE),        "time",     " s");
    printBoard("indovina",tr(T_GUESS_GAME),   "tentativi"," t.");
    waitKey();
}

// ============================================================
// SISTEMA AMICI
// ============================================================
void showFriends() {
    disableRawMode(); CLEAR_SCREEN();
    if (!g_onlineMode) { cout<<"\n"<<tr(T_ONLINE_ONLY)<<"\n"; waitKey(); return; }
    setColor(11); cout<<tr(T_FRIENDS_TITLE)<<"\n\n"; resetColor();

    string resp=httpGet(g_backendUrl+"/api/friends/list",g_authToken);

    size_t pp=resp.find("\"pending\":[");
    if (pp!=string::npos) {
        string pb=resp.substr(pp+11);
        if (pb[0]!=']') {
            setColor(14); cout<<"-- Richieste di amicizia ricevute --\n"; resetColor();
            size_t p=0; int idx=1;
            vector<pair<int,string>> pendings;
            while(p<pb.size()&&pb[p]!=']') {
                size_t ip=pb.find("\"id\":",p);
                size_t up=pb.find("\"username\":\"",p);
                if(ip==string::npos||up==string::npos) break;
                ip+=5; size_t ie=pb.find_first_of(",}",ip);
                up+=12; size_t ue=pb.find('"',up);
                if(ie==string::npos||ue==string::npos) break;
                int fid=stoi(pb.substr(ip,ie-ip));
                string uname=pb.substr(up,ue-up);
                setColor(10); cout<<"  "<<idx<<") "<<uname; resetColor();
                cout<<"  [A / R]\n";
                pendings.push_back({fid,uname});
                p=ue; idx++;
            }
            if (!pendings.empty()) {
                cout<<"\n#: "; int n; if(cin>>n){cin.ignore(9999,'\n');
                    if(n>=1&&n<=(int)pendings.size()) {
                        cout<<"(A) / (R)? "; char c; cin>>c; cin.ignore(9999,'\n');
                        string act=(c=='a'||c=='A')?"accept":"decline";
                        string body="{\"friendship_id\":"+to_string(pendings[n-1].first)+",\"action\":\""+act+"\"}";
                        string r2=httpPost(g_backendUrl+"/api/friends/respond",body,g_authToken);
                        string msg=jsonStr(r2,"message");
                        setColor(jsonBool(r2,"success")?10:12); cout<<msg<<"\n"; resetColor();
                        if(act=="accept") unlockAchievement("Social Gamer");
                        PLATFORM_SLEEP(900);
                    }
                } else cin.ignore(9999,'\n');
                CLEAR_SCREEN(); setColor(11); cout<<tr(T_FRIENDS_TITLE)<<"\n\n"; resetColor();
            }
        }
    }

    size_t fp=resp.find("\"friends\":[");
    setColor(14); cout<<"-- "<<tr(T_FRIENDS)<<" --\n"; resetColor();
    if (fp==string::npos||resp.find("\"friends\":[]")!=string::npos) {
        setColor(8); cout<<"  ---\n"; resetColor();
    } else {
        string fb=resp.substr(fp+11);
        size_t p=0; int idx=1;
        while(p<fb.size()&&fb[p]!=']') {
            size_t up=fb.find("\"username\":\"",p);
            size_t sp=fb.find("\"snakeBestScore\":",p);
            size_t dp=fb.find("\"dodgeBestTime\":",p);
            if(up==string::npos) break;
            up+=12; size_t ue=fb.find('"',up);
            string uname=fb.substr(up,ue-up);
            int ss=0,dt=0;
            if(sp!=string::npos){sp+=17;size_t se=fb.find_first_of(",}",sp);ss=stoi(fb.substr(sp,se-sp));}
            if(dp!=string::npos){dp+=16;size_t de=fb.find_first_of(",}",dp);dt=stoi(fb.substr(dp,de-dp));}
            setColor(11); cout<<"  "<<idx++<<") "<<uname; resetColor();
            setColor(8); cout<<"  [Snake:"<<ss<<"pt | Dodge:"<<dt<<"s]\n"; resetColor();
            p=ue;
        }
    }

    cout<<"\n";
    cout<<"  1) "<<tr(T_ADD_FRIEND)<<"\n";
    cout<<"  2) "<<tr(T_BACK)<<"\n";
    cout<<"\n> "; int c; if(cin>>c){cin.ignore(9999,'\n');}else{cin.clear();cin.ignore(9999,'\n');c=2;}
    if (c==1) {
        cout<<"Username: "; string name; getline(cin,name);
        string body="{\"username\":\""+name+"\"}";
        string r2=httpPost(g_backendUrl+"/api/friends/request",body,g_authToken);
        string msg=jsonStr(r2,"message");
        setColor(jsonBool(r2,"success")?10:12); cout<<msg<<"\n"; resetColor();
        PLATFORM_SLEEP(1000);
    }
}

// ============================================================
// SISTEMA SFIDE
// ============================================================
void showChallenges() {
    disableRawMode(); CLEAR_SCREEN();
    if (!g_onlineMode) { cout<<"\n"<<tr(T_ONLINE_ONLY)<<"\n"; waitKey(); return; }
    setColor(13); cout<<tr(T_CHALLENGES_TITLE)<<"\n\n"; resetColor();

    string resp=httpGet(g_backendUrl+"/api/challenges/list",g_authToken);
    size_t cp=resp.find("\"challenges\":[");
    if (cp==string::npos||resp.find("\"challenges\":[]")!=string::npos) {
        setColor(8); cout<<tr(T_STATS_NO_DATA)<<"\n"; resetColor();
    } else {
        string cb=resp.substr(cp+14);
        size_t p=0; int idx=1;
        vector<pair<int,string>> pendingCh;
        while(p<cb.size()&&cb[p]!=']') {
            size_t ip=cb.find("\"id\":",p);
            size_t gp=cb.find("\"game\":\"",p);
            size_t tp=cb.find("\"target\":",p);
            size_t sp=cb.find("\"status\":\"",p);
            size_t cp2=cb.find("\"challenger\":\"",p);
            size_t dp=cb.find("\"direction\":\"",p);
            if(ip==string::npos||gp==string::npos) break;
            ip+=5; size_t ie=cb.find_first_of(",}",ip);
            gp+=8; size_t ge=cb.find('"',gp);
            tp+=9; size_t te=cb.find_first_of(",}",tp);
            sp+=10; size_t se=cb.find('"',sp);
            cp2+=14; size_t ce=cb.find('"',cp2);
            dp+=13; size_t de=cb.find('"',dp);
            if(ie==string::npos||ge==string::npos||te==string::npos) break;
            int chid=stoi(cb.substr(ip,ie-ip));
            string game=cb.substr(gp,ge-gp);
            int target=stoi(cb.substr(tp,te-tp));
            string status=cb.substr(sp,se-sp);
            string challenger=(ce!=string::npos)?cb.substr(cp2,ce-cp2):"?";
            string dir=(de!=string::npos)?cb.substr(dp,de-dp):"sent";
            string statusColor=(status=="pending"||status=="accepted")?"active":"done";
            if(statusColor=="active") setColor(13); else setColor(8);
            cout<<"  "<<idx<<") ["<<(dir=="sent"?">>":"<<")<<"] ";
            cout<<game<<" target:"<<target;
            if(dir=="received"&&status!="sent") cout<<" ("<<challenger<<")";
            cout<<" ["<<status<<"]\n";
            resetColor();
            if(dir=="received"&&status=="pending") pendingCh.push_back({chid,dir});
            p=se; idx++;
        }
        if (!pendingCh.empty()) {
            cout<<"\n# (0=skip): ";
            int n; if(cin>>n){cin.ignore(9999,'\n');
                if(n>=1&&n<=(int)pendingCh.size()) {
                    cout<<"(A) / (R)? "; char c; cin>>c; cin.ignore(9999,'\n');
                    string act=(c=='a'||c=='A')?"accept":"decline";
                    string body="{\"challenge_id\":"+to_string(pendingCh[n-1].first)+",\"action\":\""+act+"\"}";
                    string r2=httpPost(g_backendUrl+"/api/challenges/respond",body,g_authToken);
                    string msg=jsonStr(r2,"message");
                    setColor(jsonBool(r2,"success")?10:12); cout<<msg<<"\n"; resetColor();
                    PLATFORM_SLEEP(900);
                }
            } else cin.ignore(9999,'\n');
        }
    }

    cout<<"\n";
    cout<<"  1) "<<tr(T_SEND_CHALLENGE)<<"\n";
    cout<<"  2) "<<tr(T_BACK)<<"\n";
    cout<<"\n> "; int c; if(cin>>c){cin.ignore(9999,'\n');}else{cin.clear();cin.ignore(9999,'\n');c=2;}
    if (c==1) {
        cout<<"Username: "; string name; getline(cin,name);
        cout<<"Game (snake/dodge): "; string game; getline(cin,game);
        if(game!="snake"&&game!="dodge"){setColor(12);cout<<"N/A\n";resetColor();waitKey();return;}
        cout<<"Target: "; int target; cin>>target; cin.ignore(9999,'\n');
        string body="{\"username\":\""+name+"\",\"game\":\""+game+"\",\"target\":"+to_string(target)+"}";
        string r2=httpPost(g_backendUrl+"/api/challenges/send",body,g_authToken);
        string msg=jsonStr(r2,"message");
        setColor(jsonBool(r2,"success")?10:12); cout<<msg<<"\n"; resetColor();
        if(jsonBool(r2,"success")) unlockAchievement("Challenger");
        PLATFORM_SLEEP(1000);
    }
}

// ============================================================
// STATISTICHE — Grafico ASCII
// ============================================================
void drawBarChart(const vector<int>& data, const string& unit, int color) {
    if (data.empty()) { setColor(8); cout<<"  "<<tr(T_STATS_NO_DATA)<<"\n"; resetColor(); return; }
    int maxVal=*max_element(data.begin(),data.end());
    if (maxVal==0) maxVal=1;
    const int chartH=8, barW=4;
    // Asse Y + barre
    for (int row=chartH;row>=0;row--) {
        if (row==chartH) { cout<<"    "; setColor(8); cout<<maxVal<<"\n"; resetColor(); continue; }
        if (row==0)      { setColor(8); cout<<"  0 +"; for(int i=0;i<(int)data.size();i++) cout<<string(barW,'-'); cout<<">\n"; resetColor(); break; }
        cout<<"    |";
        for (int i=0;i<(int)data.size();i++) {
            int filled=(int)((double)data[i]*chartH/maxVal);
            if (filled>=row) { setColor(color); cout<<string(barW,'#'); resetColor(); }
            else               cout<<string(barW,' ');
        }
        cout<<"\n";
    }
    // Indici
    setColor(8); cout<<"     ";
    for (int i=0;i<(int)data.size();i++) cout<<" "<<(i+1)<<"  ";
    cout<<"\n";
    // Valori
    cout<<"     ";
    for (int i=0;i<(int)data.size();i++) {
        string v=to_string(data[i])+unit;
        v=v.substr(0,min((int)v.size(),3));
        cout<<v<<string(barW-(int)v.size(),' ')<<" ";
    }
    resetColor(); cout<<"\n";
}

void showStats() {
    disableRawMode(); CLEAR_SCREEN();
    setColor(10); cout<<tr(T_STATS_TITLE)<<"\n\n"; resetColor();

    // Record migliori
    setColor(14); cout<<tr(T_STATS_BEST_RECORDS)<<"\n"; resetColor();
    cout<<"  Snake:  ";
    if(g_snakeBestScore>0){setColor(11);cout<<g_snakeBestScore<<" "<<tr(T_STATS_SCORE);}
    else{setColor(8);cout<<"---";}
    resetColor(); cout<<"\n";
    cout<<"  Dodge:  ";
    if(g_dodgeBestTime>0){setColor(11);cout<<g_dodgeBestTime<<" "<<tr(T_STATS_TIME);}
    else{setColor(8);cout<<"---";}
    resetColor(); cout<<"\n";
    cout<<"  Guess:  ";
    if(g_migliorTentativi<9999){setColor(11);cout<<g_migliorTentativi<<" "<<tr(T_STATS_TRIES);}
    else{setColor(8);cout<<"---";}
    resetColor(); cout<<"\n\n";

    // Grafico Snake
    setColor(10); cout<<tr(T_STATS_SNAKE_HIST)<<"\n"; resetColor();
    drawBarChart(g_snakeHistory,"p",10);
    cout<<"\n";

    // Grafico Dodge
    setColor(13); cout<<tr(T_STATS_DODGE_HIST)<<"\n"; resetColor();
    drawBarChart(g_dodgeHistory,"s",13);
    cout<<"\n";

    // Grafico Indovina
    setColor(14); cout<<tr(T_STATS_INDOVINA_HIST)<<"\n"; resetColor();
    drawBarChart(g_indovinaHistory,"t",14);
    cout<<"\n";

    waitKey();
}

// ============================================================
// CREDITI
// ============================================================
void showCredits() {
    disableRawMode(); CLEAR_SCREEN();
    setColor(10);
    cout<<"\n";
    cout<<"  +============================================+\n";
    cout<<"  |                                            |\n";
    setColor(14);
    cout<<"  |     VALENTINO GAME COLLECTION v2.0        |\n";
    setColor(10);
    cout<<"  |         by The_Meme_Gaimer                |\n";
    cout<<"  |                                            |\n";
    cout<<"  +============================================+\n\n";
    resetColor();

    setColor(11);
    cout<<"  "<<tr(T_CREDITS_FOUNDER)<<"\n";
    resetColor();
    cout<<"  "<<tr(T_CREDITS_ROLE_FOUNDER)<<":  ";
    setColor(14); cout<<"Valentino Ingrao\n"; resetColor();

    cout<<"\n";
    setColor(11);
    cout<<"  "<<tr(T_CREDITS_THANKS)<<":\n";
    resetColor();
    setColor(7);
    cout<<"    - Antonino Ingrao\n";
    cout<<"    - Donatella Martino\n";
    cout<<"    - Desire' Ingrao\n";
    cout<<"    - Riccardo Zanaga\n";
    cout<<"    - Paolo Grandi\n";
    resetColor();

    cout<<"\n";
    setColor(8);
    cout<<"  "<<tr(T_CREDITS_RIGHTS)<<"\n\n";
    resetColor();

    waitKey();
}

// ============================================================
// IMPOSTAZIONI
// ============================================================
void showSettings() {
    while(true) {
        disableRawMode(); CLEAR_SCREEN();
        setColor(10); cout<<tr(T_SETTINGS_TITLE)<<"\n\n"; resetColor();

        const char* langNames[3]={"Italiano","English","Francais"};

        setColor(7);
        cout<<"  1) "<<tr(T_VOLUME)<<":          [";
        int barW=20; int filled=(int)((double)g_volume/100*barW);
        setColor(10); cout<<string(filled,'|'); setColor(8); cout<<string(barW-filled,' ');
        setColor(7); cout<<"] "<<g_volume<<"%\n";

        cout<<"  2) "<<tr(T_LANGUAGE_LABEL)<<":         ["<<langNames[g_language]<<"]\n";
        cout<<"  3) "<<tr(T_PROFILE)<<":\n";
        cout<<"       ";
        setColor(14); cout<<g_playerName; resetColor();
        if(g_onlineMode){setColor(11);cout<<"  ["<<tr(T_ONLINE)<<"]";}
        else            {setColor(8); cout<<"  ["<<tr(T_OFFLINE)<<"]";}
        cout<<"\n";
        setColor(7);
        cout<<"  4) "<<tr(T_CREDITS)<<"\n";
        cout<<"  5) "<<tr(T_SAVE_PROFILE)<<"\n";
        cout<<"  6) "<<tr(T_BACK)<<"\n";
        resetColor();

        cout<<"\n> ";
        int c; if(!(cin>>c)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');

        if (c==1) {
            cout<<tr(T_VOLUME)<<" (0-100): ";
            int v; if(cin>>v){cin.ignore(9999,'\n'); g_volume=max(0,min(100,v));}
            else cin.ignore(9999,'\n');
        } else if (c==2) {
            cout<<"1) Italiano  2) English  3) Francais\n> ";
            int l; if(cin>>l){cin.ignore(9999,'\n');
                if(l>=1&&l<=3){g_language=l-1;setColor(10);cout<<tr(T_LANG_CHANGED)<<"\n";resetColor();PLATFORM_SLEEP(700);}
            } else cin.ignore(9999,'\n');
        } else if (c==3) {
            cout<<"\n  "<<tr(T_PROFILE)<<": "; setColor(14); cout<<g_playerName; resetColor();
            if(g_onlineMode){setColor(11);cout<<"  ["<<tr(T_ONLINE)<<"]";}
            cout<<"\n  Snake: "<<g_snakeBestScore<<"pt  |  Dodge: "<<g_dodgeBestTime<<"s  |  Guess: ";
            if(g_migliorTentativi<9999)cout<<g_migliorTentativi<<"t"; else cout<<"---";
            cout<<"\n\n";
            waitKey();
        } else if (c==4) {
            showCredits();
        } else if (c==5) {
            saveProfile();
            setColor(10); cout<<"\n  "<<tr(T_SAVED)<<"\n"; resetColor();
            PLATFORM_SLEEP(800);
        } else if (c==6) {
            break;
        }
    }
}

// ============================================================
// INSTALLATION WIZARD
// ============================================================
bool isInstalled() { return fs::exists(".vgc_installed"); }
void runInstallWizard() {
    CLEAR_SCREEN();
    setColor(10);
    cout<<"\n";
    cout<<"  ##############################################\n";
    cout<<"  #    VALENTINO GAME COLLECTION v2.0         #\n";
    cout<<"  #          by The_Meme_Gaimer               #\n";
    cout<<"  ##############################################\n\n";
    setColor(7);
    PLATFORM_SLEEP(800);
    struct Step { const char* msg; int ms; };
    Step steps[]={
        {"  >> Verifica requisiti di sistema...",   400},
        {"  >> Creazione cartelle di gioco...",     300},
        {"  >> Installazione core engine...",       500},
        {"  >> Configurazione grafica terminale...",400},
        {"  >> Caricamento sistema achievements...",300},
        {"  >> Configurazione sistema profili...",  350},
        {"  >> Connessione server online...",       600},
        {"  >> Configurazione sistema amici...",    300},
        {"  >> Configurazione sistema sfide...",    300},
        {"  >> Verifica email service...",          350},
        {"  >> Caricamento sistema statistiche...", 300},
        {"  >> Finalizzazione installazione...",    400},
    };
    for (auto& s:steps) {
        setColor(14); slowPrint(string(s.msg)+"\n",10); resetColor();
        cout<<"  [";
        for(int i=0;i<30;i++){cout<<'#'<<flush;PLATFORM_SLEEP(s.ms/30);}
        cout<<"] OK\n";
    }
    PLATFORM_SLEEP(400); CLEAR_SCREEN();
    setColor(10); cout<<"\n  Installazione completata!\n\n"; resetColor();
    cout<<"  Benvenuto in Valentino Game Collection.\n";
    cout<<"  La tua avventura sta per iniziare...\n\n";
    PLATFORM_SLEEP(1200);
    ensureProfilesDir();
    ofstream f(".vgc_installed"); f<<"installed";
}

// ============================================================
// PASSWORD NASCOSTA
// ============================================================
string promptHidden(const string& label) {
    cout<<label;
    string pw;
#ifdef _WIN32
    char ch; while((ch=_getch())!='\r'){if(ch=='\b'){if(!pw.empty()){pw.pop_back();cout<<"\b \b";}}else{pw+=ch;cout<<'*';}} cout<<'\n';
#else
    struct termios oldt; tcgetattr(STDIN_FILENO,&oldt);
    struct termios newt=oldt; newt.c_lflag&=~ECHO;
    tcsetattr(STDIN_FILENO,TCSANOW,&newt);
    getline(cin,pw);
    tcsetattr(STDIN_FILENO,TCSANOW,&oldt);
    cout<<'\n';
#endif
    return pw;
}

// ============================================================
// VERIFICA EMAIL
// ============================================================
bool sendVerificationCode(const string& email) {
    setColor(8); cout<<"Invio codice a "<<email<<"..."; resetColor();
    string body="{\"email\":\""+email+"\"}";
    string resp=httpPost(g_backendUrl+"/api/send-verification",body);
    if (!jsonBool(resp,"success")) {
        string msg=jsonStr(resp,"message");
        setColor(12); cout<<"\nErrore: "<<(msg.empty()?"server non raggiungibile":msg)<<"\n"; resetColor();
        return false;
    }
    setColor(10); cout<<" OK!\n"; resetColor();
    return true;
}

bool doRegister() {
    CLEAR_SCREEN();
    setColor(11); cout<<"=== REGISTRAZIONE ===\n\n"; resetColor();
    cout<<"Username: "; string username; getline(cin,username);
    cout<<"Email: ";    string email;    getline(cin,email);

    if (!sendVerificationCode(email)) { waitKey(); return false; }

    setColor(14);
    cout<<"\nCodice a 6 cifre ricevuto via email:\n";
    resetColor();
    cout<<"Codice: "; string code; getline(cin,code);

    string pw  = promptHidden("Password (min 6): ");
    string pw2 = promptHidden("Conferma password: ");
    if (pw!=pw2) { setColor(12);cout<<"\nPassword non corrispondenti.\n";resetColor();waitKey();return false; }

    string body="{\"username\":\""+username+"\",\"email\":\""+email+"\",\"password\":\""+pw+"\",\"code\":\""+code+"\"}";
    string resp=httpPost(g_backendUrl+"/api/register",body);
    if (!jsonBool(resp,"success")) {
        string msg=jsonStr(resp,"message");
        setColor(12);cout<<"\nErrore: "<<(msg.empty()?"server non raggiungibile":msg)<<"\n";resetColor();waitKey();return false;
    }
    g_authToken=jsonStr(resp,"token");
    g_playerName=username;
    setColor(10);cout<<"\nRegistrazione completata! Benvenuto, "<<g_playerName<<"!\n";resetColor();
    unlockAchievement("Verified");
    PLATFORM_SLEEP(1000);
    return true;
}

bool doLogin() {
    CLEAR_SCREEN();
    setColor(11); cout<<"=== LOGIN ===\n\n"; resetColor();
    cout<<"Email: "; string email; getline(cin,email);
    string pw=promptHidden("Password: ");
    string body="{\"email\":\""+email+"\",\"password\":\""+pw+"\"}";
    string resp=httpPost(g_backendUrl+"/api/login",body);
    if (!jsonBool(resp,"success")) {
        string msg=jsonStr(resp,"message");
        setColor(12);cout<<"\nErrore: "<<(msg.empty()?"impossibile connettersi":msg)<<"\n";resetColor();waitKey();return false;
    }
    g_authToken=jsonStr(resp,"token");
    g_playerName=jsonStr(resp,"username");
    setColor(10);cout<<"\nBenvenuto, "<<g_playerName<<"!\n";resetColor();
    PLATFORM_SLEEP(800);
    unlockAchievement("First Login");
    return true;
}

// ============================================================
// MENU PROFILI OFFLINE
// ============================================================
bool offlineProfileMenu() {
    while(true) {
        CLEAR_SCREEN();
        cout<<"=====================================\n";
        cout<<"         PROFILI OFFLINE\n";
        cout<<"=====================================\n\n";
        cout<<"1) Nuovo profilo\n2) Seleziona profilo\n3) Elimina profilo\n4) "<<tr(T_BACK)<<"\n\n";
        auto profs=listLocalProfiles();
        cout<<"Profili salvati:\n";
        if(profs.empty()) cout<<"  - Nessuno\n\n";
        else for(size_t i=0;i<profs.size();i++) cout<<"  "<<(i+1)<<") "<<profs[i]<<"\n";
        cout<<"\n> "; int c; if(!(cin>>c)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
        if(c==1){
            cout<<"Nome: "; string name; getline(cin,name);
            initAchievements(); g_playerName=name;
            g_migliorTempo=9999;g_migliorTentativi=9999;g_vittorieConsecutive=0;
            g_snakeBestScore=g_snakeBestDifficulty=g_dodgeBestTime=g_dodgeBestDifficulty=0;
            g_snakeHistory.clear(); g_dodgeHistory.clear(); g_indovinaHistory.clear();
            saveLocalProfile(); return true;
        } else if(c==2){
            if(profs.empty()){cout<<"Nessun profilo.\n";waitKey();continue;}
            cout<<"Numero: "; int idx; if(!(cin>>idx)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
            if(idx<1||idx>(int)profs.size()){cout<<"Non valido.\n";waitKey();continue;}
            initAchievements(); if(!loadLocalProfile(profs[idx-1])){cout<<"Errore.\n";waitKey();continue;}
            return true;
        } else if(c==3){
            if(profs.empty()){cout<<"Nessun profilo.\n";waitKey();continue;}
            cout<<"Numero: "; int idx; if(!(cin>>idx)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
            if(idx<1||idx>(int)profs.size()){cout<<"Non valido.\n";waitKey();continue;}
            error_code ec; fs::remove(profilePath(profs[idx-1]),ec);
            cout<<"Eliminato.\n"; waitKey();
        } else if(c==4) return false;
    }
}

// ============================================================
// SCHERMATA ACCESSO
// ============================================================
bool accessScreen() {
    while(true) {
        CLEAR_SCREEN();
        setColor(10);
        cout<<"\n  ##########################################\n";
        cout<<"  #    VALENTINO GAME COLLECTION v2.0     #\n";
        cout<<"  #         by The_Meme_Gaimer            #\n";
        cout<<"  ##########################################\n\n";
        resetColor();
        if (g_onlineMode) {
            setColor(14); cout<<"  [ SERVER ONLINE DISPONIBILE ]\n\n"; resetColor();
            cout<<"  1) Login (email + password)\n";
            cout<<"  2) Registrati (con verifica email)\n";
            cout<<"  3) Gioca offline\n";
            cout<<"  4) "<<tr(T_QUIT)<<"\n";
        } else {
            setColor(8); cout<<"  [ SERVER OFFLINE - modalita' locale ]\n\n"; resetColor();
            cout<<"  1) Seleziona / crea profilo offline\n";
            cout<<"  2) "<<tr(T_QUIT)<<"\n";
        }
        cout<<"\n  > "; int c; if(!(cin>>c)){cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
        if (g_onlineMode) {
            if(c==1){if(doLogin()){g_onlineMode=true;loadOnlineProfile();checkNotifications();return true;}}
            else if(c==2){if(doRegister()){g_onlineMode=true;return true;}}
            else if(c==3){g_onlineMode=false;if(offlineProfileMenu())return true;}
            else if(c==4) return false;
        } else {
            if(c==1){if(offlineProfileMenu())return true;}
            else if(c==2) return false;
        }
    }
}

// ============================================================
// GIOCHI
// ============================================================
void gameBootScreen(const string& title) {
    disableRawMode(); CLEAR_SCREEN();
    setColor(11); cout<<"[ "<<title<<" SYSTEM BOOTING ]\n"; resetColor();
    PLATFORM_SLEEP(200);
    slowPrint("...\n",15); PLATFORM_SLEEP(300);
}
string difficultyName(int d) {
    if(d==1) return tr(T_EASY);
    if(d==2) return tr(T_MEDIUM);
    if(d==3) return tr(T_HARD);
    return "-";
}

void miniGiocoIndovina(const string& nome, int diff) {
    disableRawMode(); playTransition(0,nome); CLEAR_SCREEN();
    setColor(11); cout<<"=== "<<tr(T_GUESS_GAME)<<" ===\n\n"; resetColor();
    int maxRange=(diff==1)?20:(diff==2)?50:100;
    int segreto=rand()%maxRange+1, tentativi=0;
    time_t startTime=time(nullptr);
    cout<<"1 - "<<maxRange<<"?\n\n";
    int t=0;
    do {
        cout<<"# "; if(!(cin>>t)){cout<<"N/A\n";cin.clear();cin.ignore(9999,'\n');continue;} cin.ignore(9999,'\n');
        tentativi++;
        if(t<1||t>maxRange){cout<<"OOB!\n";continue;}
        if(t>segreto){setColor(12);cout<<"TOO HIGH\n";resetColor();}
        else if(t<segreto){setColor(14);cout<<"TOO LOW\n";resetColor();}
        else {
            int durata=(int)(time(nullptr)-startTime);
            setColor(10); cout<<"\nGOT IT in "<<tentativi<<" t., "<<durata<<"s!\n"; resetColor();
            if(durata<g_migliorTempo){g_migliorTempo=durata;setColor(14);cout<<"Record!\n";resetColor();}
            if(tentativi<g_migliorTentativi){g_migliorTentativi=tentativi;setColor(14);cout<<"Record!\n";resetColor();}
            if(durata<5)     unlockAchievement("Speedrunner");
            if(tentativi==1) unlockAchievement("Perfect Guess");
            if(diff==3)      unlockAchievement("Hardcore Mode");
            g_vittorieConsecutive++;
            if(g_vittorieConsecutive>=3) unlockAchievement("Persistence");
            addToHistory(g_indovinaHistory, tentativi);
            saveProfile();
        }
    } while(t!=segreto);
    waitKey(); playTransition(0,nome); CLEAR_SCREEN();
}

struct SnakeSeg { int x,y; };
void giocoSnake(const string& nome, int diff) {
    gameBootScreen("SNAKE"); enableRawMode(); hideCursor();
    const int W=30,H=20;
    int delayMs=(diff==1)?150:(diff==2)?100:60;
    vector<SnakeSeg> snake; snake.push_back({W/2,H/2});
    int dirX=1,dirY=0,foodX=rand()%W,foodY=rand()%H,score=0;
    bool over=false;
    while(!over) {
        int key=platformReadKey();
        if(key==KEY_UP_CODE   ||key=='w'||key=='W'){if(dirY!=1) {dirX=0;dirY=-1;}}
        if(key==KEY_DOWN_CODE ||key=='s'||key=='S'){if(dirY!=-1){dirX=0;dirY=1; }}
        if(key==KEY_LEFT_CODE ||key=='a'||key=='A'){if(dirX!=1) {dirX=-1;dirY=0;}}
        if(key==KEY_RIGHT_CODE||key=='d'||key=='D'){if(dirX!=-1){dirX=1; dirY=0;}}
        if(key=='q'||key=='Q') break;
        SnakeSeg nh={snake[0].x+dirX,snake[0].y+dirY};
        if(nh.x<0||nh.x>=W||nh.y<0||nh.y>=H) over=true;
        for(auto& s:snake) if(s.x==nh.x&&s.y==nh.y){over=true;break;}
        if(over) break;
        snake.insert(snake.begin(),nh);
        if(nh.x==foodX&&nh.y==foodY){
            score++;
            bool v=false;
            while(!v){foodX=rand()%W;foodY=rand()%H;v=true;for(auto& s:snake)if(s.x==foodX&&s.y==foodY){v=false;break;}}
        } else snake.pop_back();
        CLEAR_SCREEN();
        setColor(10);cout<<"=== SNAKE ===  "<<nome<<"  "<<tr(T_STATS_SCORE)<<":"<<score<<"  [Q]\n";resetColor();
        for(int y=-1;y<=H;y++){
            for(int x=-1;x<=W;x++){
                if(y==-1||y==H||x==-1||x==W){cout<<'#';continue;}
                bool pr=false;
                if(x==foodX&&y==foodY){setColor(12);cout<<'*';setColor(7);pr=true;}
                else for(size_t i=0;i<snake.size();i++) if(snake[i].x==x&&snake[i].y==y){setColor(i==0?10:2);cout<<(i==0?'O':'o');resetColor();pr=true;break;}
                if(!pr) cout<<' ';
            }
            cout<<'\n';
        }
        PLATFORM_SLEEP(delayMs);
    }
    showCursor(); disableRawMode(); CLEAR_SCREEN();
    setColor(12);cout<<"GAME OVER!\n\n";resetColor();
    cout<<tr(T_STATS_SCORE)<<": "<<score<<"\n";
    if(score>g_snakeBestScore){g_snakeBestScore=score;g_snakeBestDifficulty=diff;setColor(14);cout<<"Record!\n";resetColor();}
    else cout<<"Best: "<<g_snakeBestScore<<" ("<<difficultyName(g_snakeBestDifficulty)<<")\n";
    if(score>=10) unlockAchievement("Snake Rookie");
    if(score>=30) unlockAchievement("Snake Survivor");
    if(score>=50) unlockAchievement("Snake Master");
    addToHistory(g_snakeHistory, score);
    saveProfile(); waitKey();
}

struct Obstacle { int x,y; };
void giocoDodge(const string& nome, int diff) {
    gameBootScreen("DODGE GAME"); enableRawMode(); hideCursor();
    const int W=30,H=20;
    int delayMs=(diff==1)?120:(diff==2)?80:50;
    int pX=W/2,pY=H-2; vector<Obstacle> obs; bool over=false;
    uint64_t startT=getTickMs(); int survived=0;
    while(!over) {
        int key=platformReadKey();
        if(key==KEY_LEFT_CODE ||key=='a'||key=='A'){pX--;if(pX<0)pX=0;}
        if(key==KEY_RIGHT_CODE||key=='d'||key=='D'){pX++;if(pX>=W)pX=W-1;}
        if(key=='q'||key=='Q') break;
        int spawnChance=(diff==1)?12:(diff==2)?8:5;
        if(rand()%spawnChance==0) obs.push_back({rand()%W,0});
        for(auto& o:obs) o.y++;
        vector<Obstacle> nobs;
        for(auto& o:obs){if(o.y==pY&&o.x==pX)over=true;if(o.y<H)nobs.push_back(o);}
        obs=nobs;
        survived=(int)((getTickMs()-startT)/1000);
        CLEAR_SCREEN();
        setColor(10);cout<<"=== DODGE ===  "<<nome<<"  "<<tr(T_STATS_TIME)<<":"<<survived<<"s  [Q]\n";resetColor();
        for(int y=0;y<H;y++){
            for(int x=0;x<W;x++){
                bool pr=false;
                if(y==pY&&x==pX){setColor(14);cout<<'^';resetColor();pr=true;}
                else for(auto& o:obs) if(o.x==x&&o.y==y){setColor(12);cout<<'*';resetColor();pr=true;break;}
                if(!pr) cout<<' ';
            }
            cout<<'\n';
        }
        PLATFORM_SLEEP(delayMs);
    }
    showCursor(); disableRawMode(); CLEAR_SCREEN();
    if(over){setColor(12);cout<<"HIT!\n\n";resetColor();}
    cout<<tr(T_STATS_TIME)<<": "<<survived<<"s\n";
    if(survived>g_dodgeBestTime){g_dodgeBestTime=survived;g_dodgeBestDifficulty=diff;setColor(14);cout<<"Record!\n";resetColor();}
    else cout<<"Best: "<<g_dodgeBestTime<<"s ("<<difficultyName(g_dodgeBestDifficulty)<<")\n";
    if(survived>=20) unlockAchievement("Dodger");
    if(survived>=40) unlockAchievement("Ultra Dodger");
    if(survived>=60) unlockAchievement("Untouchable");
    addToHistory(g_dodgeHistory, survived);
    saveProfile(); waitKey();
}

int scegliDifficolta() {
    enableRawMode(); hideCursor();
    int sel=0;
    string opts[]={tr(T_EASY),tr(T_MEDIUM),tr(T_HARD),tr(T_BACK)};
    while(true) {
        CLEAR_SCREEN(); setColor(10);cout<<"=== "<<tr(T_DIFFICULTY)<<" ===\n\n";resetColor();
        for(int i=0;i<4;i++){if(i==sel){setColor(10);cout<<"> "<<opts[i]<<"\n";resetColor();}else{setColor(7);cout<<"  "<<opts[i]<<"\n";resetColor();}}
        int key=platformReadKey(); PLATFORM_SLEEP(80);
        if(key==KEY_UP_CODE  ||key=='w'||key=='W'){sel--;if(sel<0)sel=3;}
        else if(key==KEY_DOWN_CODE||key=='s'||key=='S'){sel++;if(sel>3)sel=0;}
        else if(key=='\n'||key=='\r'||key==13){disableRawMode();showCursor();return sel+1;}
    }
}

// ============================================================
// MENU PRINCIPALE
// ============================================================
int matrixMenu(const string& nome) {
    enableRawMode(); hideCursor();
    int headY[80]={},speed[80]={};
    for(int i=0;i<WIDTH;i++){headY[i]=rand()%HEIGHT;speed[i]=1+rand()%3;}
    string secretInput,codeInput;
    int sel=0;
    const int N=11;
    string opts[N];
    auto refreshOpts=[&](){
        opts[0]  = tr(T_GUESS_GAME);
        opts[1]  = tr(T_SNAKE);
        opts[2]  = tr(T_DODGE);
        opts[3]  = tr(T_ACHIEVEMENTS);
        opts[4]  = tr(T_LEADERBOARD);
        opts[5]  = tr(T_FRIENDS);
        opts[6]  = tr(T_CHALLENGES);
        opts[7]  = tr(T_NOTIFICATIONS);
        opts[8]  = tr(T_STATS);
        opts[9]  = tr(T_SETTINGS);
        opts[10] = tr(T_QUIT);
    };
    refreshOpts();
    CLEAR_SCREEN();
    while(true) {
        matrixBgFrame(headY,speed,WIDTH,HEIGHT);

        gotoXY(0,1);
        setColor(10); cout<<"  ====[ VALENTINO GAME COLLECTION v2.0 ]===="; resetColor();
        gotoXY(0,2);
        setColor(8); cout<<"                by The_Meme_Gaimer          "; resetColor();
        gotoXY(0,3);
        cout<<"  "; setColor(7); cout<<tr(T_PROFILE)<<": "; setColor(14); cout<<nome; resetColor();
        if(g_onlineMode){setColor(11);cout<<"  ["<<tr(T_ONLINE)<<"]";resetColor();}
        else            {setColor(8); cout<<"  ["<<tr(T_OFFLINE)<<"]";resetColor();}

        if(g_unreadNotifs>0){
            gotoXY(0,4);
            setColor(12); cout<<"  ["<<g_unreadNotifs<<" notif.]"; resetColor();
        }

        int statRow=5;
        if(g_snakeBestScore>0)     {gotoXY(0,statRow++);setColor(8);cout<<"  Snake: ";setColor(14);cout<<g_snakeBestScore<<"pt     ";resetColor();}
        if(g_dodgeBestTime>0)      {gotoXY(0,statRow++);setColor(8);cout<<"  Dodge: ";setColor(14);cout<<g_dodgeBestTime<<"s      ";resetColor();}

        for(int i=0;i<N;i++){
            gotoXY(4,9+i);
            if(i==sel){setColor(10);cout<<"> "<<opts[i]<<"          ";resetColor();}
            else      {setColor(7); cout<<"  "<<opts[i]<<"          ";resetColor();}
        }
        gotoXY(0,9+N+1);
        setColor(8); cout<<"  "<<tr(T_NAVIGATE)<<"  |  "<<tr(T_SELECT)<<"  "; resetColor();

        gotoXY(0,HEIGHT-1);
        setColor(8); cout<<"  VGC v2.0 (C) TMG Studio 2026              "; resetColor();

        int key=platformReadKey(); PLATFORM_SLEEP(60);
        if(key==KEY_UP_CODE  ||key=='w'||key=='W'){sel--;if(sel<0)sel=N-1;}
        else if(key==KEY_DOWN_CODE||key=='s'||key=='S'){sel++;if(sel>=N)sel=0;}
        else if(key=='\n'||key=='\r'||key==13){
            disableRawMode();showCursor();
            playTransition(4,nome);CLEAR_SCREEN();
            return sel+1;
        }
        else if(key=='r'||key=='R') unlockAchievement("Red Awakening");
        else if(key=='b'||key=='B') unlockAchievement("Blue Dream");

        if(key>0&&key<128){
            secretInput+=(char)key; codeInput+=(char)key;
            if(secretInput.size()>40) secretInput.erase(0,secretInput.size()-40);
            if(codeInput.size()>10)   codeInput.erase(0,codeInput.size()-10);
        }
        if(secretInput.find("opensesame")!=string::npos){unlockAchievement("Secret Door");secretInput.clear();}
        if(codeInput=="404"){unlockAchievement("System Error");codeInput.clear();}

        // Aggiorna etichette se la lingua e' cambiata nelle impostazioni
        refreshOpts();
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    srand((unsigned)time(nullptr));
#ifndef _WIN32
    atexit([](){disableRawMode();showCursor();printf("\033[0m\n");});
#endif

    ensureProfilesDir();
    initAchievements();

    if (!isInstalled()) runInstallWizard();

    setColor(8); cout<<"\nConnessione al server..."; fflush(stdout);
    g_onlineMode=backendAvailable();
    if(g_onlineMode){setColor(10);cout<<" OK\n";}
    else            {setColor(8); cout<<" Non disponibile (offline)\n";}
    resetColor(); PLATFORM_SLEEP(600);

    if (!accessScreen()) { cout<<"\nArrivederci!\n"; return 0; }

    // Easter egg nomi
    if(g_playerName=="Neo")       unlockAchievement("THE ONE");
    if(g_playerName=="Trinity")   unlockAchievement("Connection Est.");
    if(g_playerName=="Morpheus")  unlockAchievement("Free Your Mind");
    if(g_playerName=="Valentino") unlockAchievement("Dev God Mode");
    if(g_playerName=="Architect") unlockAchievement("The Architect");

    // Intro
    CLEAR_SCREEN();
    setColor(10); slowPrint("\n  ==========================================\n",4);
    slowPrint("      VALENTINO GAME COLLECTION v2.0\n",35);
    setColor(8); slowPrint("             by The_Meme_Gaimer\n",25); resetColor();
    setColor(10); slowPrint("  ==========================================\n\n",4);
    slowPrint("  Loading",35);
    for(int i=0;i<5;i++){cout<<'.';fflush(stdout);PLATFORM_SLEEP(300);}
    cout<<"\n\n";

    playTransition(0,g_playerName);
    matrixRainPro(2200);

    int scelta=0;
    do {
        scelta=matrixMenu(g_playerName);
        if     (scelta==1)  {int d=scegliDifficolta();if(d<=3)miniGiocoIndovina(g_playerName,d);}
        else if(scelta==2)  {int d=scegliDifficolta();if(d<=3)giocoSnake(g_playerName,d);}
        else if(scelta==3)  {int d=scegliDifficolta();if(d<=3)giocoDodge(g_playerName,d);}
        else if(scelta==4)  {showAchievements();}
        else if(scelta==5)  {showLeaderboard();}
        else if(scelta==6)  {showFriends();}
        else if(scelta==7)  {showChallenges();}
        else if(scelta==8)  {showNotifications();}
        else if(scelta==9)  {showStats();}
        else if(scelta==10) {showSettings();}
    } while(scelta!=11);

    saveProfile();
    CLEAR_SCREEN();
    setColor(10); cout<<"\n  Progresso salvato. Arrivederci!\n";
    setColor(8);  cout<<"  (C) TMG Studio 2026 - The_Meme_Gaimer\n\n"; resetColor();
    return 0;
}
