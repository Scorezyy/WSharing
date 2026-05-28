#include "TrayIcon.h"
#include <strsafe.h>

TrayIcon::~TrayIcon() { destroy(); }

bool TrayIcon::create(HWND hwnd, const std::wstring& tooltip, ActionCallback cb)
{
    m_hwnd     = hwnd;
    m_callback = std::move(cb);

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize           = sizeof(m_nid);
    m_nid.hWnd             = hwnd;
    m_nid.uID              = TRAY_ID;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = TRAY_MSG;
    m_nid.hIcon            = LoadIconW(nullptr, IDI_APPLICATION);

    StringCchCopyW(m_nid.szTip, ARRAYSIZE(m_nid.szTip), tooltip.c_str());

    m_created = Shell_NotifyIconW(NIM_ADD, &m_nid) == TRUE;
    return m_created;
}

void TrayIcon::destroy()
{
    if (m_created) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_created = false;
    }
}

void TrayIcon::handleMessage(WPARAM /*wParam*/, LPARAM lParam)
{
    switch (LOWORD(lParam)) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            showContextMenu();
            break;
        case WM_LBUTTONDBLCLK:
            if (m_callback) m_callback(ID_SHOW);
            break;
        default:
            break;
    }
}

void TrayIcon::showContextMenu()
{
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, ID_SHOW, L"Anzeigen / Verstecken");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_EXIT, L"Beenden");

    // Required so the menu closes when user clicks elsewhere
    SetForegroundWindow(m_hwnd);

    POINT pt{};
    GetCursorPos(&pt);

    int cmd = TrackPopupMenu(menu,
                              TPM_RETURNCMD | TPM_RIGHTBUTTON,
                              pt.x, pt.y, 0, m_hwnd, nullptr);
    DestroyMenu(menu);

    if (cmd && m_callback) m_callback(cmd);
}
