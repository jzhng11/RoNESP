#pragma once
#include <vector>
#include "game.h"

struct EspConfig {
    bool showBoxes      = true;
    bool showTracers    = true;
    bool showNames      = true;
    bool showHealth     = true;
    bool showDist       = true;
    bool showDead       = false;
    bool showCrosshair  = true;
};

class ESP {
public:
    void Draw(const GameState& state, const EspConfig& cfg);

private:
    void DrawBox(const FVector2D& head, const FVector2D& feet, uint32_t color);
    void DrawLine(const FVector2D& from, const FVector2D& to, uint32_t color);
    void DrawText(const std::string& text, float x, float y, uint32_t color);
    void DrawHealthBar(const FVector2D& head, const FVector2D& feet, float health, float maxHealth);
    void DrawEntity(const GameState& state, const Entity& ent, const EspConfig& cfg);
    void DrawCrosshair();
};
