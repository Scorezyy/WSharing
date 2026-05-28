#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include "App.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool startMinimized = (argc >= 2 && std::wstring(argv[1]) == L"--minimized");
    LocalFree(argv);

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
                    L"Bitte DirectX 11 und Windows 10+ pr\u00fcfen.",
                    L"WSharing \u2013 Fehler",
                    MB_ICONERROR | MB_OK);
        CloseHandle(mutex);
        return 1;
    }


    app.run();

    CloseHandle(mutex);
    return 0;
}
