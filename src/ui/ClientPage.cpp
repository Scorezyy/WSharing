#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"
#include "../design/Strings.h"
#include "../network/DriveMounter.h"

void App::drawClientPage()
{
    float fw    = ImGui::GetContentRegionAvail().x - 40.f;
    auto  hosts = m_disc.getHosts();

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TEXT);
    ImGui::Text("Client");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().clientSubtitle.c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.f);
    ImGui::SetCursorPosX(20.f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 14.f});
    ImGui::BeginChild("##clientcard", {fw, 0}, true, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text(S().foundHosts.c_str(), (int)hosts.size());
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE2);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
    ImGui::BeginChild("##hostlist", {fw - 32.f, 140.f}, false);

    if (hosts.empty()) {
        ImGui::SetCursorPos({10.f, 55.f});
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
        ImGui::TextUnformatted(S().searchingHosts.c_str());
        ImGui::PopStyleColor();
    }

    for (int i = 0; i < (int)hosts.size(); ++i) {
        bool sel = (m_selHost == i);
        ImGui::SetCursorPosX(6.f);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Header,
            {Colors::ACCENT.x, Colors::ACCENT.y, Colors::ACCENT.z, 0.2f});
        char lbl[256];
        std::string n(hosts[i].name.begin(), hosts[i].name.end());
        snprintf(lbl, sizeof(lbl), "%s  |  %s:%d###h%d",
            n.c_str(), hosts[i].ip.c_str(), hosts[i].port, i);
        if (ImGui::Selectable(lbl, sel, 0, {fw - 44.f, 28.f})) m_selHost = i;
        if (sel) ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().driveLetter.c_str());
    ImGui::PopStyleColor();

    {
        auto avail = DriveMounter::getAvailableLetters();

        wchar_t curLetter = m_dlBuf[0] ? (wchar_t)toupper((unsigned char)m_dlBuf[0]) : L'Z';

        int selIdx = 0;
        for (int i = 0; i < (int)avail.size(); ++i)
            if (avail[i] == curLetter) { selIdx = i; break; }

        char curLabel[4] = { m_dlBuf[0] ? (char)toupper((unsigned char)m_dlBuf[0]) : 'Z', ':', '\0', '\0' };

        ImGui::SetNextItemWidth(80.f);
        if (ImGui::BeginCombo("##dl", curLabel)) {
            for (int i = 0; i < (int)avail.size(); ++i) {
                char opt[4] = { (char)avail[i], ':', '\0', '\0' };
                bool isSelected = (i == selIdx);
                if (ImGui::Selectable(opt, isSelected)) {
                    m_dlBuf[0] = (char)avail[i];
                    m_dlBuf[1] = '\0';
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().displayName.c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(fw - 32.f);
    ImGui::InputText("##drivename", m_driveNameBuf, sizeof(m_driveNameBuf));

    ImGui::Spacing();

    if (m_connected) {
        ImGui::PushStyleColor(ImGuiCol_Button,        Colors::DANGER);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.95f, 0.2f, 0.17f, 1.f});
        if (ImGui::Button(S().disconnect.c_str(), {0, 36.f})) cmdDisconnect();
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.f);
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::SUCCESS);
        ImGui::Text(S().connectedTo.c_str(), m_connectedTo.c_str());
        ImGui::PopStyleColor();
    } else {
        bool can = (m_selHost >= 0 && m_selHost < (int)hosts.size());
        if (!can) ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button,        Colors::ACCENT);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.26f, 0.62f, 1.f, 1.f});
        if (ImGui::Button(S().connect.c_str(), {0, 36.f})) cmdConnect();
        ImGui::PopStyleColor(2);
        if (!can) ImGui::EndDisabled();
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
