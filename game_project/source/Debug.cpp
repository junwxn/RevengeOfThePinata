#include "pch.h"
#include "Debug.h"
#include "Player.h"
#include "Enemy.h"
#include "camera.h"
#include "Map.h"
#include "Utils.h"
#include "GameStateManager.h"
#include "Raycast.h"
#include <cstdio>

// --- File-scoped statics ---
static s8  debugFont       = -1;
static AEGfxVertexList* debugCircleMesh = nullptr;
static AEGfxVertexList* debugRectMesh   = nullptr;
static AEGfxVertexList* debugDiamondMesh = nullptr;
static AEGfxVertexList* debugRingMesh = nullptr;
static AEGfxVertexList* debugAimLineMesh = nullptr;

static bool s_showHUD           = false;   // F1
static bool s_showCollisionGrid = false;   // F2
static bool s_showEnemyDebug = false;   // F3
static bool s_showPlayerCombatDebug = false;   // F4

static DebugContext s_ctx = {};

// --- Helpers ---
static const char* PlayerStateStr(PlayerState st) {
    switch (st) {
        case PlayerState::STATE_IDLE:   return "IDLE";
        case PlayerState::STATE_MOVING: return "MOVING";
        case PlayerState::STATE_ATTACK: return "ATTACK";
        case PlayerState::STATE_BLOCK:  return "BLOCK";
        case PlayerState::STATE_PARRY:  return "PARRY";
        case PlayerState::STATE_DEAD:   return "DEAD";
        default: return "???";
    }
}

static const char* GameStateStr(int gs) {
    switch (gs) {
        case GS_MAINMENU:  return "MAINMENU";
        case GS_TUTORIAL:  return "TUTORIAL";
        case GS_LEVEL1:    return "LEVEL1";
        case GS_LEVEL2:    return "LEVEL2";
        case GS_LEVEL3:    return "LEVEL3";
        case GS_BOSSLEVEL: return "BOSSLEVEL";
        case GS_VICTORY:   return "VICTORY";
        case GS_GAMEOVER:  return "GAMEOVER";
        default: return "???";
    }
}

// --- Public API ---

void Debug_Load() {
    debugFont       = AEGfxCreateFont("Assets/fonts/liberation-mono.ttf", 30);
    debugCircleMesh = CreateCircleMesh(1.0f, 32, 0xFFFFFFFF);
    debugRectMesh = CreateRectMesh(0xFFFFFFFF);
    debugRingMesh = CreateRingMesh(32, 0.01f);


    // Diamond mesh for isometric tile overlay
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f,
                 0.0f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f,
                 0.5f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(-0.5f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f,
                 0.5f, 0.0f, 0xFFFFFFFF, 0.0f, 0.0f,
                 0.0f,-0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    debugDiamondMesh = AEGfxMeshEnd();

    debugAimLineMesh = CreateLineMesh(200.0f, 0xFFFF35FF); // yellow aim line (attack range = 200)
}

void Debug_Init() {
    s_showHUD           = false;
    s_showCollisionGrid = false;
    s_showEnemyDebug = false;
    s_showPlayerCombatDebug = false;
    s_ctx               = {};
}

void Debug_Register(const DebugContext& ctx) {
    s_ctx = ctx;
}

void Debug_Update() {
    if (AEInputCheckTriggered(AEVK_F1))
        s_showHUD = !s_showHUD;
    if (AEInputCheckTriggered(AEVK_F2))
        s_showCollisionGrid = !s_showCollisionGrid;
    if (AEInputCheckTriggered(AEVK_F3))
        s_showEnemyDebug = !s_showEnemyDebug;
    if (AEInputCheckTriggered(AEVK_F4))
        s_showPlayerCombatDebug = !s_showPlayerCombatDebug;
}

// -----------------------------------------------------------------
// World-space overlays (call while camera is still active)
// -----------------------------------------------------------------
void Debug_DrawWorld(float camX, float camY) {
    if (!s_showHUD && !s_showCollisionGrid && !s_showEnemyDebug && !s_showPlayerCombatDebug) return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // --- Player attack/parry sector (F4) ---
    if (s_showPlayerCombatDebug && s_ctx.player) {
        AEVec2 playerPos{ s_ctx.player->GetX(), s_ctx.player->GetY() };

        const float range = s_ctx.player->GetAttackRange();

        Debug_DrawAttackRadius(playerPos, range);

        // Yellow aim line
        float lineAngle = s_ctx.player->IsAttacking() ? s_ctx.player->GetCurrentAngle() : s_ctx.player->GetAimAngle();
        DrawMesh(debugAimLineMesh, 1.0f, 5.0f, playerPos.x, playerPos.y, lineAngle, 255, 255, 53, 255);

        if (s_ctx.player->GetParryStatus() || s_ctx.player->GetBlockStatus()) {
            const float startAngle = s_ctx.player->GetStartAngle();
            const float endAngle = s_ctx.player->GetEndAngle();

            // Fixed parry swing sector
            Debug_DrawSwingSector(playerPos, range, startAngle, endAngle, 255, 60, 60, 220);
        }
        else {
            // Fallback aim cone when not parrying/blocking
            const float aimAngle = s_ctx.player->GetAimAngle();
            const float halfAngleRad = AEDegToRad(30.0f);

            Debug_DrawCone(playerPos, range, aimAngle, halfAngleRad, 255, 220, 80, 180);
        }
    }

    // --- Enemy LOS / Attack Range / Hitbox (F3) ---
    if (s_showEnemyDebug && s_ctx.player && s_ctx.map) {
        AEVec2 playerPos{ s_ctx.player->GetX(), s_ctx.player->GetY() };

        for (int w = 0; w < s_ctx.waveCount; ++w) {
            if (!s_ctx.waves[w]) continue;

            for (auto const& ep : *s_ctx.waves[w]) {
                if (!ep->GetIsAlive()) continue;
                AEVec2 enemyPos{ ep->GetX(), ep->GetY() };

                Debug_DrawCircleWorld(enemyPos, ep->GetSize());
                Debug_DrawAttackRadius(enemyPos, ep->GetAttackRange());

                bool los = HasLineOfSight_Grid(enemyPos, playerPos, *s_ctx.map);

                Debug_DrawLOS(enemyPos, playerPos, los);
            }
        }
    }

    // --- Collision grid (F2) ---
    if (s_showCollisionGrid && s_ctx.map) {
        unsigned mapW = s_ctx.map->GetMapWidth();
        unsigned mapH = s_ctx.map->GetMapHeight();

        for (unsigned row = 0; row < mapH; ++row) {
            for (unsigned col = 0; col < mapW; ++col) {
                if (s_ctx.map->IsWalkable(static_cast<int>(col), static_cast<int>(row)))
                    continue;

                AEVec2 wp = s_ctx.map->TMXToWorld(static_cast<int>(col), static_cast<int>(row));

                // Frustum cull
                if (fabsf(wp.x - camX) > 900.0f || fabsf(wp.y - camY) > 600.0f)
                    continue;

                DrawMesh(debugDiamondMesh, GRID_W, GRID_H,
                         wp.x, wp.y, 0.0f, 255, 30, 30, 80);
            }
        }
    }

    // --- Enemy path lines (F1) ---
    if (s_showHUD) {
        for (int w = 0; w < s_ctx.waveCount; ++w) {
            if (!s_ctx.waves[w]) continue;
            for (auto const& ep : *s_ctx.waves[w]) {
                if (!ep->GetIsAlive() || !ep->GetHasValidPath()) continue;

                auto const& path = ep->GetPath();
                int idx = ep->GetPathIndex();

                for (int i = 0; i < static_cast<int>(path.size()); ++i) {
                    // Line between consecutive waypoints
                    if (i > 0) {
                        float dx  = path[i].x - path[i - 1].x;
                        float dy  = path[i].y - path[i - 1].y;
                        float len = sqrtf(dx * dx + dy * dy);
                        float angle = atan2f(dy, dx);
                        DrawMesh(debugRectMesh, len, 2.0f,
                                 path[i - 1].x, path[i - 1].y, angle,
                                 255, 200, 0, 120);
                    }

                    // Waypoint dot
                    float dotSz = 5.0f;
                    float r = 255, g = 255, b = 0, a = 180; // yellow
                    if (i == idx) { r = 0; g = 255; b = 0; a = 255; dotSz = 8.0f; } // green = current
                    DrawMesh(debugCircleMesh, dotSz, dotSz,
                             path[i].x, path[i].y, 0.0f,
                             r, g, b, a);
                }
            }
        }
    }
}

// -----------------------------------------------------------------
// Screen-space HUD text (call after Pause_Draw)
// -----------------------------------------------------------------
void Debug_DrawHUD() {
    if (!s_showHUD) return;

    // Camera position needed for enemy label coordinate conversion
    float camX = s_ctx.camera ? s_ctx.camera->GetX() : 0.0f;
    float camY = s_ctx.camera ? s_ctx.camera->GetY() : 0.0f;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    // Dark background panel (top-left of screen, offset by camera to stay fixed)
    DrawMesh(debugRectMesh, 600, 450, camX - 800, camY + 270, 0.0f, 0, 0, 0, 160);

    char buf[256];
    float textX = -0.98f;
    float textY =  0.92f;
    const float lineH = 0.06f;
    const float scale = 0.6f;

    // FPS
    f64 frameTime = AEFrameRateControllerGetFrameTime();
    float fps = (frameTime > 0.0001) ? static_cast<float>(1.0 / frameTime) : 0.0f;
    snprintf(buf, sizeof(buf), "FPS: %.0f  (%.2fms)", fps, frameTime * 1000.0);
    AEGfxPrint(debugFont, buf, textX, textY, scale, 0.0f, 1.0f, 0.0f, 1.0f);
    textY -= lineH;

    // Game state
    snprintf(buf, sizeof(buf), "State: %s  Level: %s",
             GameStateStr(current), s_ctx.levelName ? s_ctx.levelName : "?");
    AEGfxPrint(debugFont, buf, textX, textY, scale, 0.8f, 0.8f, 0.8f, 1.0f);
    textY -= lineH;

    // Camera
    snprintf(buf, sizeof(buf), "Cam: (%.0f, %.0f)", camX, camY);
    AEGfxPrint(debugFont, buf, textX, textY, scale, 0.8f, 0.8f, 0.8f, 1.0f);
    textY -= lineH;

    // --- Player ---
    if (s_ctx.player) {
        const Player& p = *s_ctx.player;
        textY -= lineH * 0.5f;
        AEGfxPrint(debugFont, "--- PLAYER ---", textX, textY, scale, 1.0f, 1.0f, 0.0f, 1.0f);
        textY -= lineH;

        snprintf(buf, sizeof(buf), "Pos: (%.1f, %.1f)", p.GetX(), p.GetY());
        AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 1.0f, 1.0f, 1.0f);
        textY -= lineH;

        if (s_ctx.map) {
            GridPos gp = s_ctx.map->WorldToTMX(p.GetX(), p.GetY());
            snprintf(buf, sizeof(buf), "TMX: (col=%d, row=%d)", gp.col, gp.row);
            AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 1.0f, 1.0f, 1.0f);
            textY -= lineH;
        }

        auto stats = p.GetCombatStats();
        snprintf(buf, sizeof(buf), "HP: %.0f / %.0f", stats.health, stats.maxHealth);
        AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 0.3f, 0.3f, 1.0f);
        textY -= lineH;

        snprintf(buf, sizeof(buf), "State: %s", PlayerStateStr(p.GetState()));
        AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 1.0f, 1.0f, 1.0f);
        textY -= lineH;

        snprintf(buf, sizeof(buf), "Charges: %d  Dash: %d/%d",
                 p.GetAttackCharges(), p.GetDashCharges(), p.GetMaxDashCharges());
        AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 1.0f, 1.0f, 1.0f);
        textY -= lineH;

        snprintf(buf, sizeof(buf), "Aim: %.1f deg  Speed: %.0f",
                 p.GetAimAngle() * (180.0f / 3.14159265f), p.GetSpeed());
        AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 1.0f, 1.0f, 1.0f);
        textY -= lineH;

        auto flags = p.GetCombatFlag();
        snprintf(buf, sizeof(buf), "Stun:%d Blk:%d Parry:%d Hit:%d",
                 flags.stunned, flags.blockOn, flags.parryOn, flags.attackHit);
        AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 0.6f, 0.0f, 1.0f);
        textY -= lineH;
    }

    // --- Enemy count ---
    int totalEnemies = 0;
    for (int w = 0; w < s_ctx.waveCount; ++w)
        if (s_ctx.waves[w]) totalEnemies += static_cast<int>(s_ctx.waves[w]->size());
    snprintf(buf, sizeof(buf), "Enemies alive: %d", totalEnemies);
    AEGfxPrint(debugFont, buf, textX, textY, scale, 1.0f, 0.5f, 0.5f, 1.0f);
    textY -= lineH;

    // --- Per-enemy floating labels ---
    for (int w = 0; w < s_ctx.waveCount; ++w) {
        if (!s_ctx.waves[w]) continue;
        for (auto const& ep : *s_ctx.waves[w]) {
            float sx = (ep->GetX() - camX) / 800.0f;
            float sy = (ep->GetY() - camY + 40.0f) / 450.0f;

            if (sx < -1.2f || sx > 1.2f || sy < -1.2f || sy > 1.2f) continue;

            auto es = ep->GetCombatStats();
            const char* st = "idle";
            if (ep->IsAttacking())     st = "ATK";
            else if (ep->IsWindingUp()) st = "WIND";
            else if (ep->IsStunned())  st = "STUN";
            else if (ep->IsParried())  st = "PARRY";

            snprintf(buf, sizeof(buf), "HP:%.0f %s D:%.0f P:%d/%d",
                     es.health, st, ep->GetDistMag(),
                     ep->GetPathIndex(),
                     static_cast<int>(ep->GetPath().size()));

            AEGfxPrint(debugFont, buf, sx - 0.15f, sy,
                       0.45f, 1.0f, 0.7f, 0.7f, 1.0f);
        }
    }

    // Toggle hints (bottom-right)
    AEGfxPrint(debugFont, "[F1] HUD  [F2] Grid  [F3] Enemy Debug  [F4] Player Combat",
        0.20f, -0.95f, 0.5f, 0.5f, 0.5f, 0.5f, 1.0f);
}

void Debug_Unload() {
    if (debugCircleMesh) { AEGfxMeshFree(debugCircleMesh); debugCircleMesh = nullptr; }
    if (debugRectMesh)   { AEGfxMeshFree(debugRectMesh);   debugRectMesh   = nullptr; }
    if (debugDiamondMesh) { AEGfxMeshFree(debugDiamondMesh);   debugDiamondMesh = nullptr; }
    if (debugRingMesh){ AEGfxMeshFree(debugRingMesh); debugRingMesh = nullptr; }
    if (debugAimLineMesh) { AEGfxMeshFree(debugAimLineMesh); debugAimLineMesh = nullptr; }
    if (debugFont >= 0)  { AEGfxDestroyFont(debugFont);    debugFont       = -1; }
}

// Helper fucntions --------------------------------------------------------------------------------
// -----------------------------------------------------------------
// Enemy hitbox, for after adding sprite
// -----------------------------------------------------------------
void Debug_DrawCircleWorld(AEVec2 pos, f32 radius) {
    DrawMesh(debugCircleMesh, radius, radius, pos.x, pos.y, 0.0f, 125, 255, 252, 255);
}

// -----------------------------------------------------------------
// Enemy attack radius
// -----------------------------------------------------------------
void Debug_DrawAttackRadius(AEVec2 pos, f32 radius) {
    //DrawMesh(debugCircleMesh, radius, radius, pos.x, pos.y, 0.0f, 255, 44, 44, 100); // full circle
    DrawMesh(debugRingMesh, radius, radius, pos.x, pos.y, 0.0f, 0, 0, 0, 180); // outline circle
}

// -----------------------------------------------------------------
// Enemy attack radius
// -----------------------------------------------------------------
void Debug_DrawLOS(AEVec2 start, AEVec2 end, bool visible) {
    const f32 thickness = 3.0f;

    // true = red, false = light gray
    f32 r = visible ? 255.f : 200.f;
    f32 g = visible ? 60.f : 200.f;
    f32 b = visible ? 60.f : 200.f;
    f32 a = 180.f;

    f32 dx = end.x - start.x;
    f32 dy = end.y - start.y;
    f32 len = sqrtf(dx * dx + dy * dy);

    if (len < 0.001f) return;

    f32 angle = atan2f(dy, dx);
    DrawMesh(debugRectMesh, len, thickness, start.x, start.y, angle, r, g, b, a);
}

// -----------------------------------------------------------------
// Player attack cone
// -----------------------------------------------------------------
void Debug_DrawSwingSector(AEVec2 origin, float range, float startAngle, float endAngle,
    f32 r, f32 g, f32 b, f32 a)
{
    AEVec2 startEnd{
        origin.x + cosf(startAngle) * range,
        origin.y + sinf(startAngle) * range
    };

    AEVec2 endEnd{
        origin.x + cosf(endAngle) * range,
        origin.y + sinf(endAngle) * range
    };

    const float thickness = 3.0f;

    auto drawLine = [&](AEVec2 const& from, AEVec2 const& to)
        {
            const float dx = to.x - from.x;
            const float dy = to.y - from.y;
            const float len = sqrtf(dx * dx + dy * dy);
            if (len <= 0.001f)
                return;

            const float angle = atan2f(dy, dx);
            DrawMesh(debugRectMesh, len, thickness, from.x, from.y, angle, r, g, b, a);
        };

    drawLine(origin, startEnd);
    drawLine(origin, endEnd);
    drawLine(startEnd, endEnd);
}

void Debug_DrawCone(AEVec2 origin, float range, float centerAngle, float halfAngleRad,
    f32 r, f32 g, f32 b, f32 a)
{
    const float leftAngle = centerAngle + halfAngleRad;
    const float rightAngle = centerAngle - halfAngleRad;

    AEVec2 leftEnd{
        origin.x + cosf(leftAngle) * range,
        origin.y + sinf(leftAngle) * range
    };

    AEVec2 rightEnd{
        origin.x + cosf(rightAngle) * range,
        origin.y + sinf(rightAngle) * range
    };

    const float thickness = 3.0f;

    auto drawLine = [&](AEVec2 const& from, AEVec2 const& to)
        {
            const float dx = to.x - from.x;
            const float dy = to.y - from.y;
            const float len = sqrtf(dx * dx + dy * dy);
            if (len <= 0.001f)
                return;

            const float angle = atan2f(dy, dx);
            DrawMesh(debugRectMesh, len, thickness, from.x, from.y, angle, r, g, b, a);
        };

    drawLine(origin, leftEnd);
    drawLine(origin, rightEnd);
    drawLine(leftEnd, rightEnd);
}