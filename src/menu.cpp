#include <iostream>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include <string>

using namespace std;

const int WIDTH = 80;
const int HEIGHT = 25;

int g_speedBoost = 0; // per Red/Blue pill

// Cambia colore al testo
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Stampa lenta (effetto macchina da scrivere)
void slowPrint(string text, int delay = 30) {
    for (char c : text) {
        cout << c;
        Sleep(delay);
    }
}

// Testo lampeggiante
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

// Schermata iniziale stilosa
void splashScreen() {
    setColor(11);
    slowPrint("\n=====================================\n", 5);
    Sleep(150);

    setColor(10); // verde Matrix
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

// Matrix Rain PRO per intro
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
                    setColor(10); // testa luminosa
                    char c = 33 + rand() % 94;
                    cout << c;
                } else if (y < h && y > h - 6) {
                    setColor(2); // scia
                    char c = 33 + rand() % 94;
                    cout << c;
                } else {
                    cout << " ";
                }
            }
            cout << "\n";
        }

        for (int i = 0; i < cols; i++) {
            headY[i] += speed[i];
            if (headY[i] - 6 > HEIGHT + rand() % 10) {
                headY[i] = - (rand() % HEIGHT);
                speed[i] = 1 + rand() % 3;
            }
        }

        Sleep(70);
    }

    setColor(7);
    system("cls");
}

// Posiziona il cursore
void gotoXY(int x, int y) {
    COORD c;
    c.X = x;
    c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// Nasconde il cursore
void hideCursor() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(out, &cursorInfo);
}

// Motore Matrix sfondo con glitch (un frame)
void matrixBackgroundFrame(int width, int height, int headY[], int speed[]) {
    for (int x = 0; x < width; x++) {

        bool glitch = (rand() % 40 == 0);

        int h = headY[x];

        // testa luminosa
        if (h >= 0 && h < height) {
            gotoXY(x, h);
            setColor(glitch ? 15 : 10);
            cout << (char)(33 + rand() % 94);
        }

        // scia
        for (int t = 1; t < 6; t++) {
            int y = h - t;
            if (y >= 0 && y < height) {
                gotoXY(x, y);
                setColor(2);
                cout << (char)(33 + rand() % 94);
            }
        }

        // cancella coda
        int tail = h - 6;
        if (tail >= 0 && tail < height) {
            gotoXY(x, tail);
            cout << " ";
        }

        // aggiorna posizione
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

// Menu Matrix con sfondo animato + glitch + Easter Egg
int matrixMenu(const string& nomeUtente,
               int migliorTempo,
               int migliorTentativi,
               bool easterNeo,
               bool easterTrinity,
               bool easterMorpheus,
               bool easterValentino,
               bool easterArchitect) {
    const int numOptions = 4;
    string options[numOptions] = {
        "Saluta",
        "Mostra un numero",
        "Esci",
        "Mini-gioco: Indovina il numero"
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
        // sfondo animato
        matrixBackgroundFrame(WIDTH, HEIGHT, headY, speed);

        // piccolo glitch anche sul menu
        bool menuGlitch = (rand() % 50 == 0);

        int panelX = 18;
        int panelY = 3;

        // pulisci pannello
        setColor(0);
        for (int y = 0; y < 14; y++) {
            gotoXY(panelX, panelY + y);
            cout << "                              ";
        }

        // Easter Egg: Dev Mode (Valentino)
        if (easterValentino) {
            gotoXY(panelX, panelY - 2);
            setColor(14);
            cout << "[DEV GOD MODE ENABLED]        ";
        }

        // Easter Egg: Neo
        if (easterNeo) {
            gotoXY(panelX, panelY - 1);
            setColor(15);
            cout << "      THE ONE HAS LOGGED IN      ";
        }

        // Easter Egg: Trinity
        if (easterTrinity) {
            gotoXY(panelX, panelY - 1);
            setColor(11);
            cout << "Connection established, Trinity. ";
        }

        // Easter Egg: Architect
        if (easterArchitect) {
            gotoXY(panelX - 8, panelY - 1);
            setColor(15);
            cout << "Welcome, Architect.                         ";
        }

        // bordo
        setColor(menuGlitch ? 15 : 10);
        gotoXY(panelX, panelY);
        cout << "==============================";
        gotoXY(panelX, panelY + 1);
        cout << "      VALENTINO CONSOLE      ";
        gotoXY(panelX, panelY + 2);
        cout << "==============================";

        // info utente
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

        // opzioni
        for (int i = 0; i < numOptions; i++) {
            gotoXY(panelX + 2, panelY + 10 + i);
            if (i == selected) {
                setColor(menuGlitch ? 15 : 10);
                cout << "> [" << options[i] << "]           ";
            } else {
                setColor(7);
                cout << "  " << options[i] << "              ";
            }
        }

        // Easter Egg: Morpheus frasi random
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

        // Easter Egg: glitch event random
        if (rand() % 200 == 0) {
            gotoXY(10, 12);
            setColor(15);
            cout << "SYSTEM BREACH DETECTED      ";
        }

        // Easter Egg: cat déjà-vu
        if (GetAsyncKeyState('C') & 0x8000) {
            catCounter++;
            Sleep(150);
        }
        if (catCounter >= 2) {
            gotoXY(5, HEIGHT - 3);
            setColor(14);
            cout << "=^.^=  (déjà-vu?)           ";
            catCounter = 0;
        }

        // Easter Egg: Red / Blue pill + glitch boost
        if (GetAsyncKeyState('R') & 0x8000) {
            g_speedBoost = 2;
            gotoXY(0, 0);
            setColor(12);
            cout << "RED PILL: MATRIX FULL POWER        ";
        }
        if (GetAsyncKeyState('B') & 0x8000) {
            g_speedBoost = 0;
            gotoXY(0, 0);
            setColor(9);
            cout << "BLUE PILL: NORMAL MODE             ";
        }
        if (GetAsyncKeyState('G') & 0x8000) {
            gotoXY(0, 1);
            setColor(15);
            cout << "!!! SYSTEM GLITCH BOOST !!!        ";
        }

        // Easter Egg: open sesame (stanza segreta)
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
            system("cls");
            setColor(10);
            cout << "ACCESSO ALLA STANZA SEGRETA...\n\n";
            Sleep(1000);
            cout << "Qui potresti mettere un mini-gioco segreto,\n";
            cout << "un messaggio cifrato o un codice da decifrare.\n\n";
            system("pause");
            secretInput.clear();
        }

        // Easter Egg: 404 -> mini-gioco nascosto
        if (GetAsyncKeyState('4') & 0x8000) { codeInput += '4'; Sleep(120); }
        if (GetAsyncKeyState('0') & 0x8000) { codeInput += '0'; Sleep(120); }

        if (codeInput.size() > 10)
            codeInput.erase(0, codeInput.size() - 10);

        if (codeInput == "404") {
            system("cls");
            setColor(12);
            cout << "ERROR HANDLER MODE\n\n";
            cout << "Correggi gli errori prima che esplodano! (placeholder)\n\n";
            system("pause");
            codeInput.clear();
        }

        Beep(200, 10);
        Sleep(80);

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
        } else if ((down & 0x8000) || (sKey & 0x8000)) {
            selected++;
            if (selected >= numOptions) selected = 0;
            Beep(500, 60);
            Sleep(150);
        } else if (enterKey & 0x8000) {
            Beep(900, 120);
            Sleep(150);
            system("cls");
            return selected + 1;
        }
    }

    return 3;
}

int main() {
    srand(time(0));

    splashScreen();
    matrixRainPro(3000);

    string nomeUtente;
    cout << "Inserisci il tuo nome: ";
    cin >> nomeUtente;
    cout << "\nBenvenuto " << nomeUtente << "!\n\n";

    // Easter Egg flags
    bool easterNeo = false;
    bool easterTrinity = false;
    bool easterMorpheus = false;
    bool easterValentino = false;
    bool easterArchitect = false;

    if (nomeUtente == "Neo") easterNeo = true;
    if (nomeUtente == "Trinity") easterTrinity = true;
    if (nomeUtente == "Morpheus") easterMorpheus = true;
    if (nomeUtente == "Valentino") easterValentino = true;
    if (nomeUtente == "Architect") easterArchitect = true;

    int migliorTempo = 9999;
    int migliorTentativi = 9999;

    int scelta = 0;

    do {
        scelta = matrixMenu(nomeUtente,
                            migliorTempo,
                            migliorTentativi,
                            easterNeo,
                            easterTrinity,
                            easterMorpheus,
                            easterValentino,
                            easterArchitect);

        switch (scelta) {

            case 1:
                setColor(10);
                slowPrint("\nCiao " + nomeUtente + "! Grande che stai imparando il C++!\n", 30);
                setColor(7);
                system("pause");
                break;

            case 2:
                setColor(14);
                slowPrint("\nIl numero magico di oggi è: 42\n", 30);
                setColor(7);
                system("pause");
                break;

            case 3:
                setColor(12);
                slowPrint("\nUscita dal programma...\n", 30);
                setColor(7);
                break;

            case 4: {
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

                int difficolta;
                int maxRange;

                cout << "Scegli la difficolta:\n";
                setColor(10); cout << "1) Facile (1-20)\n"; setColor(7);
                setColor(14); cout << "2) Medio (1-50)\n"; setColor(7);
                setColor(12); cout << "3) Difficile (1-100)\n"; setColor(7);
                cout << "Scelta: ";
                cin >> difficolta;

                switch (difficolta) {
                    case 1: maxRange = 20; break;
                    case 2: maxRange = 50; break;
                    case 3: maxRange = 100; break;
                    default:
                        setColor(12);
                        cout << "Scelta non valida, imposto difficolta media.\n";
                        Beep(400, 300);
                        setColor(7);
                        maxRange = 50;
                        break;
                }

                int numeroSegreto = rand() % maxRange + 1;
                int tentativo = 0;
                int tentativi = 0;

                time_t startTime = time(nullptr);

                cout << "\nHo scelto un numero tra 1 e " << maxRange << ". Prova a indovinarlo!\n";

                do {
                    cout << "Il tuo tentativo: ";
                    cin >> tentativo;
                    tentativi++;

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

                        if (durata < migliorTempo) {
                            migliorTempo = durata;
                            setColor(14);
                            slowPrint("\nNuovo record di tempo! ", 40);
                            cout << "(" << durata << " secondi)\n";
                            nuovoRecord = true;
                        }

                        if (tentativi < migliorTentativi) {
                            migliorTentativi = tentativi;
                            setColor(14);
                            slowPrint("Nuovo record di tentativi! ", 40);
                            cout << "(" << tentativi << " tentativi)\n";
                            nuovoRecord = true;
                        }

                        if (!nuovoRecord) {
                            setColor(11);
                            slowPrint("\nNon hai battuto nessun record stavolta... ma ci sei andato vicino!\n", 30);
                        }

                        setColor(7);
                    }

                } while (tentativo != numeroSegreto);

                system("pause");
                break;
            }

            default:
                setColor(12);
                cout << "\nScelta non valida. Riprova.\n";
                Beep(400, 250);
                setColor(7);
                system("pause");
                break;
        }

    } while (scelta != 3);

    return 0;
}
