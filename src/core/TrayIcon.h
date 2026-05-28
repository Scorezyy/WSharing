#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <functional>
#include <string>

// Windows system tray icon with a context menu.
// Must be created and destroyed on the same thread that runs the message loop.
class TrayIcon
{
public:
    using ActionCallback = std::function<void(int menuId)>;

    // menuId constants
    static constexpr int ID_SHOW  = 1001;
    static constexpr int ID_EXIT  = 1002;

    TrayIcon() = default;
    ~TrayIcon();

    bool create(HWND hwnd, const std::wstring& tooltip, ActionCallback cb);
    void destroy();

    // Call from your WndProc when uMsg == WM_APP + 1
    void handleMessage(WPARAM wParam, LPARAM lParam);

private:
    void showContextMenu();

    HWND           m_hwnd     { nullptr };
    NOTIFYICONDATAW m_nid     {};
    ActionCallback m_callback;
    bool           m_created  { false };

    static constexpr UINT TRAY_MSG = WM_APP + 1;
    static constexpr UINT TRAY_ID  = 1;
};
