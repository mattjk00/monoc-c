#include "include/winutil.h"
#include <windows.h>

void hideConsole() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow( hWnd, SW_HIDE );
}