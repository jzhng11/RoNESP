#include "esp.h"
#include "w2s.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <format>

static uint32_t Color(float r, float g, float b, float a = 1.f) {
    return IM_COL32(
        static_cast<int>(r * 255.f),
        static_cast<int>(g * 255.f),
        static_cast<int>(b * 255.f),
        static_cast<int>(a * 255.f)
    );
}

void ESP::Draw(const GameState& state, const EspConfig& cfg) {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;

    if (cfg.showCrosshair)
        DrawCrosshair();

    for (const auto& ent : state.entities) {
        if (!ent.IsValid()) continue;
        if (!ent.isAlive && !cfg.showDead) continue;
        DrawEntity(state, ent, cfg);
    }
}

void ESP::DrawEntity(const GameState& state, const Entity& ent, const EspConfig& cfg) {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;

    FVector2D screenSize(static_cast<float>(state.camera.screenWidth),
                         static_cast<float>(state.camera.screenHeight));

    FVector2D feetScreen, headScreen;

    if (!WorldToScreen(ent.feetPosition, state.camera.location, state.camera.rotation,
        state.camera.fov, screenSize.x, screenSize.y, feetScreen))
        return;

    if (!WorldToScreen(ent.headPosition, state.camera.location, state.camera.rotation,
        state.camera.fov, screenSize.x, screenSize.y, headScreen))
        return;

    float boxHeight = std::abs(feetScreen.y - headScreen.y);
    float boxWidth = boxHeight * 0.5f;

    uint32_t boxColor, nameColor;
    if (ent.isDowned) {
        boxColor = Color(1.f, 1.f, 0.2f);
        nameColor = Color(1.f, 1.f, 0.4f);
    } else if (!ent.isAlive) {
        boxColor = Color(0.5f, 0.5f, 0.5f, 0.5f);
        nameColor = Color(0.5f, 0.5f, 0.5f, 0.5f);
    } else if (ent.isSuspect) {
        boxColor = Color(1.f, 0.2f, 0.2f);
        nameColor = Color(1.f, 0.4f, 0.4f);
    } else if (ent.isCivilian) {
        boxColor = Color(0.2f, 1.f, 0.2f);
        nameColor = Color(0.4f, 1.f, 0.4f);
    } else {
        boxColor = Color(0.2f, 0.6f, 1.f);
        nameColor = Color(0.4f, 0.8f, 1.f);
    }

    if (cfg.showBoxes && boxHeight > 2.f && boxWidth > 1.f) {
        float x = feetScreen.x - boxWidth * 0.5f;
        float y = headScreen.y;

        draw->AddRect(
            ImVec2(x, y),
            ImVec2(x + boxWidth, y + boxHeight),
            boxColor, 0.f, 0, 1.5f
        );
    }

    if (cfg.showHealth) {
        DrawHealthBar(headScreen, feetScreen, ent.health, ent.maxHealth);
    }

    if (cfg.showNames && !ent.name.empty()) {
        std::string label = ent.name;
        if (label.size() > 30) {
            label = label.substr(0, 27) + "...";
        }

        draw->AddText(
            ImVec2(headScreen.x - 50.f, headScreen.y - 18.f),
            nameColor, label.c_str()
        );
    }

    float belowY = feetScreen.y + 2.f;
    if (cfg.showDist) {
        std::string distStr = std::format("{:.0f}m", ent.distance);
        draw->AddText(
            ImVec2(feetScreen.x - 50.f, belowY),
            Color(1.f, 1.f, 1.f, 0.6f), distStr.c_str()
        );
        belowY += 16.f;
    }

    if (ent.isDowned) {
        draw->AddText(
            ImVec2(feetScreen.x - 50.f, belowY),
            Color(1.f, 1.f, 0.2f), "[DOWNED]"
        );
    } else if (!ent.isAlive) {
        draw->AddText(
            ImVec2(feetScreen.x - 50.f, belowY),
            Color(0.5f, 0.5f, 0.5f, 0.5f), "[DEAD]"
        );
    }

    if (cfg.showTracers) {
        DrawLine(
            FVector2D(feetScreen.x, feetScreen.y),
            FVector2D(screenSize.x * 0.5f, screenSize.y),
            boxColor
        );
    }
}

void ESP::DrawBox(const FVector2D& head, const FVector2D& feet, uint32_t color) {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;

    float height = std::abs(feet.y - head.y);
    float width = height * 0.5f;
    float x = feet.x - width * 0.5f;

    draw->AddRect(
        ImVec2(x, head.y),
        ImVec2(x + width, feet.y),
        color, 0.f, 0, 1.5f
    );
}

void ESP::DrawLine(const FVector2D& from, const FVector2D& to, uint32_t color) {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    draw->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), color, 1.f);
}

void ESP::DrawText(const std::string& text, float x, float y, uint32_t color) {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;
    draw->AddText(ImVec2(x, y), color, text.c_str());
}

void ESP::DrawHealthBar(const FVector2D& head, const FVector2D& feet, float health, float maxHealth) {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw || maxHealth <= 0.f) return;

    float height = std::abs(feet.y - head.y);
    if (height < 2.f) return;
    float width = 4.f;

    float x = feet.x - height * 0.25f - width - 2.f;
    float y = head.y;

    float ratio = std::clamp(health / maxHealth, 0.f, 1.f);
    float fillHeight = height * ratio;

    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), Color(0.1f, 0.1f, 0.1f, 0.6f));

    uint32_t fillColor;
    if (ratio > 0.6f) {
        fillColor = Color(0.2f, 1.f, 0.2f);
    } else if (ratio > 0.3f) {
        fillColor = Color(1.f, 1.f, 0.2f);
    } else {
        fillColor = Color(1.f, 0.2f, 0.2f);
    }

    float fillY = y + (height - fillHeight);
    draw->AddRectFilled(ImVec2(x, fillY), ImVec2(x + width, y + fillHeight), fillColor);

    draw->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), Color(0.5f, 0.5f, 0.5f, 0.5f), 0.f, 0, 1.f);
}

void ESP::DrawCrosshair() {
    auto* draw = ImGui::GetBackgroundDrawList();
    if (!draw) return;

    ImVec2 center = ImGui::GetIO().DisplaySize;
    center.x *= 0.5f;
    center.y *= 0.5f;

    float size = 8.f;
    uint32_t color = Color(1.f, 1.f, 1.f, 0.5f);

    draw->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), color, 1.f);
    draw->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), color, 1.f);
}
