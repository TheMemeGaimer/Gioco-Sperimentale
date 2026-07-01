#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << "╔══════════════════════════════════════════════════════════════╗" << endl;
    cout << "║           GIOCO-SPERIMENTALE - Valentino Game Collection     ║" << endl;
    cout << "╚══════════════════════════════════════════════════════════════╝" << endl;
    cout << endl;
    cout << "  This project is a Windows-only terminal game suite." << endl;
    cout << "  It relies on Windows-specific APIs (windows.h, mmsystem.h)" << endl;
    cout << "  that are not available on Linux/Replit." << endl;
    cout << endl;
    cout << "  The full game source is located in: src/menu.cpp" << endl;
    cout << "  To run the game, compile and execute on a Windows machine" << endl;
    cout << "  using a C++ compiler such as MinGW (g++) or MSVC." << endl;
    cout << endl;
    cout << "  Games included:" << endl;
    cout << "    - Snake (giocoSnake)" << endl;
    cout << "    - Dodge (giocoDodge)" << endl;
    cout << "    - Guessing Game (miniGiocoIndovina)" << endl;
    cout << endl;
    cout << "  Features:" << endl;
    cout << "    - Matrix rain visual effects" << endl;
    cout << "    - Profile system with binary serialization" << endl;
    cout << "    - Achievement system" << endl;
    cout << "    - CRT glitch transitions" << endl;
    cout << endl;
    return 0;
}
