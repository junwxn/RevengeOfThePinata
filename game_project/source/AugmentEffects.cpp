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

// Poison Trail
struct PoisonCloud {
    float x, y;
    float lifetime;
};
static std::vector<PoisonCloud> g_poisonClouds;
static const int MAX_POISON_CLOUDS = 50;

// Speed Boost (shared between Dash Speed Boost and Attack Speed Boost)
static float g_speedBoostTimer = 0.0f;

// Meshes
static AEGfxVertexList* s_poisonMesh = nullptr;
static AEGfxVertexList* s_markMesh = nullptr;

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
    g_poisonClouds.clear();
    g_speedBoostTimer = 0.0f;

    // Free any existing meshes before creating new ones (prevents leaks on restart)
    if (s_poisonMesh) { AEGfxMeshFree(s_poisonMesh); s_poisonMesh = nullptr; }
    if (s_markMesh)   { AEGfxMeshFree(s_markMesh);   s_markMesh   = nullptr; }

    s_poisonMesh = CreateCircleMesh(1.0f, 16, 0x00FF00);
    s_markMesh = CreateRectMesh(0xFF0000);
}

// ---- Register callbacks based on current g_Augments ----
void AugmentEffects_Register() {
    // --- Set 1: Dash augments ---

    // Shield Dash
    if (g_Augments.Has(AugmentID::SHIELD_DASH)) {
        g_Events.Subscribe(GameEvent::ON_DASH, [](EventData const&) {
            g_shieldActive = true;
            g_shieldTimer = 0.25f; // 15 frames at 60fps
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
    // Shield Dash timer
    if (g_shieldActive) {
        g_shieldTimer -= dt;
        if (g_shieldTimer <= 0.0f) {
            g_shieldActive = false;
        }
    }

    // Poison Trail: tick lifetimes and damage enemies
    for (auto it = g_poisonClouds.begin(); it != g_poisonClouds.end();) {
        it->lifetime -= dt;
        if (it->lifetime <= 0.0f) {
            it = g_poisonClouds.erase(it);
            continue;
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

    // Store pre-dash position for next frame's poison trail
    s_preDashX = player.GetX();
    s_preDashY = player.GetY();
}

// ---- Draw ----
void AugmentEffects_Draw() {
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);

    // Draw poison clouds
    for (auto& cloud : g_poisonClouds) {
        float alpha = (cloud.lifetime / 3.0f) * 150.0f;
        DrawMesh(s_poisonMesh, 30.0f, 30.0f, cloud.x, cloud.y, 0.0f, 0, 200, 50, (int)alpha);
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
}
