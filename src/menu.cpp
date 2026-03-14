#include <iostream>
#include <cstdlib>   // per rand() e srand()
#include <ctime>     // per time()
using namespace std;

int main() {
    int scelta = 0;

    // inizializza il generatore di numeri casuali
    srand(time(0));

    do {
        cout << "\n=== MENU PRINCIPALE ===\n";
        cout << "1) Saluta\n";
        cout << "2) Mostra un numero\n";
        cout << "3) Esci\n";
        cout << "4) Mini-gioco: Indovina il numero\n";
        cout << "Scegli un'opzione: ";
        cin >> scelta;

        switch (scelta) {
            case 1:
                cout << "\nCiao Valentino! Grande che stai imparando il C++!\n";
                break;

            case 2:
                cout << "\nIl numero magico di oggi è: 42\n";
                break;

            case 3:
                cout << "\nUscita dal programma...\n";
                break;

            case 4: {
                cout << "\n=== INDOVINA IL NUMERO ===\n";
                int numeroSegreto = rand() % 50 + 1; // numero tra 1 e 50
                int tentativo = 0;
                int tentativi = 0;

                cout << "Ho scelto un numero tra 1 e 50. Prova a indovinarlo!\n";

                do {
                    cout << "Il tuo tentativo: ";
                    cin >> tentativo;
                    tentativi++;

                    if (tentativo > numeroSegreto)
                        cout << "Troppo alto!\n";
                    else if (tentativo < numeroSegreto)
                        cout << "Troppo basso!\n";
                    else
                        cout << "Bravo Valentino! Hai indovinato in " << tentativi << " tentativi!\n";

                } while (tentativo != numeroSegreto);

                break;
            }

            default:
                cout << "\nScelta non valida. Riprova.\n";
                break;
        }

    } while (scelta != 3);

    return 0;
}
