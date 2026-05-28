#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"

void App::setStatus(bool ok, const std::string& msg)
{
    m_statusOk   = ok;
    m_statusMsg  = msg;
    m_statusTime = std::chrono::steady_clock::now();
}

void App::drawStatusToast()
{
    if (m_statusMsg.empty()) return;

    float secs = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - m_statusTime).count();
    if (secs > 4.f) { m_statusMsg.clear(); return; }

    float alpha = secs > 3.f ? 1.f - (secs - 3.f) : 1.f;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.5f, io.DisplaySize.y - 60.f},
        ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowBgAlpha(alpha * 0.92f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, m_statusOk ? Colors::SUCCESS : Colors::DANGER);
    ImGui::PushStyleColor(ImGuiCol_Text,     ImVec4{1, 1, 1, alpha});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  {16.f, 8.f});

    ImGui::Begin("##toast", nullptr,
        ImGuiWindowFlags_NoDecoration  | ImGuiWindowFlags_NoMove      |
        ImGuiWindowFlags_NoNav         | ImGuiWindowFlags_NoInputs    |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("%s", m_statusMsg.c_str());
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}
