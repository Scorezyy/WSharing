#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"

void App::drawSettingsPage()
{
    float fw = ImGui::GetContentRegionAvail().x - 40.f;

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TEXT);
    ImGui::Text("Einstellungen");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Programm-Konfiguration");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.f);
    ImGui::SetCursorPosX(20.f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 14.f});
    ImGui::BeginChild("##settcard", {fw, 0}, true, ImGuiWindowFlags_NoScrollbar);

    bool autostart = m_cfg.autostart;
    if (ImGui::Checkbox("Mit Windows starten", &autostart)) {
        m_cfg.autostart = autostart;
        cmdSaveSettings();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("WSharing beim Windows-Start automatisch oeffnen");
    ImGui::PopStyleColor();

    ImGui::Separator();

    bool minimized = m_cfg.startMinimized;
    if (ImGui::Checkbox("Minimiert starten", &minimized)) {
        m_cfg.startMinimized = minimized;
        cmdSaveSettings();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Fenster beim Start in der Taskleiste verstecken");
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Netzwerkadapter");
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(fw - 32.f);

    const char* curLabel = (m_selAdapter < 0)
        ? "Automatisch (empfohlen)"
        : m_adapters[m_selAdapter].name.c_str();

    if (ImGui::BeginCombo("##adapter", curLabel)) {
        if (ImGui::Selectable("Automatisch (empfohlen)", m_selAdapter < 0)) {
            m_selAdapter = -1;
            m_cfg.networkInterface.clear();
            cmdSaveSettings();
        }
        if (m_selAdapter < 0) ImGui::SetItemDefaultFocus();

        for (int i = 0; i < (int)m_adapters.size(); ++i) {
            bool sel = (m_selAdapter == i);
            if (ImGui::Selectable(m_adapters[i].name.c_str(), sel)) {
                m_selAdapter = i;
                m_cfg.networkInterface = m_adapters[i].ip;
                cmdSaveSettings();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Adapter fuer Discovery und Host-Broadcast");
    ImGui::PopStyleColor();

    ImGui::Separator();

    // --- Drive icon ---
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Laufwerk-Icon (.ico)");
    ImGui::PopStyleColor();
    float iconFieldW = fw - 32.f - 90.f - 8.f;
    ImGui::SetNextItemWidth(iconFieldW);
    if (ImGui::InputText("##driveicon", m_driveIconBuf, sizeof(m_driveIconBuf)))
    {
        m_cfg.driveIconPath = std::wstring(m_driveIconBuf, m_driveIconBuf + strlen(m_driveIconBuf));
        m_cfg.save();
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        Colors::SURFACE2);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::BORDER);
    if (ImGui::Button("Suchen...##iconbrowse", {90.f, 0})) cmdBrowseDriveIcon();
    ImGui::PopStyleColor(2);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Icon fuer das Laufwerk im Explorer (leer = Standard)");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
