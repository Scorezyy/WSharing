#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"
#include "../design/Strings.h"

void App::drawSidebar()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE);
    ImGui::BeginChild("##sidebar", {200.f, -1}, false);

    ImGui::SetCursorPos({16.f, 20.f});
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::ACCENT);
    ImGui::Text("WSharing");
    ImGui::PopStyleColor();

    ImGui::SetCursorPos({16.f, 44.f});
    if (m_smb.isRunning()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::SUCCESS);
        ImGui::TextUnformatted(S().hostingActive.c_str());
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
        ImGui::TextUnformatted(S().notHosting.c_str());
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(90.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {12.f, 10.f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  {0.f,   4.f});

    const char* labels[] = {S().navHost.c_str(), S().navClient.c_str(), S().navSettings.c_str(), S().navLog.c_str()};
    for (int i = 0; i < 4; ++i) {
        bool active = (m_page == i);
        ImGui::PushStyleColor(ImGuiCol_Button,
            active ? ImVec4{Colors::ACCENT.x, Colors::ACCENT.y, Colors::ACCENT.z, 0.18f}
                   : ImVec4{0, 0, 0, 0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            active ? ImVec4{Colors::ACCENT.x, Colors::ACCENT.y, Colors::ACCENT.z, 0.25f}
                   : Colors::SURFACE2);
        ImGui::PushStyleColor(ImGuiCol_Text, active ? Colors::ACCENT : Colors::TEXT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        ImGui::SetCursorPosX(8.f);
        if (ImGui::Button(labels[i], {184.f, 0})) m_page = i;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    ImGui::PopStyleVar(2);

    ImGui::SetCursorPos({16.f, io.DisplaySize.y - 30.f});
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("v2.1.0 \xe2\x80\xa2 by Jxstn");
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
