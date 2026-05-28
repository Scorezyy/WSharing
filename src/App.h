#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#include <chrono>

#include "core/Config.h"
#include "core/webdav/WebDavServer.h"
#include "core/TrayIcon.h"
#include "core/Discovery.h"
#include "network/DriveMounter.h"

class App
{
public:
    App();
    ~App();
    bool init(HINSTANCE hInstance, bool startMinimized);
    void run();

private:
    // Win32 / DX11
    bool createWindow(HINSTANCE hInstance);
    bool createDevice();
    void cleanupDevice();
    void resizeBuffers(UINT w, UINT h);
    void show(); void hide();
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

    HWND                     m_hwnd{};
    bool                     m_visible{true};
    ID3D11Device*            m_device{};
    ID3D11DeviceContext*     m_ctx{};
    IDXGISwapChain*          m_swapChain{};
    ID3D11RenderTargetView*  m_rtv{};

    // UI state
    int  m_page{0};           // 0=Host 1=Client 2=Settings
    char m_folderBuf[512]{};
    char m_nameBuf[128]{};
    char m_portBuf[8]{"45679"};
    char m_dlBuf[4]{"Z"};
    char m_driveNameBuf[128]{"WSharing"};  // custom Explorer display name
    char m_driveIconBuf[512]{};            // path to .ico for Explorer icon
    int  m_selHost{-1};
    std::string m_statusMsg;
    bool m_statusOk{true};
    std::chrono::steady_clock::time_point m_statusTime{};

    // Rendering
    void setupStyle();
    void renderFrame();
    void drawSidebar();
    void drawHostPage();
    void drawClientPage();
    void drawSettingsPage();
    void drawLogPage();
    void setStatus(bool ok, const std::string& msg);
    void drawStatusToast();

    // Commands
    void cmdStartHost();
    void cmdStopHost();
    void cmdBrowseFolder();
    void cmdConnect();
    void cmdDisconnect();
    void cmdSaveSettings();
    void cmdBrowseDriveIcon();

    // Network adapters for settings page
    struct AdapterInfo { std::string ip; std::string name; };
    std::vector<AdapterInfo> m_adapters;
    int                      m_selAdapter{-1}; // index into m_adapters, -1=auto
    void                     enumAdapters();

    // Core systems
    Config       m_cfg;
    WebDavServer m_server;
    Discovery    m_disc;
    TrayIcon     m_tray;

    bool        m_connected{false};
    std::string m_connectedTo;
    bool        m_logAutoScroll{true};  // auto-scroll log to bottom

    static App* s_inst;
};