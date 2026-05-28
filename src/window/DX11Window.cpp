#include "../App.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <dwmapi.h>
#include <dxgi.h>
#include "../design/Colors.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

bool App::createWindow(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"WSharingApp";
    wc.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);
    m_hwnd = CreateWindowExW(0, L"WSharingApp", L"WSharing",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 980, 640,
        nullptr, nullptr, hInstance, nullptr);
    return m_hwnd != nullptr;
}

bool App::createDevice()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount        = 2;
    sd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow       = m_hwnd;
    sd.SampleDesc.Count   = 1;
    sd.Windowed           = TRUE;
    sd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl;
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &fl, &m_ctx)))
        return false;
    ID3D11Texture2D* buf{};
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&buf));
    m_device->CreateRenderTargetView(buf, nullptr, &m_rtv);
    buf->Release();
    return true;
}

void App::cleanupDevice()
{
    if (m_rtv)       { m_rtv->Release();      m_rtv       = nullptr; }
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_ctx)       { m_ctx->Release();       m_ctx       = nullptr; }
    if (m_device)    { m_device->Release();    m_device    = nullptr; }
}

void App::resizeBuffers(UINT w, UINT h)
{
    if (m_rtv) { m_rtv->Release(); m_rtv = nullptr; }
    m_swapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    ID3D11Texture2D* buf{};
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&buf));
    m_device->CreateRenderTargetView(buf, nullptr, &m_rtv);
    buf->Release();
}

void App::show() { m_visible = true;  ShowWindow(m_hwnd, SW_SHOW); SetForegroundWindow(m_hwnd); }
void App::hide() { m_visible = false; ShowWindow(m_hwnd, SW_HIDE); }

void App::setupStyle()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 8.f;  s.FrameRounding  = 6.f;
    s.GrabRounding      = 6.f;  s.PopupRounding  = 6.f;
    s.ScrollbarRounding = 6.f;  s.TabRounding    = 6.f;
    s.FramePadding  = {10.f, 6.f};
    s.ItemSpacing   = {10.f, 8.f};
    s.WindowPadding = {0.f,  0.f};
    s.ScrollbarSize = 10.f;
    s.WindowBorderSize = 0.f;
    s.FrameBorderSize  = 1.f;

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]             = Colors::BG;
    c[ImGuiCol_ChildBg]              = Colors::SURFACE;
    c[ImGuiCol_PopupBg]              = Colors::SURFACE;
    c[ImGuiCol_Border]               = Colors::BORDER;
    c[ImGuiCol_FrameBg]              = Colors::SURFACE2;
    c[ImGuiCol_FrameBgHovered]       = {0.16f, 0.19f, 0.23f, 1.f};
    c[ImGuiCol_FrameBgActive]        = Colors::SURFACE2;
    c[ImGuiCol_TitleBg]              = Colors::SURFACE;
    c[ImGuiCol_TitleBgActive]        = Colors::SURFACE;
    c[ImGuiCol_Header]               = {0.18f, 0.22f, 0.27f, 1.f};
    c[ImGuiCol_HeaderHovered]        = {0.22f, 0.27f, 0.34f, 1.f};
    c[ImGuiCol_HeaderActive]         = Colors::ACCENT;
    c[ImGuiCol_Button]               = Colors::SURFACE2;
    c[ImGuiCol_ButtonHovered]        = {0.22f, 0.27f, 0.34f, 1.f};
    c[ImGuiCol_ButtonActive]         = Colors::ACCENT;
    c[ImGuiCol_CheckMark]            = Colors::ACCENT;
    c[ImGuiCol_SliderGrab]           = Colors::ACCENT;
    c[ImGuiCol_SliderGrabActive]     = Colors::ACCENT;
    c[ImGuiCol_Separator]            = Colors::BORDER;
    c[ImGuiCol_SeparatorHovered]     = Colors::BORDER;
    c[ImGuiCol_SeparatorActive]      = Colors::BORDER;
    c[ImGuiCol_Text]                 = Colors::TEXT;
    c[ImGuiCol_TextDisabled]         = Colors::MUTED;
    c[ImGuiCol_ScrollbarBg]          = Colors::BG;
    c[ImGuiCol_ScrollbarGrab]        = Colors::BORDER;
    c[ImGuiCol_ScrollbarGrabHovered] = Colors::MUTED;
    c[ImGuiCol_Tab]                  = Colors::SURFACE;
    c[ImGuiCol_TabHovered]           = Colors::SURFACE2;
    c[ImGuiCol_TabActive]            = Colors::SURFACE2;
}

void App::renderFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x < 10.f || io.DisplaySize.y < 10.f) {
        ImGui::EndFrame();
        return;
    }

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##root", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav);

    drawSidebar();
    ImGui::SameLine(0, 0);

    ImGui::BeginChild("##content", {io.DisplaySize.x - 200.f, -1}, false);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
    if      (m_page == 0) drawHostPage();
    else if (m_page == 1) drawClientPage();
    else if (m_page == 2) drawSettingsPage();
    else                  drawLogPage();
    ImGui::EndChild();

    drawStatusToast();
    ImGui::End();

    const float bg[4] = {Colors::BG.x, Colors::BG.y, Colors::BG.z, 1.f};
    m_ctx->OMSetRenderTargets(1, &m_rtv, nullptr);
    m_ctx->ClearRenderTargetView(m_rtv, bg);
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_swapChain->Present(1, 0);
}

LRESULT CALLBACK App::wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return true;
    App* app = s_inst;
    switch (msg) {
    case WM_SIZE:
        if (app && app->m_swapChain && wp != SIZE_MINIMIZED)
            app->resizeBuffers(LOWORD(lp), HIWORD(lp));
        return 0;
    case WM_APP + 1:
        if (app) app->m_tray.handleMessage(wp, lp);
        return 0;
    case WM_CLOSE:
        if (app) app->hide();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
