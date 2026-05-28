#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"
#include "../design/Strings.h"
#include "../core/Lang.h"

void App::drawSettingsPage()
{
    float fw = ImGui::GetContentRegionAvail().x - 40.f;

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TEXT);
    ImGui::TextUnformatted(S().settingsTitle.c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().settingsSubtitle.c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.f);
    ImGui::SetCursorPosX(20.f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 14.f});
    ImGui::BeginChild("##settcard", {fw, 0}, true, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().language.c_str());
    ImGui::PopStyleColor();
    int langIdx = m_cfg.language;
    int langCount = Lang::instance().count();
    const char* curName = (langIdx >= 0 && langIdx < langCount)
        ? Lang::instance().nameAt(langIdx).c_str() : "";
    ImGui::SetNextItemWidth(fw - 32.f);
    if (ImGui::BeginCombo("##language", curName)) {
        for (int i = 0; i < langCount; ++i) {
            bool sel = (langIdx == i);
            if (ImGui::Selectable(Lang::instance().nameAt(i).c_str(), sel)) {
                m_cfg.language = i;
                Lang::instance().load(i);
                cmdSaveSettings();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    bool autostart = m_cfg.autostart;
    if (ImGui::Checkbox(S().startWithWindows.c_str(), &autostart)) {
        m_cfg.autostart = autostart;
        cmdSaveSettings();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().startWithWindowsHint.c_str());
    ImGui::PopStyleColor();

    ImGui::Separator();

    bool minimized = m_cfg.startMinimized;
    if (ImGui::Checkbox(S().startMinimized.c_str(), &minimized)) {
        m_cfg.startMinimized = minimized;
        cmdSaveSettings();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().startMinimizedHint.c_str());
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().networkAdapter.c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(fw - 32.f);

    const char* curLabel = (m_selAdapter < 0)
        ? S().adapterAuto.c_str()
        : m_adapters[m_selAdapter].name.c_str();

    if (ImGui::BeginCombo("##adapter", curLabel)) {
        if (ImGui::Selectable(S().adapterAuto.c_str(), m_selAdapter < 0)) {
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
    ImGui::TextUnformatted(S().adapterHint.c_str());
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().driveIcon.c_str());
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
    if (ImGui::Button(S().browse.c_str(), {90.f, 0})) cmdBrowseDriveIcon();
    ImGui::PopStyleColor(2);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().driveIconHint.c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
