#pragma once

#include <vector>
#include <memory>

class Player;
class Enemy;
class Camera;
class MapSystem;

struct DebugContext {
    const Player*   player;
    const Camera*   camera;
    const MapSystem* map;
    const std::vector<std::unique_ptr<Enemy>>* waves[3];
    int waveCount;
    const char* levelName;
};

void Debug_Load();
void Debug_Init();
void Debug_Register(const DebugContext& ctx);
void Debug_Update();
void Debug_DrawWorld(float camX, float camY);   // call while camera is active (paths, grid)
void Debug_DrawHUD();                            // call after Pause_Draw (screen-space text)
void Debug_Unload();
void Debug_DrawCircleWorld(AEVec2 pos, f32 radius);
void Debug_DrawAttackRadius(AEVec2 pos, f32 radius);
void Debug_DrawLOS(AEVec2 start, AEVec2 end, bool visible);
