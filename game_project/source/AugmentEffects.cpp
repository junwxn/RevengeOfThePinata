/*************************************************************************
@file		AugmentEffects.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the function definitions for managing augment
            effects, including their initialization, updating, rendering,
            and transitioning between states.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"
#include "AugmentEffects.h"
#include "AugmentData.h"
#include "EventSystem.h"
#include "Player.h"
#include "Enemy.h"
#include "Utils.h"
#include "MathFunctions.h"

// ---- Internal state ----
static Player* s_player = nullptr;

// Shield Dash
static bool g_shieldActive = false;
static float g_shieldTimer = 0.0f;
static int g_shieldFrame = 0;
static float g_shieldFrameTimer = 0.0f;

// Poison Trail
struct PoisonCloud {
    float x, y;
    float lifetime;

    int frame = 0;
    float frameTimer = 0.0f;
};
static std::vector<PoisonCloud> g_poisonClouds;
static const int MAX_POISON_CLOUDS = 50;

// Speed Boost (shared between Dash Speed Boost and Attack Speed Boost)
static float g_speedBoostTimer = 0.0f;

// Meshes / Textures
static AEGfxVertexList* s_poisonMesh = nullptr;
static AEGfxVertexList* s_markMesh = nullptr;
static AEGfxTexture* s_poisonTexture = nullptr;

static AEGfxVertexList* s_shieldMesh = nullptr;
static AEGfxTexture* s_shieldTexture = nullptr;

// Poison sprite settings
static constexpr int   POISON_FRAME_COUNT = 8;
static constexpr float POISON_FRAME_DURATION = 0.08f;
static constexpr float POISON_CLOUD_SIZE = 80.0f;

// Shield sprite settings
static constexpr int   SHIELD_FRAME_COUNT = 8;
static constexpr int   SHIELD_ROW_COUNT = 8;
static constexpr float SHIELD_FRAME_DURATION = 0.0625f;
static constexpr float SHIELD_DRAW_SIZE = 140.0f;

// Pre-dash position for poison trail interpolation
static float s_preDashX = 0.0f;
static float s_preDashY = 0.0f;

// ---- Public query ----
bool AugmentEffects_IsShieldActive() {
    return g_shieldActive;
}

// ---- Init ----
void AugmentEffects_Init(Player* player) {
    s_player = player;

    g_shieldActive = false;
    g_shieldTimer = 0.0f;
    g_shieldFrame = 0;
    g_shieldFrameTimer = 0.0f;

    g_poisonClouds.clear();
    g_speedBoostTimer = 0.0f;

    // Free any existing meshes/textures before creating new ones
    if (s_poisonMesh) { AEGfxMeshFree(s_poisonMesh); s_poisonMesh = nullptr; }
    if (s_markMesh) { AEGfxMeshFree(s_markMesh);   s_markMesh = nullptr; }
    if (s_poisonTexture) { AEGfxTextureUnload(s_poisonTexture); s_poisonTexture = nullptr; }

    if (s_shieldMesh) { AEGfxMeshFree(s_shieldMesh); s_shieldMesh = nullptr; }
    if (s_shieldTexture) { AEGfxTextureUnload(s_shieldTexture); s_shieldTexture = nullptr; }

    // Poison cloud sprite sheet: 8 columns, 1 row
    s_poisonMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 1.0f);
    s_poisonTexture = AEGfxTextureLoad("Assets/Sprites/PoisonCloud_Spritesheet.png");

    // Shield bash sprite sheet: 8 columns, 8 rows
    s_shieldMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 8.0f);
    s_shieldTexture = AEGfxTextureLoad("Assets/Sprites/ShieldBash_Spritesheet2.png");

    s_markMesh = CreateRectMesh(0xFF0000);
}

// ---- Register callbacks based on current g_Augments ----
void AugmentEffects_Register() {
    // --- Set 1: Dash augments ---

    // Shield Dash
    if (g_Augments.Has(AugmentID::SHIELD_DASH)) {
        g_Events.Subscribe(GameEvent::ON_DASH, [](EventData const&) {
            g_shieldActive = true;
            g_shieldTimer = 0.5f; // 15 frames at 60fps

            g_shieldFrame = 0;
            g_shieldFrameTimer = 0.0f;
            });
    }

    // Poison Trail
    if (g_Augments.Has(AugmentID::POISON_TRAIL)) {
        g_Events.Subscribe(GameEvent::ON_DASH, [](EventData const& data) {
            // Interpolate from pre-dash position to current player position
            float dx = data.playerX - s_preDashX;
            float dy = data.playerY - s_preDashY;
            float dist = sqrtf(dx * dx + dy * dy);
            int numClouds = (int)(dist / 32.0f) + 1;

            for (int i = 0; i <= numClouds && (int)g_poisonClouds.size() < MAX_POISON_CLOUDS; ++i) {
                float t = (numClouds > 0) ? (float)i / numClouds : 0.0f;
                PoisonCloud cloud;
                cloud.x = s_preDashX + dx * t;
                cloud.y = s_preDashY + dy * t;
                cloud.lifetime = 3.0f;

                // Spawn randomly + offset
                cloud.frame = (i + rand() % POISON_FRAME_COUNT) % POISON_FRAME_COUNT;
                cloud.frameTimer = ((float)rand() / (float)RAND_MAX) * POISON_FRAME_DURATION;

                g_poisonClouds.push_back(cloud);
            }
            });
    }

    // Dash Speed Boost
    if (g_Augments.Has(AugmentID::DASH_SPEED_BOOST)) {
        g_Events.Subscribe(GameEvent::ON_DASH, [](EventData const&) {
            g_speedBoostTimer = 2.0f;
            });
    }

    // --- Set 2: Attack augments ---
    // Chain Attack: no callback needed, gated in Player.cpp

    // Damaging Mark
    if (g_Augments.Has(AugmentID::DAMAGING_MARK)) {
        g_Events.Subscribe(GameEvent::ON_ATTACK_HIT, [](EventData const& data) {
            if (data.targetEnemy) {
                Enemy* enemy = static_cast<Enemy*>(data.targetEnemy);
                if (!enemy->m_marked) {
                    enemy->m_markAccumulatedDamage = 0.0f;
                }
                enemy->m_marked = true;
                enemy->m_markTimer = 3.0f;
            }
            });
    }

    // Attack Speed Boost
    if (g_Augments.Has(AugmentID::ATTACK_SPEED_BOOST)) {
        g_Events.Subscribe(GameEvent::ON_ATTACK_HIT, [](EventData const&) {
            g_speedBoostTimer = 2.0f;
            });
    }

    // --- Set 3: Parry augments ---

    // More Parry Charges
    if (g_Augments.Has(AugmentID::MORE_PARRY_CHARGES)) {
        g_Events.Subscribe(GameEvent::ON_PARRY_SUCCESS, [](EventData const&) {
            // Combat system already gives +1, this adds +2 more for total +3
            if (s_player) {
                s_player->GainAttackCharge();
                s_player->GainAttackCharge();
            }
            });
    }

    // Faster Parry — applied once, not per-event
    if (g_Augments.Has(AugmentID::FASTER_PARRY)) {
        if (s_player) {
            s_player->SetBlockFrames(1, 7, 8);
        }
    }

    // Amplified Damage
    if (g_Augments.Has(AugmentID::AMPLIFIED_DAMAGE)) {
        g_Events.Subscribe(GameEvent::ON_PARRY_SUCCESS, [](EventData const& data) {
            if (data.targetEnemy) {
                Enemy* enemy = static_cast<Enemy*>(data.targetEnemy);
                enemy->m_damageAmplified = true;
                enemy->m_amplifyTimer = 5.0f;
                enemy->m_damageMultiplier = 2.0f;
            }
            });
    }
}

// ---- Update ----
void AugmentEffects_Update(float dt, Player& player, std::vector<std::unique_ptr<Enemy>>& wave) {
    // Shield Dash timer + animation
    if (g_shieldActive) {
        g_shieldTimer -= dt;

        g_shieldFrameTimer += dt;
        while (g_shieldFrameTimer >= SHIELD_FRAME_DURATION) {
            g_shieldFrameTimer -= SHIELD_FRAME_DURATION;
            if (g_shieldFrame < SHIELD_FRAME_COUNT - 1) {
                ++g_shieldFrame;
            }
        }

        if (g_shieldTimer <= 0.0f) {
            g_shieldActive = false;
            g_shieldTimer = 0.0f;
            g_shieldFrame = 0;
            g_shieldFrameTimer = 0.0f;
        }
    }

    // Poison Trail: tick lifetimes and damage enemies
    for (auto it = g_poisonClouds.begin(); it != g_poisonClouds.end();) {
        it->lifetime -= dt;
        if (it->lifetime <= 0.0f) {
            it = g_poisonClouds.erase(it);
            continue;
        }

        // Animate poison sprite
        it->frameTimer += dt;
        while (it->frameTimer >= POISON_FRAME_DURATION) {
            it->frameTimer -= POISON_FRAME_DURATION;
            it->frame = (it->frame + 1) % POISON_FRAME_COUNT;
        }

        // Damage enemies within radius
        for (auto& enemy : wave) {
            float dx = enemy->GetX() - it->x;
            float dy = enemy->GetY() - it->y;
            float distSq = dx * dx + dy * dy;
            if (distSq < 30.0f * 30.0f) {
                enemy->DeductHealth(15.0f * dt);
            }
        }
        ++it;
    }

    // Speed Boost timer
    if (g_speedBoostTimer > 0.0f) {
        g_speedBoostTimer -= dt;
        player.SetSpeedMultiplier(1.5f);
        if (g_speedBoostTimer <= 0.0f) {
            player.SetSpeedMultiplier(1.0f);
        }
    }

    // Damaging Mark: tick timers on each enemy
    for (auto& enemy : wave) {
        if (enemy->m_marked) {
            enemy->m_markTimer -= dt;
            if (enemy->m_markTimer <= 0.0f) {
                // Detonate: 10% max HP base + 20% of damage dealt while marked
                float detonateDmg = enemy->GetCombatStats().maxHealth * 0.1f
                    + enemy->m_markAccumulatedDamage * 0.2f;
                enemy->DeductHealth(detonateDmg);
                enemy->m_marked = false;
                enemy->m_markTimer = 0.0f;
                enemy->m_markAccumulatedDamage = 0.0f;
                enemy->m_markDetonating = true;
                enemy->m_markDetonateTimer = 0.3f;
            }
        }

        // Amplified Damage: tick timer
        if (enemy->m_damageAmplified) {
            enemy->m_amplifyTimer -= dt;
            if (enemy->m_amplifyTimer <= 0.0f) {
                enemy->m_damageAmplified = false;
                enemy->m_amplifyTimer = 0.0f;
                enemy->m_damageMultiplier = 1.0f;
            }
        }
    }

    // Store pre-dash position for poison trail (freeze during dash so ON_DASH gets the real start)
    if (!player.IsDashing()) {
        s_preDashX = player.GetX();
        s_preDashY = player.GetY();
    }
}

// ---- Draw ----
void AugmentEffects_Draw(float camX, float camY) {
    (void)camX;
    (void)camY;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Draw poison clouds
    if (s_poisonMesh && s_poisonTexture) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        for (auto& cloud : g_poisonClouds) {
            float alpha = cloud.lifetime / 3.0f;
            if (alpha < 0.0f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;

            float u = cloud.frame * (1.0f / 8.0f);
            float v = 0.0f;

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
            AEGfxSetTransparency(alpha);
            AEGfxTextureSet(s_poisonTexture, u, v);

            AEMtx33 scale, rot, trans, final;
            AEMtx33Scale(&scale, POISON_CLOUD_SIZE, POISON_CLOUD_SIZE);
            AEMtx33Rot(&rot, 0.0f);
            AEMtx33Trans(&trans, cloud.x, cloud.y);

            AEMtx33Concat(&final, &rot, &scale);
            AEMtx33Concat(&final, &trans, &final);

            AEGfxSetTransform(final.m);
            AEGfxMeshDraw(s_poisonMesh, AE_GFX_MDM_TRIANGLES);
        }
    }

    // Draw shield bash effect while active
    if (g_shieldActive && s_player && s_shieldMesh && s_shieldTexture) {
        float alpha = g_shieldTimer / 0.25f;
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;

        // Using row 0 for now
        float u = g_shieldFrame * (1.0f / 8.0f);
        float v = 0.0f;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
        AEGfxSetTransparency(alpha);
        AEGfxTextureSet(s_shieldTexture, u, v);

        AEMtx33 scale, rot, trans, final;
        AEMtx33Scale(&scale, SHIELD_DRAW_SIZE, SHIELD_DRAW_SIZE);
        AEMtx33Rot(&rot, 0.0f);
        AEMtx33Trans(&trans, s_player->GetX(), s_player->GetY());

        AEMtx33Concat(&final, &rot, &scale);
        AEMtx33Concat(&final, &trans, &final);

        AEGfxSetTransform(final.m);
        AEGfxMeshDraw(s_shieldMesh, AE_GFX_MDM_TRIANGLES);
    }

    // Draw mark indicators on enemies (small red diamond above marked enemies)
    // This requires iterating enemies, but we don't have the wave here.
    // Instead, mark drawing is done inline in the level Draw via the enemy's own Draw
    // or we skip it for simplicity since we can't access the wave from Draw().
}

// ---- Free ----
void AugmentEffects_Free() {
    s_player = nullptr;
    g_shieldActive = false;
    g_shieldTimer = 0.0f;
    g_shieldFrame = 0;
    g_shieldFrameTimer = 0.0f;

    g_poisonClouds.clear();
    g_speedBoostTimer = 0.0f;

    if (s_poisonMesh) {
        AEGfxMeshFree(s_poisonMesh);
        s_poisonMesh = nullptr;
    }
    if (s_markMesh) {
        AEGfxMeshFree(s_markMesh);
        s_markMesh = nullptr;
    }
    if (s_poisonTexture) {
        AEGfxTextureUnload(s_poisonTexture);
        s_poisonTexture = nullptr;
    }

    if (s_shieldMesh) {
        AEGfxMeshFree(s_shieldMesh);
        s_shieldMesh = nullptr;
    }
    if (s_shieldTexture) {
        AEGfxTextureUnload(s_shieldTexture);
        s_shieldTexture = nullptr;
    }
}