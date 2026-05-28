#pragma once
#include <imgui.h>

// Central color palette. Use Colors::NAME everywhere in UI code.
namespace Colors {
    inline const ImVec4 BG       = {0.051f, 0.067f, 0.090f, 1.f};
    inline const ImVec4 SURFACE  = {0.086f, 0.106f, 0.133f, 1.f};
    inline const ImVec4 SURFACE2 = {0.129f, 0.149f, 0.176f, 1.f};
    inline const ImVec4 BORDER   = {0.188f, 0.212f, 0.239f, 1.f};
    inline const ImVec4 ACCENT   = {0.184f, 0.506f, 0.969f, 1.f};
    inline const ImVec4 SUCCESS  = {0.247f, 0.729f, 0.314f, 1.f};
    inline const ImVec4 DANGER   = {0.973f, 0.318f, 0.286f, 1.f};
    inline const ImVec4 TEXT     = {0.902f, 0.929f, 0.953f, 1.f};
    inline const ImVec4 MUTED    = {0.545f, 0.580f, 0.620f, 1.f};
}
