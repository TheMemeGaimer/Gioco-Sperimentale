#include <iostream>
using namespace std;

int main() {
    int scelta = 0;

    do {
        cout << "\n=== MENU PRINCIPALE ===\n";
        cout << "1) Saluta\n";
        cout << "2) Mostra un numero\n";
        cout << "3) Esci\n";
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

            default:
                cout << "\nScelta non valida. Riprova.\n";
                break;
        }

    } while (scelta != 3);

    return 0;
}
