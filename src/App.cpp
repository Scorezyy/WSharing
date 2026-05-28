#include "App.h"
#include "core/AppLog.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <commdlg.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "comdlg32.lib")

App* App::s_inst = nullptr;

App::App()  { s_inst = this; }
App::~App() { s_inst = nullptr; }

bool App::init(HINSTANCE hInstance, bool startMinimized)
{
    m_cfg.load();

    snprintf(m_folderBuf, sizeof(m_folderBuf), "%s",
        std::string(m_cfg.sharedFolder.begin(), m_cfg.sharedFolder.end()).c_str());
    snprintf(m_nameBuf,   sizeof(m_nameBuf),   "%s",
        std::string(m_cfg.shareName.begin(), m_cfg.shareName.end()).c_str());
    snprintf(m_portBuf,   sizeof(m_portBuf),   "%d", m_cfg.hostPort);
    m_dlBuf[0] = (char)m_cfg.driveLetter;
    m_dlBuf[1] = '\0';

    // Custom drive name
    WideCharToMultiByte(CP_UTF8, 0,
        m_cfg.customDriveName.c_str(), -1,
        m_driveNameBuf, sizeof(m_driveNameBuf), nullptr, nullptr);

    // Custom drive icon path
    WideCharToMultiByte(CP_UTF8, 0,
        m_cfg.driveIconPath.c_str(), -1,
        m_driveIconBuf, sizeof(m_driveIconBuf), nullptr, nullptr);

    m_connected = false;

    enumAdapters();
    for (int i = 0; i < (int)m_adapters.size(); ++i) {
        if (m_adapters[i].ip == m_cfg.networkInterface) { m_selAdapter = i; break; }
    }

    if (!createWindow(hInstance)) return false;
    if (!createDevice())          return false;

    BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename  = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    setupStyle();
    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_device, m_ctx);

    m_tray.create(m_hwnd, L"WSharing", [this](int id) {
        if      (id == TrayIcon::ID_SHOW) show();
        else if (id == TrayIcon::ID_EXIT) DestroyWindow(m_hwnd);
    });

    if (!startMinimized && !m_cfg.startMinimized) {
        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hwnd);
    } else {
        m_visible = false;
    }
    return true;
}

void App::run()
{
    const std::string& iface = m_cfg.networkInterface;
    m_disc.startListening(iface);

    if (m_cfg.hostEnabled && !m_cfg.sharedFolder.empty()) {
        if (m_server.start(m_cfg.sharedFolder, m_cfg.hostPort))
            m_disc.startBroadcasting(m_cfg.shareName, m_cfg.hostPort, iface);
        else
            m_cfg.hostEnabled = false;
    }

    MSG msg{};
    while (true) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) goto done;
        }
        if (m_visible) renderFrame();
        else           Sleep(16);
    }

done:
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_server.stop();
    m_disc.stopBroadcasting();
    m_disc.stopListening();
    if (m_connected) DriveMounter::unmount(m_cfg.driveLetter);

    cleanupDevice();
}

void App::enumAdapters()
{
    m_adapters.clear();
    ULONG sz = 0;
    GetAdaptersInfo(nullptr, &sz);
    if (sz == 0) return;

    std::vector<BYTE> buf(sz);
    if (GetAdaptersInfo(reinterpret_cast<IP_ADAPTER_INFO*>(buf.data()), &sz) != NO_ERROR)
        return;

    auto* p = reinterpret_cast<IP_ADAPTER_INFO*>(buf.data());
    for (; p; p = p->Next) {
        std::string ip = p->IpAddressList.IpAddress.String;
        if (ip == "0.0.0.0" || ip.empty()) continue;
        AdapterInfo ai;
        ai.ip   = ip;
        ai.name = std::string(p->Description) + "  (" + ip + ")";
        m_adapters.push_back(ai);
    }
}

static std::wstring utf8ToWide(const char* s)
{
    if (!s || !*s) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), n);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

void App::cmdStartHost()
{
    m_server.stop();
    m_disc.stopBroadcasting();

    std::wstring folder = utf8ToWide(m_folderBuf);
    std::wstring name   = utf8ToWide(m_nameBuf);
    int          port   = atoi(m_portBuf);

    if (folder.empty()) { setStatus(false, "Bitte zuerst einen Ordner waehlen."); return; }

    m_cfg.sharedFolder = folder;
    m_cfg.shareName    = name;
    m_cfg.hostPort     = (uint16_t)port;

    if (m_server.start(folder, m_cfg.hostPort)) {
        m_cfg.hostEnabled = true;
        m_disc.startBroadcasting(m_cfg.shareName, m_cfg.hostPort, m_cfg.networkInterface);
        AppLog::get().add(LogEntry::Kind::Info,
            "Host gestartet – Port " + std::to_string(port) +
            "  Freigabe: " + std::string(m_nameBuf));
        setStatus(true, "Freigabe aktiv auf Port " + std::to_string(port));
    } else {
        m_cfg.hostEnabled = false;
        AppLog::get().add(LogEntry::Kind::Error,
            "Host-Start fehlgeschlagen – Port " + std::to_string(port) + " belegt");
        setStatus(false, "Port " + std::to_string(port) + " ist bereits belegt.");
    }
    m_cfg.save();
}

void App::cmdStopHost()
{
    m_server.stop();
    m_disc.stopBroadcasting();
    m_cfg.hostEnabled = false;
    m_cfg.save();
    AppLog::get().add(LogEntry::Kind::Info, "Host gestoppt.");
    setStatus(true, "Server gestoppt.");
}

void App::cmdBrowseFolder()
{
    BROWSEINFOW bi{};
    bi.hwndOwner = m_hwnd;
    bi.lpszTitle = L"Ordner zum Freigeben";
    bi.ulFlags   = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH]{};
        if (SHGetPathFromIDListW(pidl, path))
            WideCharToMultiByte(CP_UTF8, 0, path, -1,
                m_folderBuf, sizeof(m_folderBuf), nullptr, nullptr);
        CoTaskMemFree(pidl);
    }
}

void App::cmdConnect()
{
    auto hosts = m_disc.getHosts();
    if (m_selHost < 0 || m_selHost >= (int)hosts.size()) return;

    wchar_t dl = (wchar_t)(m_dlBuf[0] ? toupper(m_dlBuf[0]) : L'Z');
    m_cfg.driveLetter = dl;

    m_cfg.customDriveName = utf8ToWide(m_driveNameBuf);
    m_cfg.driveIconPath   = utf8ToWide(m_driveIconBuf);

    auto& h = hosts[m_selHost];
    if (DriveMounter::mount(h.ip, h.port, dl, h.shareName,
                            m_cfg.customDriveName, m_cfg.driveIconPath)) {
        m_connected   = true;
        m_connectedTo = std::string(h.name.begin(), h.name.end());
        m_cfg.clientEnabled = true;
        AppLog::get().add(LogEntry::Kind::Connect,
            "Verbunden mit " + m_connectedTo + " (" + h.ip + ":" + std::to_string(h.port) +
            ")  ->  " + std::string(1, (char)dl) + ":");
        setStatus(true, "Verbunden mit " + m_connectedTo);
    } else {
        AppLog::get().add(LogEntry::Kind::Error,
            "Verbindung fehlgeschlagen: " + h.ip + ":" + std::to_string(h.port));
        setStatus(false, "Verbindung fehlgeschlagen. WebClient-Dienst aktiv?");
    }
    m_cfg.save();
}

void App::cmdDisconnect()
{
    DriveMounter::unmount(m_cfg.driveLetter);
    AppLog::get().add(LogEntry::Kind::Disconnect,
        "Getrennt von " + m_connectedTo);
    m_connected   = false;
    m_connectedTo.clear();
    m_cfg.clientEnabled = false;
    m_cfg.save();
    setStatus(true, "Laufwerk getrennt.");
}

void App::cmdSaveSettings()
{
    HKEY key{};
    RegOpenKeyExW(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &key);
    if (key) {
        if (m_cfg.autostart) {
            wchar_t exe[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe, MAX_PATH);
            std::wstring val = std::wstring(L"\"") + exe + L"\" --minimized";
            RegSetValueExW(key, L"WSharing", 0, REG_SZ,
                (const BYTE*)val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(key, L"WSharing");
        }
        RegCloseKey(key);
    }
    m_cfg.save();

    setStatus(true, "Einstellungen gespeichert.");
}

void App::cmdBrowseDriveIcon()
{
    OPENFILENAMEW ofn{};
    wchar_t       path[MAX_PATH]{};

    // Vorbefüllen mit aktuellem Wert
    MultiByteToWideChar(CP_UTF8, 0, m_driveIconBuf, -1, path, MAX_PATH);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = m_hwnd;
    ofn.lpstrFilter = L"Icon-Dateien (*.ico)\0*.ico\0Alle Dateien (*.*)\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle  = L"Icon für Laufwerk wählen";

    if (GetOpenFileNameW(&ofn)) {
        WideCharToMultiByte(CP_UTF8, 0, path, -1,
            m_driveIconBuf, sizeof(m_driveIconBuf), nullptr, nullptr);
    }
}
