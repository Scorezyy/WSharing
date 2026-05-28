#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    // Check for --minimized flag (set by autostart registry entry)
    bool startMinimized = (strstr(lpCmdLine, "--minimized") != nullptr);

    // Prevent multiple instances
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"WSharing_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 0;
    }

    App app;
    if (!app.init(hInstance, startMinimized)) {
        MessageBoxW(nullptr,
                    L"Initialisierung fehlgeschlagen.\n"
                    L"Bitte DirectX 11 und Windows 10+ prüfen.",
                    L"WSharing – Fehler",
                    MB_ICONERROR | MB_OK);
        CloseHandle(mutex);
        return 1;
    }

    app.run();

    CloseHandle(mutex);
    return 0;
}
