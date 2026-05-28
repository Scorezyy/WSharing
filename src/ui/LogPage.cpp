#include "../App.h"
#include <imgui.h>
#include "../design/Colors.h"
#include "../core/AppLog.h"

void App::drawLogPage()
{
    float fw = ImGui::GetContentRegionAvail().x - 40.f;
    float fh = ImGui::GetContentRegionAvail().y - 20.f;

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TEXT);
    ImGui::Text("Log");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(20.f);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
    ImGui::Text("Verbindungen und Dateioperationen");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.f);

    // Toolbar row
    ImGui::SetCursorPosX(20.f);
    ImGui::Checkbox("Auto-Scroll", &m_logAutoScroll);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        Colors::SURFACE2);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::BORDER);
    if (ImGui::Button("Leeren##log")) AppLog::get().clear();
    ImGui::PopStyleColor(2);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
    ImGui::SetCursorPosX(20.f);

    // Log list
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SURFACE);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10.f, 8.f});
    ImGui::BeginChild("##logchild", {fw, fh - 90.f}, true, ImGuiWindowFlags_HorizontalScrollbar);

    auto entries = AppLog::get().snapshot();

    if (entries.empty()) {
        ImGui::SetCursorPosY(ImGui::GetContentRegionAvail().y * 0.4f);
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - 180.f) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
        ImGui::Text("Noch keine Ereignisse.");
        ImGui::PopStyleColor();
    }

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1
                    ? ImGui::GetIO().Fonts->Fonts[1] : nullptr);

    for (const auto& e : entries)
    {
        // Timestamp in MUTED
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::MUTED);
        ImGui::TextUnformatted(e.timestamp.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 8.f);

        // Color + icon by kind
        ImVec4 col = Colors::TEXT;
        const char* icon = "  ";
        switch (e.kind) {
            case LogEntry::Kind::Connect:    col = Colors::SUCCESS; icon = "[+]"; break;
            case LogEntry::Kind::Disconnect: col = Colors::MUTED;   icon = "[-]"; break;
            case LogEntry::Kind::Transfer:   col = Colors::ACCENT;  icon = "[~]"; break;
            case LogEntry::Kind::Error:      col = Colors::DANGER;  icon = "[!]"; break;
            case LogEntry::Kind::Info:       col = Colors::TEXT;    icon = "[ ]"; break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%s  %s", icon, e.text.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::PopFont();

    if (m_logAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.f)
        ImGui::SetScrollHereY(1.f);

    // Always scroll to bottom when new entries arrive and auto-scroll is on
    static size_t s_lastCount = 0;
    if (m_logAutoScroll && entries.size() != s_lastCount) {
        ImGui::SetScrollHereY(1.f);
        s_lastCount = entries.size();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
