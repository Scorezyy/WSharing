#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"
#include "../design/Strings.h"

void App::drawHostPage()
{
    float fw = ImGui::GetContentRegionAvail().x - 40.f;

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TEXT);
    ImGui::Text("Host");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().hostSubtitle.c_str());
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.f);
    ImGui::SetCursorPosX(20.f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.f, 14.f});
    ImGui::BeginChild("##hostcard", {fw, 0}, true, ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().folder.c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(fw - 110.f);
    ImGui::InputText("##folder", m_folderBuf, sizeof(m_folderBuf));
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, Colors::SURFACE2);
    if (ImGui::Button(S().browse.c_str(), {-1, 0})) cmdBrowseFolder();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::TextUnformatted(S().shareName.c_str());
    ImGui::PopStyleColor();
    ImGui::SetNextItemWidth(fw - 32.f);
    ImGui::InputText("##name", m_nameBuf, sizeof(m_nameBuf));

    ImGui::Spacing();

    bool running = m_smb.isRunning();
    if (running) {
        ImGui::PushStyleColor(ImGuiCol_Button,        Colors::DANGER);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.95f, 0.2f, 0.17f, 1.f});
        if (ImGui::Button(S().stopSharing.c_str(), {0, 36.f})) cmdStopHost();
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.f);
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::SUCCESS);
        ImGui::TextUnformatted(S().sharingActive.c_str());
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        Colors::ACCENT);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.26f, 0.62f, 1.f, 1.f});
        if (ImGui::Button(S().startSharing.c_str(), {0, 36.f})) cmdStartHost();
        ImGui::PopStyleColor(2);
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
