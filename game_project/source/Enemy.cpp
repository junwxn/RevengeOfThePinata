#include "pch.h"

#include "Enemy.h"
#include "Player.h"
#include "Utils.h"
#include "Map.h"
#include "Raycast.h"
#include "Sprite.h"
#include "Shadow.h"
#include "Combat.h"
#include "Audio.h"
#include <queue>

// Helper Functions
namespace {
    static f32 EaseInOutSine(f32 t)
        {
        t = AEClamp(t, 0.0f, 1.0f);
        return 0.5f - 0.5f * cosf(t * PI);
    }
}

std::ostream& operator<<(std::ostream& os, AEVec2 vector) {
    return os << vector.x << " " << vector.y << std::endl;
}

// ---------------------
// | Base Class: Enemy |
// ---------------------
// Base constructor
Enemy::Enemy(AEVec2 pos, f32 size, f32 hp, f32 speed)
    : m_pos{ pos }, m_hp{ hp }, m_speed{ speed }, m_size{ size } {}

// Base destructor 
Enemy::~Enemy() {
    if (m_enemyMesh) {
        AEGfxMeshFree(m_enemyMesh);
        m_enemyMesh = nullptr;
    }

    if (m_AttackRangeMesh) {
        AEGfxMeshFree(m_AttackRangeMesh);
        m_AttackRangeMesh = nullptr;
    }

    if (m_enemyHealthBarMesh) {
        AEGfxMeshFree(m_enemyHealthBarMesh);
        m_enemyHealthBarMesh = nullptr;
    }

    if (m_markMesh) {
        AEGfxMeshFree(m_markMesh);
        m_markMesh = nullptr;
    }

    // Weapon
    if (m_BatMesh) {
        AEGfxMeshFree(m_BatMesh);
        m_BatMesh = nullptr;
    }
    if (m_BatTexture) {
        AEGfxTextureUnload(m_BatTexture);
        m_BatTexture = nullptr;
    }

    // Detonate Mark
    if (m_DetonateMesh) {
        AEGfxMeshFree(m_DetonateMesh);
        m_DetonateMesh = nullptr;
    }
    if (m_DetonateTexture) {
        AEGfxTextureUnload(m_DetonateTexture);
        m_DetonateTexture = nullptr;
    }
}

void Enemy::Init() {
    // Free any existing meshes before creating new ones (prevents leaks on re-init)
    if (m_AttackRangeMesh)    { AEGfxMeshFree(m_AttackRangeMesh);    m_AttackRangeMesh    = nullptr; }
    if (m_enemyMesh)          { AEGfxMeshFree(m_enemyMesh);          m_enemyMesh          = nullptr; }
    if (m_enemyHealthBarMesh) { AEGfxMeshFree(m_enemyHealthBarMesh); m_enemyHealthBarMesh = nullptr; }
    if (m_markMesh)           { AEGfxMeshFree(m_markMesh);           m_markMesh           = nullptr; }

    m_AttackRangeMesh = CreateAttackRangeMesh(m_AttackRange, 0xFF0000);
    m_enemyMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
    m_enemyHealthBarMesh = CreateRectMesh(0xAEF359);
    m_markMesh = CreateRectMesh(0xFFFFFF);

    m_EnemySprite.Sprite_Init();
    m_EnemySpriteSheet = m_EnemySprite.GetSpriteSheet();
    m_EnemyWindupSpriteSheet = m_EnemySprite.GetEnemyWindupSpriteSheet();
    m_EnemyAttackSpriteSheet = m_EnemySprite.GetEnemyAttackSpriteSheet();

    m_DasherSprite.Sprite_Init();
    m_DasherSpriteSheet = m_DasherSprite.GetSpriteSheet();
    m_DasherWindupSpriteSheet = m_DasherSprite.GetEnemyWindupSpriteSheet();
    m_DasherAttackSpriteSheet = m_DasherSprite.GetEnemyAttackSpriteSheet();

    if (m_BatMesh) { AEGfxMeshFree(m_BatMesh); m_BatMesh = nullptr; }
    if (m_BatTexture) { AEGfxTextureUnload(m_BatTexture); m_BatTexture = nullptr; }
    m_BatMesh = CreateBatMesh(0xFFFFFFFF);
    m_BatTexture = AEGfxTextureLoad("Assets/Sprites/BatBat.png");

    if (m_DetonateMesh) { AEGfxMeshFree(m_DetonateMesh); m_DetonateMesh = nullptr; }
    if (m_DetonateTexture) { AEGfxTextureUnload(m_DetonateTexture); m_DetonateTexture = nullptr; }
    m_DetonateMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 2.0f);
    m_DetonateTexture = AEGfxTextureLoad("Assets/Sprites/Detonate_Spritesheet.png");
}

void Enemy::BaseUpdate(f32 dt, Combat::System& combat, Player& player) {

    if (m_CombatStats.health <= 0) m_CombatFlags.isAlive = false;
    //std::cout << "BASE RUNNING" << std::endl;

    // --- Knockback with wall-bounce ---
    AEVec2 frameMove;
    AEVec2Scale(&frameMove, &m_KnockbackVelocity, dt);

    float knockbackSpeed = AEVec2Length(&m_KnockbackVelocity);

    EvaluateCurrentDirection();
    m_EnemySprite.Sprite_Update(dt);
    m_DasherSprite.Sprite_Update(dt);

    if (!m_pMap || knockbackSpeed < 1.0f) {
        // No map or negligible knockback — just apply raw move
        AEVec2Add(&m_pos, &m_pos, &frameMove);
    }
    else {
        float newX = m_pos.x + frameMove.x;
        float newY = m_pos.y + frameMove.y;

        if (!m_pMap->IsPositionBlocked(newX, newY, m_size)) {
            // Full move is clear
            m_pos.x = newX;
            m_pos.y = newY;
        }
        else {
            // Wall hit — check which axes are blocked and reflect them
            bool xBlocked = m_pMap->IsPositionBlocked(m_pos.x + frameMove.x, m_pos.y, m_size);
            bool yBlocked = m_pMap->IsPositionBlocked(m_pos.x, m_pos.y + frameMove.y, m_size);

            if (xBlocked) m_KnockbackVelocity.x *= -0.5f;
            if (yBlocked) m_KnockbackVelocity.y *= -0.5f;
            if (!xBlocked && !yBlocked) {
                // Corner case: both axes clear individually but diagonal blocked
                m_KnockbackVelocity.x *= -0.5f;
                m_KnockbackVelocity.y *= -0.5f;
            }

            // Wall-impact damage: only on a real slam (speed > 500)
            constexpr float WALL_DAMAGE_SPEED_THRESHOLD = 500.0f;
            if (knockbackSpeed > WALL_DAMAGE_SPEED_THRESHOLD) {
                float wallDamage = m_CombatStats.maxHealth * 0.1f;
                m_CombatStats.health -= wallDamage;
                m_healthDepletionPercentage += wallDamage * 3.0f;
            }

            // Use ResolveCollision to find a safe final position (prevents clipping)
            ResolveCollision(m_pos.x, m_pos.y, frameMove.x, frameMove.y, m_size, *m_pMap);
        }
    }

    AEVec2Scale(&m_KnockbackVelocity, &m_KnockbackVelocity, 0.85f);

    // Damaging Mark: tick detonation animation
    if (m_markDetonating) {
        m_markDetonateTimer -= dt;
        if (m_markDetonateTimer <= 0.0f) {
            m_markDetonating = false;
            m_markDetonateTimer = 0.0f;
        }
    }

    // Only use for single enemy
    if (AEInputCheckTriggered(AEVK_R)) {
        SetPosition(50.0f, 0.0f);
    }

    if (m_AttackCooldown <= 0.0f)
    {
        m_AttackCooldown = 0.0f; // Clamp
        m_AllowAttack = true;
    }

    // Start attack -------------------
    // Wind-up: charge before swinging
    if (m_EnableMelee &&
        m_CurrentState != EnemyState::STATE_DASH_WINDUP &&
        m_CurrentState != EnemyState::STATE_DASH &&
        !m_AttackActive && m_AllowAttack && !m_WindingUp &&
        m_CombatSystem.CanStartAttack_Enemy(player, *this))
    {
        m_WindingUp = true;
        m_WindUpTimer = m_WindUpDuration;
    }

    if (m_WindingUp) {
        m_WindUpTimer -= dt;
        if (m_WindUpTimer <= 0.0f) {
            m_WindingUp = false;
            m_WindUpTimer = 0.0f;
            StartAttack(this->m_AttackData);
        }
    }
    //std::cout << "m_AttackActive: " << m_AttackActive << std::endl;

    // Update attack ------------------
    if (m_AttackActive)
    {
        m_AttackCooldown = 2.0f;

        // For normalized value between 0.0 - 1.0 range
        // 0.0 = attack start
        // 0.5 = halfway through attack
        // 1.0 = attack complete
        m_AttackFrameAccumulator += dt;

        while (m_AttackFrameAccumulator >= m_CombatSystem.GetOneFPS() && m_AttackCurrentFrame != m_AttackTotalFrames) {
            ++m_AttackCurrentFrame;
            m_AttackFrameAccumulator -= static_cast<float>(m_CombatSystem.GetOneFPS());
        }

        float m_attackProgress{};

        if (m_AttackCurrentFrame < m_AttackStartUpFrames)
        {
            // Start-up Phase
        }
        else if (m_AttackCurrentFrame < m_AttackStartUpFrames + m_AttackActiveFrames)
        {
            // Active Phase
            int activeFrameIndex{ m_AttackCurrentFrame - m_AttackStartUpFrames }; // Gives the current active frame
            m_attackProgress = float(activeFrameIndex) / (m_AttackActiveFrames - 1);
            m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, m_attackProgress);
        }
        else
        {
            // Recovery Phase
        };

        if (m_attackProgress >= 1.0f)
        {
            m_AttackActive = false;
            m_attackProgress = 1.0f;
        }
    }
    else {
        if(!m_CombatFlags.stunned) m_AttackCooldown -= dt;
        m_AllowAttack = false;
    }

    //std::cout << "BASE ENDED" << std::endl;
}

void Enemy::DrawBat(float angle)
{
    if (!m_BatMesh || !m_BatTexture) return;

    static constexpr float BAT_WIDTH = 35.0f;
    static constexpr float BAT_LENGTH = 175.0f;

    float drawAngle = angle + PI * 0.5f;

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(m_BatTexture, 0.0f, 0.0f);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, BAT_WIDTH, BAT_LENGTH);
    AEMtx33Rot(&rotate, drawAngle);
    AEMtx33Trans(&translate, m_pos.x, m_pos.y);

    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(m_BatMesh, AE_GFX_MDM_TRIANGLES);
}

static void DrawDetonateSprite(AEGfxVertexList* mesh, AEGfxTexture* texture,
    float x, float y, float size,
    float timer, float totalDuration, int row)
{
    if (!mesh || !texture) return;

    float progress = 1.0f - (timer / totalDuration);
    progress = AEClamp(progress, 0.0f, 1.0f);

    int frame = static_cast<int>(progress * 7.0f);
    if (frame < 0) frame = 0;
    if (frame > 7) frame = 7;

    float u = frame * (1.0f / 8.0f);
    float v = row * (1.0f / 2.0f);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(texture, u, v);

    AEMtx33 scale, rot, trans, final;
    AEMtx33Scale(&scale, size, size);
    AEMtx33Rot(&rot, 0.0f);
    AEMtx33Trans(&trans, x, y);

    AEMtx33Concat(&final, &rot, &scale);
    AEMtx33Concat(&final, &trans, &final);

    AEGfxSetTransform(final.m);
    AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void Enemy::Draw() {
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();

    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Calculate isometric squashed height for drawing
        //f32 isoHeight = m_size * (GRID_H / GRID_W); // Squashed
        f32 isoHeight = m_size; // Normal
        float spriteScale = m_sizeMultiplier;
        float shadowY = m_pos.y;

        Shadow_Draw(m_pos.x, shadowY, m_size);

    // Draw Meshes --------------------
    // Enemy
    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
        // Flash red just before attacking (last 0.2s of wind-up)
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetEnemyAttackSpriteMesh(),
            m_EnemySprite.GetEnemyAttackSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    //else if (isAnyWindup) {
    //    // Winding up — tint yellow to show charging
    //    //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 200, 0, 255);
    //    DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
    //        m_EnemySprite.GetEnemyWindupSpriteMesh(),
    //        m_EnemySprite.GetEnemyWindupSpriteSheet(),
    //        m_EnemySprite.GetPixelScale(),
    //        m_EnemySprite.GetPixelScale(),
    //        m_pos.x, m_pos.y, 0.0f, spriteScale);

    //    DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
    //        m_DasherSprite.GetEnemyWindupSpriteMesh(),
    //        m_DasherSprite.GetEnemyWindupSpriteSheet(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_pos.x, m_pos.y, 0.0f, spriteScale);
    //}
    else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
        //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 0, 0, 255);
        //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 0, 0, 255);
        m_EnemySprite.SetEnemyAttackSingleFrame(0, 6);

        DrawTexture(m_EnemySprite,
            m_EnemySprite.GetEnemyAttackSpriteMesh(),
            m_EnemySprite.GetEnemyAttackSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else {
        //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 44, 255, 255, 255);
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetSpriteMesh(),
            m_EnemySprite.GetSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }

    // Enemy sword
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    //DrawMesh(m_AttackRangeMesh, 1.0f , 5.0f, m_pos.x, m_pos.y, swordAngle,
    //    44, 255, 255, 255);
    if(m_AttackActive) DrawBat(swordAngle);

    // Enemy health bar
    f32 barWidth = m_size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth);
    f32 barHeight = m_size / 3.0f;
    f32 dbarWidth = m_size * 2.0f * AEClamp((m_CombatStats.health / m_CombatStats.maxHealth) + (m_healthDepletionPercentage / 100.0f), 0.0f, 1.0f);

    f32 dRate = 100.0f * dt;
    if (m_healthDepletionPercentage > 0.0f) {
        m_healthDepletionPercentage -= dRate;
        if (m_healthDepletionPercentage < 0.0f) {
            m_healthDepletionPercentage = 0.0f;
        }
    }

    DrawMesh(m_enemyHealthBarMesh, dbarWidth, barHeight, m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f, 0.0f, 255, 175, 65, 255); // Depleting bar
    DrawMesh(m_enemyHealthBarMesh, barWidth, barHeight, m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f, 0.0f, 210, 70, 75, 255); // Instant bar

    // Damaging Mark: draw dagger above marked/detonating enemies
    //if (m_marked && !m_markDetonating && m_markMesh) {
    //    // Hovering dagger with gentle bob
    //    float bobOffset = sinf(m_markTimer * 5.0f) * 4.0f;
    //    float daggerY = m_pos.y + m_size + 40.0f + bobOffset;
    //    DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 255, 255, 255);
    //}
    //else if (m_markDetonating && m_markMesh) {
    //    // Dagger drops from hover position down to enemy
    //    float t = m_markDetonateTimer / 0.3f; // 1.0 = top, 0.0 = enemy
    //    float hoverY = m_pos.y + m_size + 40.0f;
    //    float daggerY = m_pos.y + (hoverY - m_pos.y) * t;
    //    DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 80, 80, 255);
    //}

    // Damaging Mark visual
    if (m_marked) {
        if (!m_markDetonating) {
            // Hovering bomb sprite with gentle bob
            float bobOffset = sinf(m_markTimer * 5.0f) * 4.0f;
            float detonateY = m_pos.y + m_size + 40.0f + bobOffset;

            DrawDetonateSprite(
                m_DetonateMesh,
                m_DetonateTexture,
                m_pos.x,
                detonateY,
                55.0f,
                m_markTimer,
                3.0f,
                0   // first row = bomb
            );
        }
        else {
            DrawDetonateSprite(
                m_DetonateMesh,
                m_DetonateTexture,
                m_pos.x,
                m_pos.y + 15.0f,
                95.0f,
                m_markDetonateTimer,
                0.3f,
                1   // second row = explosion
            );
        }
    }

    

    // ENEMY SPRITE DRAWING
    //DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection), m_EnemySprite.GetSpriteMesh(), m_EnemySprite.GetSpriteSheet(), m_EnemySprite.GetPixelScale(),
    //            m_EnemySprite.GetPixelScale(), m_pos.x, m_pos.y, 0.0f);
    //DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection), m_EnemySprite.GetEnemyWindupSpriteMesh(), m_EnemySprite.GetEnemyWindupSpriteSheet(), m_EnemySprite.GetPixelScale(),
    //    m_EnemySprite.GetPixelScale(), m_pos.x, m_pos.y, 0.0f);
    //DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection), m_EnemySprite.GetEnemyAttackSpriteMesh(), m_EnemySprite.GetEnemyAttackSpriteSheet(), m_EnemySprite.GetPixelScale(),
    //    m_EnemySprite.GetPixelScale(), m_pos.x, m_pos.y, 0.0f);
}

void Enemy::StartAttack(Combat::CombatData::AttackData attackData) {
    m_CombatFlags.attackResolved = false;
    m_CombatFlags.blockResolved = false;
    m_CombatFlags.parryResolved = false;

    m_AttackActive = true;
    m_AllowAttack = false;
    m_AttackFrameAccumulator = 0.0f;
    m_AttackCurrentFrame = 0;

    // 60-degree cone
    m_CurrentAngle = m_AimAngle;
    m_StartAngle = m_AimAngle - AEDegToRad(attackData.startAngle);
    m_EndAngle = m_AimAngle + AEDegToRad(attackData.endAngle);
}

void Enemy::DamageInfo() {

}

// ---------------------------------------------------------------------------
// A* pathfinding helpers (file-local)
// ---------------------------------------------------------------------------
namespace {
    struct AStarNode {
        GridPos pos;
        float gCost;
        float fCost;
    };
    struct AStarCompare {
        bool operator()(AStarNode const& a, AStarNode const& b) const {
            return a.fCost > b.fCost; // min-heap
        }
    };
    float Heuristic(GridPos const& a, GridPos const& b) {
        int dx = std::abs(a.col - b.col);
        int dy = std::abs(a.row - b.row);
        return static_cast<float>((std::max)(dx, dy)) + 0.414f * static_cast<float>((std::min)(dx, dy));
    }
}

void Enemy::ComputePath(AEVec2 const& targetPos) {
    m_path.clear();
    m_pathIndex = 0;
    m_hasValidPath = false;
    m_lastTargetPos = targetPos;

    if (!m_pMap) return;

    // Compute clearance: how many extra cells around each position must be
    // walkable for this entity to fit through.  Normal enemies (25) → 0,
    // Boss (50) → 1.
    const float halfMin = (std::min)(GRID_W * 0.5f, GRID_H * 0.5f);
    int clearance = static_cast<int>(std::ceilf(m_size * 0.9f / halfMin)) - 1;
    if (clearance < 0) clearance = 0.5;

    GridPos start = m_pMap->WorldToTMX(m_pos.x, m_pos.y);
    GridPos goal  = m_pMap->WorldToTMX(targetPos.x, targetPos.y);

    // If goal is not walkable for this entity size, search outward
    if (!m_pMap->IsWalkableForSize(goal.col, goal.row, clearance)) {
        bool found = false;
        int searchRadius = clearance + 1;
        for (int ring = 1; ring <= searchRadius && !found; ++ring) {
            for (int dr = -ring; dr <= ring && !found; ++dr) {
                for (int dc = -ring; dc <= ring && !found; ++dc) {
                    if (std::abs(dr) != ring && std::abs(dc) != ring) continue;
                    GridPos alt{ goal.col + dc, goal.row + dr };
                    if (m_pMap->IsWalkableForSize(alt.col, alt.row, clearance)) {
                        goal = alt;
                        found = true;
                    }
                }
            }
        }
        if (!found) return;
    }

    // If start is not walkable for this entity size, search outward
    if (!m_pMap->IsWalkableForSize(start.col, start.row, clearance)) {
        bool found = false;
        int searchRadius = clearance + 1;
        for (int ring = 1; ring <= searchRadius && !found; ++ring) {
            for (int dr = -ring; dr <= ring && !found; ++dr) {
                for (int dc = -ring; dc <= ring && !found; ++dc) {
                    if (std::abs(dr) != ring && std::abs(dc) != ring) continue;
                    GridPos alt{ start.col + dc, start.row + dr };
                    if (m_pMap->IsWalkableForSize(alt.col, alt.row, clearance)) {
                        start = alt;
                        found = true;
                    }
                }
            }
        }
        if (!found) return;
    }

    if (start == goal) {
        m_path.push_back(targetPos);
        m_hasValidPath = true;
        return;
    }

    const int maxIter = static_cast<int>(m_pMap->GetMapWidth()) * static_cast<int>(m_pMap->GetMapHeight());

    std::priority_queue<AStarNode, std::vector<AStarNode>, AStarCompare> openSet;
    std::unordered_map<GridPos, float, GridPosHash> gScore;
    std::unordered_map<GridPos, GridPos, GridPosHash> cameFrom;

    gScore[start] = 0.0f;
    openSet.push({ start, 0.0f, Heuristic(start, goal) });

    int iterations = 0;
    bool found = false;

    static const int dc[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static const int dr[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    static const float costs[] = { 1.414f, 1.0f, 1.414f, 1.0f, 1.0f, 1.414f, 1.0f, 1.414f };

    while (!openSet.empty() && iterations < maxIter) {
        ++iterations;
        AStarNode current = openSet.top();
        openSet.pop();

        if (current.pos == goal) {
            found = true;
            break;
        }

        // Skip if we already found a better path to this node
        auto it = gScore.find(current.pos);
        if (it != gScore.end() && current.gCost > it->second)
            continue;

        for (int i = 0; i < 8; ++i) {
            GridPos neighbor{ current.pos.col + dc[i], current.pos.row + dr[i] };

            if (!m_pMap->IsWalkableForSize(neighbor.col, neighbor.row, clearance))
                continue;

            // Corner-cutting prevention for diagonals
            bool isDiagonal = (dc[i] != 0 && dr[i] != 0);
            if (isDiagonal) {
                if (!m_pMap->IsWalkableForSize(current.pos.col + dc[i], current.pos.row, clearance) ||
                    !m_pMap->IsWalkableForSize(current.pos.col, current.pos.row + dr[i], clearance))
                    continue;
            }

            float tentativeG = current.gCost + costs[i];
            auto nIt = gScore.find(neighbor);
            if (nIt == gScore.end() || tentativeG < nIt->second) {
                gScore[neighbor] = tentativeG;
                cameFrom[neighbor] = current.pos;
                openSet.push({ neighbor, tentativeG, tentativeG + Heuristic(neighbor, goal) });
            }
        }
    }

    if (!found) return;

    // Reconstruct path
    std::vector<GridPos> gridPath;
    GridPos cur = goal;
    while (cur != start) {
        gridPath.push_back(cur);
        cur = cameFrom[cur];
    }

    // Convert to world-space (reverse order so index 0 = first waypoint)
    m_path.reserve(gridPath.size());
    for (int i = static_cast<int>(gridPath.size()) - 1; i >= 0; --i) {
        m_path.push_back(m_pMap->TMXToWorld(gridPath[i].col, gridPath[i].row));
    }
    m_pathIndex = 0;
    m_hasValidPath = true;
}

// Line-of-sight check: can the entity walk from 'from' to 'to' without
// hitting a solid tile?  Steps along the line in small increments.
static bool HasLineOfSight(AEVec2 const& from, AEVec2 const& to,
                           float radius, const MapSystem& map)
{
    AEVec2 diff;
    diff.x = to.x - from.x;
    diff.y = to.y - from.y;
    float dist = AEVec2Length(&diff);
    if (dist < 1.0f) return true;

    float stepSize = (std::min)(5.0f, radius * 0.5f);
    float safeRadius = radius + 2.5f;
    int steps = static_cast<int>(dist / stepSize) + 1;
    for (int i = 1; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;
        float px = from.x + diff.x * t;
        float py = from.y + diff.y * t;
        if (map.IsPositionBlocked(px, py, safeRadius))
            return false;
    }
    return true;
}

AEVec2 Enemy::FollowPath() {
    if (!m_hasValidPath || m_pathIndex >= static_cast<int>(m_path.size()))
        return { 0.0f, 0.0f };

    // Advance past any waypoints we're already close to.
     float WAYPOINT_THRESHOLD = 1.0f;
    while (m_pathIndex < static_cast<int>(m_path.size())) {
        AEVec2 diff;
        AEVec2Sub(&diff, &m_path[m_pathIndex], &m_pos);
        if (AEVec2Length(&diff) >= WAYPOINT_THRESHOLD)
            break;
        ++m_pathIndex;
    }

    if (m_pathIndex >= static_cast<int>(m_path.size()))
        return { 0.0f, 0.0f };

    // Look-ahead: skip to the furthest visible waypoint so the enemy
    // doesn't try to hit exact tile centers at corners.
    if (m_pMap) {
        int bestIndex = m_pathIndex;
        int lookAhead = (std::min)(m_pathIndex + 4, static_cast<int>(m_path.size()) - 1);
        for (int i = lookAhead; i > m_pathIndex; --i) {
            if (HasLineOfSight(m_pos, m_path[i], m_size, *m_pMap)) {
                bestIndex = i;
                break;
            }
        }
        m_pathIndex = bestIndex;
    }

    AEVec2 dir;
    AEVec2Sub(&dir, &m_path[m_pathIndex], &m_pos);
    float dist = AEVec2Length(&dir);
    if (dist > 0.001f)
        AEVec2Normalize(&dir, &dir);
    return dir;
}

bool Enemy::NeedsPathRecalc(AEVec2 const& targetPos, f32 dt) {
    m_pathTimer -= dt;
    if (m_pathTimer <= 0.0f) {
        m_pathTimer = m_pathRecalcInterval;
        return true;
    }
    float dx = targetPos.x - m_lastTargetPos.x;
    float dy = targetPos.y - m_lastTargetPos.y;
    if (dx * dx + dy * dy > 100.0f * 100.0f)
        return true;
    return false;
}

void Enemy::MoveTowardTarget(AEVec2 const& targetPos, f32 dt) {
    if (!m_pMap) {
        // No map — direct seek
        m_pos.x += m_enemyToPlayerDir.x * m_speed * dt;
        m_pos.y += m_enemyToPlayerDir.y * m_speed * dt;
        return;
    }

    if (NeedsPathRecalc(targetPos, dt))
        ComputePath(targetPos);

    // Stuck detection: if we haven't moved much, force a full path recalculation
    float dx = m_pos.x - m_lastPos.x;
    float dy = m_pos.y - m_lastPos.y;
    if (dx * dx + dy * dy < STUCK_DIST_THRESHOLD * STUCK_DIST_THRESHOLD) {
        m_stuckTimer += dt;
        if (m_stuckTimer >= STUCK_TIME_THRESHOLD) {
            ComputePath(targetPos);
            m_stuckTimer = 0.0f;
        }
    }
    else {
        m_stuckTimer = 0.0f;
    }
    m_lastPos = m_pos;

    AEVec2 dir = FollowPath();
    float len = AEVec2Length(&dir);

    if (len < 0.001f) {
        if (!m_hasValidPath) {
            dir = m_enemyToPlayerDir;
        }
        else {
            m_pathTimer = 0.0f; // force path recalc next frame
            m_moveDir = { 0.0f, 0.0f };
            return;
        }
    }

    m_moveDir = dir;

    float velX = dir.x * m_speed * dt;
    float velY = dir.y * m_speed * dt;
    float prevX = m_pos.x, prevY = m_pos.y;
    ResolveCollision(m_pos.x, m_pos.y, velX, velY, m_size, *m_pMap);

    // If fully stuck, try each axis independently to unstick at corners
    if (m_pos.x == prevX && velX != 0.0f) {
        // X was blocked, let Y slide
        ResolveCollision(m_pos.x, m_pos.y, 0.0f, velY, m_size, *m_pMap);
    }
    if (m_pos.y == prevY && velY != 0.0f) {
        // Y was blocked, let X slide
        ResolveCollision(m_pos.x, m_pos.y, velX, 0.0f, m_size, *m_pMap);
    }
}

//------------------------
 //| Child Class: Walker |
 //-----------------------
void Walker::ChildUpdate(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies) {
    (void)combat;
    (void)enemies;

    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2 toPlayer;
    AEVec2Sub(&toPlayer, &playerPos, &m_pos);

    f32 distToPlayer = AEVec2Length(&toPlayer);
    if (distToPlayer > 0.001f) {
        AEVec2Scale(&m_enemyToPlayerDir, &toPlayer, 1.0f / distToPlayer);
    }

    // Point sword towards player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    const f32 desiredStopDistance = m_AttackRange * 0.6f;

    if (distToPlayer > desiredStopDistance) {
        MoveTowardTarget(playerPos, dt);
    }
    else {
        m_moveDir = { 0.0f, 0.0f };
    }
}

// -----------------------
// | Child Class: Dasher |
// -----------------------
Dasher::Dasher(AEVec2 pos, f32 size, f32 hp, f32 speed, f32 dashCD)
    : Enemy(pos, size, hp, speed), m_dashCD{ dashCD }
{
    // Stagger initial timers so grouped Dashers don't all lunge at once
    m_dashTimer = static_cast<f32>(rand()) / RAND_MAX * m_dashCD;
    m_CurrentState = EnemyState::STATE_IDLE;
}

void Dasher::StartDash(AEVec2 const& direction, f32 distToPlayer)
{
    m_WindingUp = false;
    m_WindUpTimer = 0.0f;
    m_AllowAttack = false;

    m_moveDir = direction;
    m_dashActive = true;
    m_dashFrameAccumulator = 0.0f;
    m_dashCurrentFrame = 0;

    m_dashDirX = direction.x;
    m_dashDirY = direction.y;

    // Land around the middle of attack range
    const f32 desiredEndDistance = m_AttackRange * m_dashEndRangeFactor;
    f32 actualDashDistance = distToPlayer - desiredEndDistance;

    actualDashDistance = AEClamp(actualDashDistance, 0.0f, m_dashDistance);

    m_dashTotalX = direction.x * actualDashDistance;
    m_dashTotalY = direction.y * actualDashDistance;

    m_CurrentState = EnemyState::STATE_DASH;
}

void Dasher::PerformDash(AEVec2 const& direction, f32 distToPlayer)
{
    StartDash(direction, distToPlayer);
}

void Dasher::ApplyDashStep(Player& player)
{
    (void)player;

    const int activeStart = m_dashStartFrames;

    int currentActiveFrame = m_dashCurrentFrame - activeStart;
    int previousActiveFrame = currentActiveFrame - 1;

    f32 prevLinearT = static_cast<f32>(previousActiveFrame) / static_cast<f32>(m_dashActiveFrames);
    f32 currLinearT = static_cast<f32>(currentActiveFrame) / static_cast<f32>(m_dashActiveFrames);

    prevLinearT = AEClamp(prevLinearT, 0.0f, 1.0f);
    currLinearT = AEClamp(currLinearT, 0.0f, 1.0f);

    f32 prevCurveT = EaseInOutSine(prevLinearT);
    f32 currCurveT = EaseInOutSine(currLinearT);

    f32 deltaT = currCurveT - prevCurveT;

    f32 dashVelX = m_dashTotalX * deltaT;
    f32 dashVelY = m_dashTotalY * deltaT;

    if (m_pMap)
    {
        const int steps = 8;
        f32 stepVelX = dashVelX / steps;
        f32 stepVelY = dashVelY / steps;

        for (int i = 0; i < steps; ++i)
        {
            f32 prevX = m_pos.x;
            f32 prevY = m_pos.y;

            ResolveCollision(m_pos.x, m_pos.y, stepVelX, stepVelY, m_size, *m_pMap);

            if (m_pos.x == prevX && m_pos.y == prevY)
            {
                m_dashActive = false;
                m_AllowAttack = true;
                m_CurrentState = EnemyState::STATE_IDLE;
                m_dashTimer = m_dashCD;
                return;
            }
        }
    }
    else
    {
        m_pos.x += dashVelX;
        m_pos.y += dashVelY;
    }
}

void Dasher::UpdateDash(f32 dt, Player& player)
{
    if (!m_dashActive)
        return;

    m_dashFrameAccumulator += dt;

    while (m_dashFrameAccumulator >= m_CombatSystem.GetOneFPS() &&
        m_dashCurrentFrame <= m_dashTotalFrames)
    {
        ++m_dashCurrentFrame;
        m_dashFrameAccumulator -= m_CombatSystem.GetOneFPS();

        if (m_dashCurrentFrame >= m_dashStartFrames &&
            m_dashCurrentFrame < m_dashStartFrames + m_dashActiveFrames)
        {
            ApplyDashStep(player);

            if (!m_dashActive)
                return;
        }
    }

    if (m_dashCurrentFrame >= m_dashTotalFrames)
    {
        m_dashActive = false;
        m_dashTimer = m_dashCD;
        m_CurrentState = EnemyState::STATE_IDLE;
        m_AllowAttack = true;

        if (m_dashAttackQueued && m_CombatSystem.CanStartAttack_Enemy(player, *this))
        {
            m_dashAttackQueued = false;
            m_WindingUp = false;
            m_WindUpTimer = 0.0f;
            m_AllowAttack = false;
            m_CurrentState = EnemyState::STATE_ATTACK;

            StartAttack(m_DashFollowupAttackData);
        }
        else
        {
            m_dashAttackQueued = false;
        }
    }
}

void Dasher::ChildUpdate(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies)
{
    (void)combat;
    (void)enemies;

    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 dist = AEVec2Length(&m_enemyToPlayerDir);

    if (dist > 0.001f)
        AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    if (m_CombatFlags.stunned)
        return;

    if (m_CurrentState != EnemyState::STATE_DASH_WINDUP &&
        m_CurrentState != EnemyState::STATE_DASH)
    {
        m_dashTimer -= dt;
    }

    // windup
    if (m_CurrentState == EnemyState::STATE_DASH_WINDUP)
    {
        m_moveDir = { 0.0f, 0.0f };
        m_dashWindupTimer -= dt;

        if (m_dashWindupTimer <= 0.0f)
        {
            AEVec2 dashToPlayer{ player.GetX() - m_pos.x, player.GetY() - m_pos.y };
            f32 dashDistToPlayer = AEVec2Length(&dashToPlayer);

            StartDash(m_lockedDashDir, dashDistToPlayer);
            UpdateDash(dt, player);
        }
        return;
    }

    // active dash
    if (m_CurrentState == EnemyState::STATE_DASH)
    {
        UpdateDash(dt, player);
        return;
    }

    // trigger dash
    if (m_dashTimer <= 0.0f &&
        dist >= m_dashMinRange &&
        dist <= m_dashRange &&
        !m_AttackActive &&
        !m_WindingUp)
    {
        m_lockedDashDir = m_enemyToPlayerDir; // lock once here
        m_CurrentState = EnemyState::STATE_DASH_WINDUP;
        m_dashWindupTimer = m_dashWindupDuration;
        m_moveDir = { 0.0f, 0.0f };
        return;
    }

    // normal chase
    if (m_WindingUp || m_AttackActive)
        return;

    const f32 desiredStopDistance = m_AttackRange * 0.6f;

    if (dist > desiredStopDistance) {
        m_CurrentState = EnemyState::STATE_MOVING;
        MoveTowardTarget(playerPos, dt);
    }
    else {
        m_CurrentState = EnemyState::STATE_IDLE;
        m_moveDir = { 0.0f, 0.0f };
    }

}

void Dasher::Draw()
{
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();
    Shadow_Draw(m_pos.x, m_pos.y, m_size);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    f32 isoHeight = m_size;
    f32 spriteScale = m_sizeMultiplier * (m_size / ENEMY_SIZE);


    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    // -------------------------
    // Dasher-specific sprites
    // -------------------------
    if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
        DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
            m_DasherSprite.GetDasherAttackSpriteMesh(),
            m_DasherSprite.GetDasherAttackSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    //else if (isAnyWindup) {
    //    DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
    //        m_DasherSprite.GetDasherWindupSpriteMesh(),
    //        m_DasherSprite.GetDasherWindupSpriteSheet(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_pos.x, m_pos.y, 0.0f, spriteScale);
    //}
    else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
        //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 0, 0, 255);
        m_DasherSprite.SetDasherAttackSingleFrame(0, 6);

        DrawTexture(m_DasherSprite,
            m_DasherSprite.GetDasherAttackSpriteMesh(),
            m_DasherSprite.GetDasherAttackSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else {
        DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
            m_DasherSprite.GetDasherSpriteMesh(),
            m_DasherSprite.GetDasherSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }

    if (m_dashActive && m_dashCurrentFrame >= m_dashStartFrames &&
        m_dashCurrentFrame < m_dashStartFrames + m_dashActiveFrames)
    {
            DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
                m_DasherSprite.GetDasherWindupSpriteMesh(),
                m_DasherSprite.GetDasherWindupSpriteSheet(),
                m_DasherSprite.GetPixelScale(),
                m_DasherSprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, m_sizeMultiplier);
    }

    // Enemy sword
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    /*DrawMesh(m_AttackRangeMesh, 1.0f, 5.0f, m_pos.x, m_pos.y, swordAngle,
        44, 255, 255, 255);*/
    if (m_AttackActive) DrawBat(swordAngle);

    // Enemy health bar
    f32 barWidth = m_size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth);
    f32 barHeight = m_size / 3.0f;
    f32 dbarWidth = m_size * 2.0f * AEClamp(
        (m_CombatStats.health / m_CombatStats.maxHealth) + (m_healthDepletionPercentage / 100.0f),
        0.0f, 1.0f);

    f32 dRate = 100.0f * dt;
    if (m_healthDepletionPercentage > 0.0f) {
        m_healthDepletionPercentage -= dRate;
        if (m_healthDepletionPercentage < 0.0f) {
            m_healthDepletionPercentage = 0.0f;
        }
    }

    DrawMesh(m_enemyHealthBarMesh, dbarWidth, barHeight,
        m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f,
        0.0f, 255, 175, 65, 255);

    DrawMesh(m_enemyHealthBarMesh, barWidth, barHeight,
        m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f,
        0.0f, 210, 70, 75, 255);

    // Damaging Mark
    if (m_marked && !m_markDetonating && m_markMesh) {
        float bobOffset = sinf(m_markTimer * 5.0f) * 4.0f;
        float daggerY = m_pos.y + m_size + 40.0f + bobOffset;
        DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 255, 255, 255);
    }
    else if (m_markDetonating && m_markMesh) {
        float t = m_markDetonateTimer / 0.3f;
        float hoverY = m_pos.y + m_size + 40.0f;
        float daggerY = m_pos.y + (hoverY - m_pos.y) * t;
        DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 80, 80, 255);
    }
}

// ---------------------
// | Child Class: Boss |
// ---------------------
Boss::Boss(AEVec2 pos, f32 size, f32 hp, f32 speed)
    : Enemy(pos, size, hp, speed)
{
    // Boss starts smaller / weaker
    m_size = size;
    m_CombatStats.health = hp;
    m_CombatStats.maxHealth = hp;
    m_CombatStats.attack = 15.0f;
    m_CombatStats.defense = 10.0f;

    m_AttackRange = 80.0f;
    m_Damage = 15;
    m_AttackData.damage = 15;

    m_AttackStartUpFrames = 12;
    m_AttackActiveFrames = 28;
    m_AttackRecoveryFrames = 16;
    m_WindUpDuration = 0.25f;

    m_AttackTotalFrames = m_AttackStartUpFrames + m_AttackActiveFrames + m_AttackRecoveryFrames;
    m_AttackData.startUp = m_AttackStartUpFrames;
    m_AttackData.active = m_AttackActiveFrames;
    m_AttackData.recovery = m_AttackRecoveryFrames;
    m_AttackData.total = m_AttackTotalFrames;

    m_GrowthHits = 0;
    m_BaseSize = m_size;
    m_BaseAttackRange = m_AttackRange;
    m_BaseAttackDamage = m_CombatStats.attack;

    m_PopBonus = 0.0f;
    m_WasGotHit = false;
    m_SizeGrowthPerHit = (70.0f - m_BaseSize) / 5.0f;
}

void Boss::ChildUpdate(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies) {
    (void)combat;
    // Prevent boss from dying before required phase transitions
    const f32 maxHP = m_CombatStats.maxHealth;
    const f32 phase3Threshold = maxHP * 0.2f;
    const f32 phase4Threshold = maxHP * 0.01f;

    // Phase 1/2 protection: boss cannot die before phase 2 is earned
    if (!m_PhaseTwoTriggered && m_CombatStats.health <= 0.0f) {
        m_CombatStats.health = m_CombatStats.maxHealth * 0.5f;
        m_CombatFlags.isAlive = true;

        // Make boss fully reach the 5-hit grown state first
        ForceGrowthHits(5);

        TriggerPhaseTwo(enemies);
        return;
    }

    // Phase 2 protection: force phase 3 instead of dying
    if (m_PhaseTwoTriggered && !m_Phase3Triggered && m_CombatStats.health <= 0.0f) {
        m_CombatStats.health = phase3Threshold;
        m_CombatFlags.isAlive = true;
    }

    // Phase 3 protection: force phase 4 instead of dying
    if (m_Phase3Triggered && !m_Phase4Triggered && m_CombatStats.health <= 0.0f) {
        m_CombatStats.health = phase4Threshold;
        m_CombatFlags.isAlive = true;
    }


    // Phase 4 trigger MUST come before the phase 3 early return
    if (!m_Phase4Triggered &&
        m_Phase3Triggered &&
        m_CombatStats.health <= m_CombatStats.maxHealth * 0.01f) {
        TriggerPhaseFour(enemies);
        return;
    }

    // Once phase 4 starts, it fully owns the boss
    if (m_Phase4Triggered) {
        UpdatePhaseFour(dt, player);
        return;
    }

    if (m_Phase3Triggered) {
        UpdatePhaseThree(dt, player, enemies);
        return;
    }

    if (m_PhaseTransitioning) {
        m_moveDir = { 0.0f, 0.0f };
        m_AttackActive = false;
        m_WindingUp = false;

        if (m_PhaseTwoBlinking) {
            m_PhaseBlinkTimer += dt;

            int blinkStep = static_cast<int>(m_PhaseBlinkTimer / m_PhaseBlinkInterval);
            m_PhaseBlinkVisible = (blinkStep % 2 == 0);

            if (m_PhaseBlinkTimer >= m_PhaseBlinkDuration) {
                m_PhaseTwoBlinking = false;
                m_PhaseBlinkVisible = true;
                m_UsePhaseTwoSprite = true;
                m_PhaseTransitioning = false;
            }
        }

        return;
    }

    if (m_CombatFlags.gotHit && !m_WasGotHit) {
        if (m_GrowthHits < 5) {
            ++m_GrowthHits;
            m_PopBonus = 8.0f;
        }

        if (m_GrowthHits >= 5 && !m_PhaseTwoTriggered) {
            TriggerPhaseTwo(enemies);
        }
    }
    m_WasGotHit = m_CombatFlags.gotHit;

    ApplyGrowthFromHits();

    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 distToPlayer = AEVec2Length(&m_enemyToPlayerDir);
    if (distToPlayer > 0.001f) {
        AEVec2Scale(&m_enemyToPlayerDir, &m_enemyToPlayerDir, 1.0f / distToPlayer);
    }

    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
        m_pos.x, m_pos.y, m_size)) {
        MoveTowardTarget(playerPos, dt);
    }
    else {
        m_moveDir = { 0.0f, 0.0f };
    }
}

void Boss::Draw()
{
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Phase 4 ball form
    if (m_Phase4BallVisible) {
        Shadow_Draw(m_Phase4BallPos.x, m_Phase4BallPos.y + 10.0f, 22.0f);
        DrawMesh(m_enemyMesh, 26.0f, 26.0f, m_Phase4BallPos.x, m_Phase4BallPos.y, 0.0f,
            120, 255, 255, 255);
        return;
    }

    // Blink-hide during phase 4 revive
    if (m_Phase4Blinking && !m_Phase4BlinkVisible) {
        return;
    }

    // Blink-hide during phase 3
    if (m_Phase3Blinking && !m_Phase3BlinkVisible) {
        return;
    }

    // Blink-hide during phase 2 transition
    if (m_PhaseTwoBlinking && !m_PhaseBlinkVisible) {
        return;
    }

    float spriteScale = m_sizeMultiplier * (m_size / 35.0f);

    float shadowY = m_pos.y;
    shadowY += m_size * 0.75f;
    Shadow_Draw(m_pos.x, shadowY, m_size);

    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    if (m_UsePhaseTwoSprite) {
        // ===== PHASE 2 ONWARDS: BOSS SPRITE =====
        if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
            DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
                m_EnemySprite.GetBossAttackSpriteMesh(),
                m_EnemySprite.GetBossAttackSpriteSheet(),
                m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, spriteScale);
        }
        else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
            DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
                m_EnemySprite.GetBossAttackSpriteMesh(),
                m_EnemySprite.GetBossAttackSpriteSheet(),
                m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, spriteScale);
        }
        else {
            DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
                m_EnemySprite.GetBossSpriteMesh(),
                m_EnemySprite.GetBossSpriteSheet(),
                m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, spriteScale);
        }
    }
    else {
        // ===== BEFORE PHASE 2: BASE / WALKER SPRITE =====
        if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
            DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
                m_EnemySprite.GetEnemyAttackSpriteMesh(),
                m_EnemySprite.GetEnemyAttackSpriteSheet(),
                m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, spriteScale);
        }
        else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
            m_EnemySprite.SetEnemyAttackSingleFrame(0, 6);

            DrawTexture(m_EnemySprite,
                m_EnemySprite.GetEnemyAttackSpriteMesh(),
                m_EnemySprite.GetEnemyAttackSpriteSheet(),
                m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, spriteScale);
        }
        else {
            DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
                m_EnemySprite.GetSpriteMesh(),
                m_EnemySprite.GetSpriteSheet(),
                m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, spriteScale);
        }
    }

    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    if (m_AttackActive) {
        DrawBat(swordAngle);
    }

    // Healthbar above head
    //f32 barWidth = m_size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth);
    //f32 barHeight = m_size / 3.0f;
    //f32 dbarWidth = m_size * 2.0f * AEClamp(
    //    (m_CombatStats.health / m_CombatStats.maxHealth) + (m_healthDepletionPercentage / 100.0f),
    //    0.0f, 1.0f);

    //f32 dRate = 100.0f * dt;
    //if (m_healthDepletionPercentage > 0.0f) {
    //    m_healthDepletionPercentage -= dRate;
    //    if (m_healthDepletionPercentage < 0.0f) {
    //        m_healthDepletionPercentage = 0.0f;
    //    }
    //}

    //DrawMesh(m_enemyHealthBarMesh, dbarWidth, barHeight,
    //    m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f,
    //    0.0f, 255, 175, 65, 255);

    //DrawMesh(m_enemyHealthBarMesh, barWidth, barHeight,
    //    m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f,
    //    0.0f, 210, 70, 75, 255);
}

void Enemy::EvaluateCurrentDirection()
{
    float angleDegrees = atan2(m_enemyToPlayerDir.y, m_enemyToPlayerDir.x) * 180.0f / PI;

    // rotate for isometric grid
    angleDegrees += 45.0f;

    if (angleDegrees > 180) angleDegrees -= 360;

    if (angleDegrees >= -30 && angleDegrees < 30) m_CurrentDirection = EnemyDirection::DIRECTION_DOWN;
    else if (angleDegrees >= 30 && angleDegrees < 90) m_CurrentDirection = EnemyDirection::DIRECTION_DOWN_RIGHT;
    else if (angleDegrees >= 90 && angleDegrees < 150) m_CurrentDirection = EnemyDirection::DIRECTION_UP_RIGHT;
    else if (angleDegrees >= 150 || angleDegrees < -150) m_CurrentDirection = EnemyDirection::DIRECTION_UP;
    else if (angleDegrees >= -150 && angleDegrees < -90) m_CurrentDirection = EnemyDirection::DIRECTION_UP_LEFT;
    else if (angleDegrees >= -90 && angleDegrees < -30) m_CurrentDirection = EnemyDirection::DIRECTION_DOWN_LEFT;
}

void Boss::ApplyGrowthFromHits()
{
    const int clampedHits = (std::min)(m_GrowthHits, 5);

    const f32 baseTargetSize = m_BaseSize + m_SizeGrowthPerHit * static_cast<f32>(clampedHits);
    m_PopBonus = Vectors::lerp(m_PopBonus, 0.0f, 0.10f);

    m_size = AEClamp(
        baseTargetSize + m_PopBonus,
        m_BaseSize, 140.0f);

    m_AttackRange = AEClamp(
        m_BaseAttackRange + m_RangeGrowthPerHit * static_cast<f32>(clampedHits),
        m_BaseAttackRange, 260.0f);

    m_CombatStats.attack = AEClamp(
        m_BaseAttackDamage + m_DamageGrowthPerHit * static_cast<f32>(clampedHits),
        m_BaseAttackDamage, 80.0f);

    m_Damage = static_cast<int>(m_CombatStats.attack);
    m_AttackData.damage = m_Damage;

    if (m_AttackRangeMesh) {
        AEGfxMeshFree(m_AttackRangeMesh);
        m_AttackRangeMesh = nullptr;
    }
    m_AttackRangeMesh = CreateAttackRangeMesh(m_AttackRange, 0xFF0000);
}

void Boss::TriggerPhaseTwo(std::vector<std::unique_ptr<Enemy>>& enemies)
{
    (void)enemies;

    m_PhaseTwoTriggered = true;
    m_PhaseTransitioning = true;

    m_PhaseTwoBlinking = true;
    m_PhaseBlinkTimer = 0.0f;
    m_PhaseBlinkVisible = true;

    // do NOT swap yet
    m_UsePhaseTwoSprite = false;

    // stop all current actions
    m_moveDir = { 0.0f, 0.0f };
    m_AttackActive = false;
    m_WindingUp = false;
    m_AttackCooldown = 0.0f;
    m_KnockbackVelocity = { 0.0f, 0.0f };

    // heal to full
    m_CombatStats.health = m_CombatStats.maxHealth;
    m_healthDepletionPercentage = 0.0f;
}

void Boss::TriggerPhaseThree()
{
    if (m_Phase3Triggered) return;

    m_Phase3Triggered = true;

    // blink + invincible immediately
    m_Phase3Transitioning = true;
    m_Phase3Blinking = true;
    m_Phase3BlinkVisible = true;

    m_RunToCenterPhase = true;
    m_Phase3ReachedCenter = false;
    m_IsThrowerPhase = false;

    m_Phase3BlinkTimer = 0.0f;
    m_Phase3HealTarget = m_CombatStats.maxHealth * 0.8f;

    m_EnableMelee = false;
    m_AttackActive = false;
    m_WindingUp = false;
    m_AllowAttack = false;
    m_moveDir = { 0.0f, 0.0f };
    m_KnockbackVelocity = { 0.0f, 0.0f };

    m_Phase3RunToCenterSpeed = 420.0f;
    m_Phase3ThrowerMoveSpeed = 5.0f;
    m_speed = m_Phase3RunToCenterSpeed;
}

void Boss::UpdatePhaseThree(f32 dt, Player& player, std::vector<std::unique_ptr<Enemy>>& enemies)
{
    if (!m_Phase4Triggered &&
        m_CombatStats.health <= m_CombatStats.maxHealth * 0.01f) {
        TriggerPhaseFour(enemies);
        return;
    }

    if (!m_pMap) return;

    // keep facing player
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2 toPlayer;
    AEVec2Sub(&toPlayer, &playerPos, &m_pos);

    float distToPlayer = AEVec2Length(&toPlayer);
    if (distToPlayer > 0.001f) {
        AEVec2Scale(&m_enemyToPlayerDir, &toPlayer, 1.0f / distToPlayer);
        m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);
    }

    // middle of map in grid space -> world space
    int centerCol = static_cast<int>(m_pMap->GetMapWidth() / 2);
    int centerRow = static_cast<int>(m_pMap->GetMapHeight() / 2);
    AEVec2 centerPos = m_pMap->TMXToWorld(centerCol, centerRow);

    // blink for the whole transition
    if (m_Phase3Blinking) {
        m_Phase3BlinkTimer += dt;
        int blinkStep = static_cast<int>(m_Phase3BlinkTimer / m_Phase3BlinkInterval);
        m_Phase3BlinkVisible = (blinkStep % 2 == 0);
    }

    // run to center while blinking
    if (m_RunToCenterPhase && !m_Phase3ReachedCenter) {
        AEVec2 toCenter;
        AEVec2Sub(&toCenter, &centerPos, &m_pos);
        float dist = AEVec2Length(&toCenter);

        if (dist > 12.0f) {
            m_speed = m_Phase3RunToCenterSpeed;
            MoveTowardTarget(centerPos, dt);
            m_KnockbackVelocity = { 0.0f, 0.0f };
            return;
        }

        // reached center: stop moving, continue blinking/healing
        m_Phase3ReachedCenter = true;
        m_RunToCenterPhase = false;
        m_moveDir = { 0.0f, 0.0f };
        m_KnockbackVelocity = { 0.0f, 0.0f };
        m_speed = 0.0f;

        // restart timer so the center-heal blink lasts properly
        m_Phase3BlinkTimer = 0.0f;
        return;
    }

    // stand in middle, blink, then finish transition
    if (m_Phase3Transitioning) {
        m_moveDir = { 0.0f, 0.0f };
        m_AttackActive = false;
        m_WindingUp = false;
        m_KnockbackVelocity = { 0.0f, 0.0f };

        if (m_Phase3ReachedCenter && m_Phase3BlinkTimer >= m_Phase3BlinkDuration) {
            m_Phase3Blinking = false;
            m_Phase3BlinkVisible = true;
            m_Phase3Transitioning = false;
            m_IsThrowerPhase = true;

            m_CombatStats.health = m_Phase3HealTarget;
            m_healthDepletionPercentage = 0.0f;
            m_speed = m_Phase3ThrowerMoveSpeed;
        }

        return;
    }

    if (!m_Phase4Triggered &&
        m_CombatStats.health <= m_CombatStats.maxHealth * 0.01f) {
        TriggerPhaseFour(enemies);
        return;
    }

    // after transition, stay in middle
    if (m_IsThrowerPhase) {
        m_moveDir = { 0.0f, 0.0f };
        m_KnockbackVelocity = { 0.0f, 0.0f };
        m_EnableMelee = false;
        return;
    }
}

void Boss::TriggerPhaseFour(std::vector<std::unique_ptr<Enemy>>& enemies)
{
    if (m_Phase4Triggered) return;

    if (!m_Phase4BuffApplied) {
        // Override speed
        m_speed = m_Phase4Speed;

        // Increase range (additive)
        m_AttackRange += m_Phase4RangeBonus;

        // Override damage (flat)
        m_CombatStats.attack = m_Phase4Damage;
        m_AttackData.damage = static_cast<int>(m_Phase4Damage);
        m_DashFollowupAttackData.damage = static_cast<int>(m_Phase4Damage);

        m_Phase4BuffApplied = true;
    }

    m_FinalState = BossFinalState::FakeDeath;

    // Despawn all enemies except this boss
    m_Phase4Triggered = true;

    m_Phase4BallVisible = true;
    m_Phase4BallPos = m_pos;

    m_Phase4Invulnerable = true;
    m_Phase4Blinking = false;
    m_Phase4BlinkVisible = true;
    m_Phase4ReviveBlinkTimer = 0.0f;

    m_moveDir = { 0.0f, 0.0f };
    m_KnockbackVelocity = { 0.0f, 0.0f };
    m_AttackActive = false;
    m_WindingUp = false;
    m_AllowAttack = false;
    m_EnableMelee = false;

    m_CurrentState = EnemyState::STATE_IDLE;

    // Keep boss alive so outer erase logic does not remove it
    m_CombatStats.health = m_CombatStats.maxHealth * 0.01f;
    m_healthDepletionPercentage = 0.0f;
    m_CombatFlags.isAlive = true;

    m_Phase4DashesRemaining = 0;
    m_Phase4DashPauseTimer = 0.0f;
    m_Phase4CurrentDashSpeed = 0.0f;
    m_Phase4RecoveryTimer = 0.0f;
    m_Phase4RecoveryStep = 0;

    m_FinalState = BossFinalState::WaitingPickup;
}

void Boss::UpdatePhaseFour(f32 dt, Player& player)
{
    switch (m_FinalState)
    {
    case BossFinalState::WaitingPickup:
    {
        m_moveDir = { 0.0f, 0.0f };
        m_KnockbackVelocity = { 0.0f, 0.0f };
        m_AttackActive = false;
        m_WindingUp = false;
        m_CurrentState = EnemyState::STATE_IDLE;
        break;
    }

    case BossFinalState::Reviving:
    {
        m_moveDir = { 0.0f, 0.0f };
        m_KnockbackVelocity = { 0.0f, 0.0f };
        m_AttackActive = false;
        m_WindingUp = false;
        m_CurrentState = EnemyState::STATE_IDLE;

        m_Phase4ReviveBlinkTimer += dt;
        int blinkStep = static_cast<int>(m_Phase4ReviveBlinkTimer / m_Phase4ReviveBlinkInterval);
        m_Phase4BlinkVisible = (blinkStep % 2 == 0);

        m_CombatStats.health += m_Phase4HealRate * dt;
        if (m_CombatStats.health >= m_CombatStats.maxHealth) {
            m_CombatStats.health = m_CombatStats.maxHealth;
            m_healthDepletionPercentage = 0.0f;

            m_Phase4Blinking = false;
            m_Phase4BlinkVisible = true;
            m_Phase4Invulnerable = false;

            StartPhase4TripleDash(player);
        }
        break;
    }

    case BossFinalState::TripleDashPattern:
        UpdatePhase4TripleDash(dt, player);
        break;

    case BossFinalState::RecoveryPattern:
        UpdatePhase4RecoveryPattern(dt, player);
        break;

    case BossFinalState::FakeDeath:
    case BossFinalState::None:
    default:
        break;
    }
}

void Boss::StartPhase4TripleDash(Player& player)
{
    (void)player;

    m_FinalState = BossFinalState::TripleDashPattern;

    m_Phase4DashesRemaining = 3;
    m_Phase4DashTravelled = 0.0f;
    m_Phase4CurrentDashSpeed = 0.0f;
    m_Phase4InterDashPauseTimer = 0.0f;

    m_Phase4DidInitialTeleport = false;
    m_Phase4PreDashBlinking = true;
    m_Phase4PreDashBlinkTimer = m_Phase4PreDashBlinkDuration;

    m_Phase4Invulnerable = false;
    m_EnableMelee = false;
    m_AllowAttack = false;
    m_AttackActive = false;
    m_WindingUp = false;

    m_CurrentState = EnemyState::STATE_IDLE;
    m_moveDir = { 0.0f, 0.0f };
}

void Boss::TeleportForPhase4Dash(Player& player)
{
    AEVec2 face = player.GetNormalizedVector();
    if (AEVec2Length(&face) < 0.001f) {
        face = { 1.0f, 0.0f };
    }

    AEVec2 desiredPos{
        player.GetX() + face.x * m_Phase4TeleportDistance,
        player.GetY() + face.y * m_Phase4TeleportDistance
    };

    AEVec2 bestPos = desiredPos;
    bool foundSpot = false;

    if (!m_pMap || !m_pMap->IsPositionBlocked(desiredPos.x, desiredPos.y, m_size)) {
        foundSpot = true;
    }
    else {
        for (int i = 0; i < 16; ++i) {
            float angle = (2.0f * PI / 16.0f) * i;
            AEVec2 testPos{
                desiredPos.x + cosf(angle) * 45.0f,
                desiredPos.y + sinf(angle) * 45.0f
            };

            if (!m_pMap->IsPositionBlocked(testPos.x, testPos.y, m_size)) {
                bestPos = testPos;
                foundSpot = true;
                break;
            }
        }
    }

    if (foundSpot) {
        m_pos = bestPos;
    }

    m_Phase4DidInitialTeleport = true;
    PreparePhase4DashTarget(player);
}

void Boss::UpdatePhase4TripleDash(f32 dt, Player& player)
{
    m_moveDir = { 0.0f, 0.0f };
    m_KnockbackVelocity = { 0.0f, 0.0f };
    m_AttackActive = false;
    m_WindingUp = false;

    if (m_Phase4DashesRemaining <= 0) {
        m_FinalState = BossFinalState::RecoveryPattern;
        m_Phase4RecoveryTimer = m_Phase4RecoveryDuration;
        m_Phase4RecoveryStep = 0;
        m_CurrentState = EnemyState::STATE_IDLE;
        m_EnableMelee = true;
        m_AllowAttack = true;
        return;
    }

    // Warning blink before first teleport
    if (m_Phase4PreDashBlinking) {
        m_Phase4PreDashBlinkTimer -= dt;
        m_Phase4Blinking = true;

        int blinkStep = static_cast<int>((m_Phase4PreDashBlinkDuration - m_Phase4PreDashBlinkTimer) / 0.08f);
        m_Phase4BlinkVisible = (blinkStep % 2 == 0);

        if (m_Phase4PreDashBlinkTimer <= 0.0f) {
            m_Phase4PreDashBlinking = false;
            m_Phase4Blinking = false;
            m_Phase4BlinkVisible = true;

            TeleportForPhase4Dash(player);
            m_CurrentState = EnemyState::STATE_DASH;
        }
        return;
    }

    // Pause between dashes, then re-aim from current side
    if (m_Phase4InterDashPauseTimer > 0.0f) {
        m_Phase4InterDashPauseTimer -= dt;
        m_CurrentState = EnemyState::STATE_IDLE;
        m_Phase4CurrentDashSpeed = 0.0f;

        if (m_Phase4InterDashPauseTimer <= 0.0f) {
            PreparePhase4DashTarget(player);
            m_CurrentState = EnemyState::STATE_DASH;
        }
        return;
    }

    if (m_CurrentState != EnemyState::STATE_DASH) {
        PreparePhase4DashTarget(player);
        m_CurrentState = EnemyState::STATE_DASH;
    }

    m_Phase4CurrentDashSpeed += m_Phase4DashAccel * dt;
    if (m_Phase4CurrentDashSpeed > m_Phase4DashMaxSpeed) {
        m_Phase4CurrentDashSpeed = m_Phase4DashMaxSpeed;
    }

    AEVec2 step;
    AEVec2Scale(&step, &m_Phase4LockedDashDir, m_Phase4CurrentDashSpeed * dt);

    float newX = m_pos.x + step.x;
    float newY = m_pos.y + step.y;

    bool endDash = false;

    if (m_pMap && m_pMap->IsPositionBlocked(newX, newY, m_size)) {
        endDash = true;
    }
    else {
        m_pos.x = newX;
        m_pos.y = newY;
    }

    if (!m_Phase4DashHitPlayer &&
        AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
            m_pos.x, m_pos.y, m_size)) {

        f32 dashDamage = m_Phase4Damage;
        player.DeductHealth(dashDamage);
        player.SetHDP(dashDamage);

        AEVec2 kb;

        // Perpendicular (side) direction
        AEVec2 perp{ -m_Phase4LockedDashDir.y, m_Phase4LockedDashDir.x };

        // Decide left or right based on player position
        AEVec2 toPlayer{ player.GetX() - m_pos.x, player.GetY() - m_pos.y };

        // Dot product to determine side
        float side = toPlayer.x * perp.x + toPlayer.y * perp.y;

        if (side < 0.0f) {
            // flip direction → push opposite side
            perp.x = -perp.x;
            perp.y = -perp.y;
        }

        // Knockback force
        float kbForce = 420.0f;
        kbForce += m_Phase4CurrentDashSpeed * 0.25f;

        AEVec2 forward;
        AEVec2Scale(&forward, &m_Phase4LockedDashDir, 120.0f);

        AEVec2Add(&kb, &kb, &forward);

        // Apply
        AEVec2Scale(&kb, &perp, kbForce);
        player.SetKnockbackVelocity(kb);
    }

    // Stop when boss reaches or passes target point
    AEVec2 toTarget{ m_Phase4DashTarget.x - m_pos.x, m_Phase4DashTarget.y - m_pos.y };
    float dot = toTarget.x * m_Phase4LockedDashDir.x + toTarget.y * m_Phase4LockedDashDir.y;

    if (dot <= 0.0f) {
        endDash = true;
    }

    if (endDash) {
        --m_Phase4DashesRemaining;
        m_CurrentState = EnemyState::STATE_IDLE;
        m_Phase4CurrentDashSpeed = 0.0f;

        if (m_Phase4DashesRemaining > 0) {
            m_Phase4InterDashPauseTimer = m_Phase4InterDashPauseDuration;
        }
    }
}

void Boss::UpdatePhase4RecoveryPattern(f32 dt, Player& player)
{
    m_Phase4RecoveryTimer -= dt;
    if (m_Phase4RecoveryTimer <= 0.0f) {
        StartPhase4TripleDash(player);
        return;
    }

    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 distToPlayer = AEVec2Length(&m_enemyToPlayerDir);

    if (distToPlayer > 0.001f) {
        AEVec2Scale(&m_enemyToPlayerDir, &m_enemyToPlayerDir, 1.0f / distToPlayer);
    }

    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    switch (m_Phase4RecoveryStep)
    {
    case 0:
    case 1:
    {
        m_EnableMelee = true;

        if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
            m_pos.x, m_pos.y, m_size)) {
            MoveTowardTarget(playerPos, dt);
        }
        else {
            m_moveDir = { 0.0f, 0.0f };
        }

        if (!m_AttackActive && !m_WindingUp && m_AllowAttack) {
            ++m_Phase4RecoveryStep;
        }
        break;
    }

    case 2:
    {
        m_EnableMelee = false;
        m_AttackActive = false;
        m_WindingUp = false;

        if (m_Phase4DashPauseTimer <= 0.0f && m_CurrentState != EnemyState::STATE_DASH) {
            TeleportForPhase4Dash(player);
            m_Phase4CurrentDashSpeed = 350.0f;
        }

        if (m_CurrentState == EnemyState::STATE_DASH) {
            m_Phase4CurrentDashSpeed += m_Phase4DashAccel * dt;
            if (m_Phase4CurrentDashSpeed > m_Phase4DashMaxSpeed) {
                m_Phase4CurrentDashSpeed = m_Phase4DashMaxSpeed;
            }

            AEVec2 step;
            AEVec2Scale(&step, &m_Phase4LockedDashDir, m_Phase4CurrentDashSpeed * dt);

            float newX = m_pos.x + step.x;
            float newY = m_pos.y + step.y;

            bool endDash = false;

            if (m_pMap && m_pMap->IsPositionBlocked(newX, newY, m_size)) {
                endDash = true;
            }
            else {
                m_pos.x = newX;
                m_pos.y = newY;
            }

            if (!m_Phase4DashHitPlayer &&
                AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
                    m_pos.x, m_pos.y, m_size)) {

                constexpr f32 dashDamage = 25.0f;
                player.DeductHealth(dashDamage);
                player.SetHDP(dashDamage);

                AEVec2 kb;
                AEVec2Scale(&kb, &m_Phase4LockedDashDir, 250.0f);
                player.SetKnockbackVelocity(kb);

                m_Phase4DashHitPlayer = true;
                endDash = true;
            }

            if (endDash) {
                m_CurrentState = EnemyState::STATE_IDLE;
                m_Phase4CurrentDashSpeed = 0.0f;
                m_Phase4RecoveryStep = 0;
                m_EnableMelee = true;
                m_AllowAttack = true;
            }
        }
        break;
    }

    default:
        m_Phase4RecoveryStep = 0;
        break;
    }
}

void Boss::ConsumePhase4Pickup()
{
    if (m_FinalState != BossFinalState::WaitingPickup) return;

    m_Phase4BallVisible = false;

    m_pos = m_Phase4BallPos;
    m_Phase4Blinking = true;
    m_Phase4BlinkVisible = true;
    m_Phase4ReviveBlinkTimer = 0.0f;

    m_FinalState = BossFinalState::Reviving;
}

void Boss::ForceGrowthHits(int hits)
{
    if (m_GrowthHits >= hits) {
        return;
    }

    m_GrowthHits = hits;
    ApplyGrowthFromHits();
}

// ------------------------
// | Child Class: Thrower |
// ------------------------
Thrower::Thrower(AEVec2 pos, f32 size, f32 hp, f32 speed) :
    Enemy(pos, size, hp, speed)
{
    m_CombatStats.health = hp;
    m_CombatStats.maxHealth = hp;
    m_CombatStats.attack = 0.0f;
    m_CombatStats.defense = 0.0f;
    m_EnableMelee = false;
}

void Thrower::Draw() {
    if (m_HideBody) {
        for (Projectile const& projectile : m_projectiles) {
            projectile.Draw();
        }
        return;
    }

    f32 dt = (f32)AEFrameRateControllerGetFrameTime();
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    float spriteScale = m_sizeMultiplier;
    float shadowY = m_pos.y;

    Shadow_Draw(m_pos.x, shadowY, m_size);

    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetThrowerAttackSpriteMesh(),
            m_EnemySprite.GetThrowerAttackSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetThrowerAttackSpriteMesh(),
            m_EnemySprite.GetThrowerAttackSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else {
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetThrowerSpriteMesh(),
            m_EnemySprite.GetThrowerSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }

    f32 barWidth = m_size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth);
    f32 barHeight = m_size / 3.0f;
    f32 dbarWidth = m_size * 2.0f * AEClamp(
        (m_CombatStats.health / m_CombatStats.maxHealth) + (m_healthDepletionPercentage / 100.0f),
        0.0f, 1.0f);

    f32 dRate = 100.0f * dt;
    if (m_healthDepletionPercentage > 0.0f) {
        m_healthDepletionPercentage -= dRate;
        if (m_healthDepletionPercentage < 0.0f) {
            m_healthDepletionPercentage = 0.0f;
        }
    }

    DrawMesh(m_enemyHealthBarMesh, dbarWidth, barHeight,
        m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f,
        0.0f, 255, 175, 65, 255);

    DrawMesh(m_enemyHealthBarMesh, barWidth, barHeight,
        m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 25.0f,
        0.0f, 210, 70, 75, 255);

    if (m_marked && !m_markDetonating && m_markMesh) {
        float bobOffset = sinf(m_markTimer * 5.0f) * 4.0f;
        float daggerY = m_pos.y + m_size + 40.0f + bobOffset;
        DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 255, 255, 255);
    }
    else if (m_markDetonating && m_markMesh) {
        float t = m_markDetonateTimer / 0.3f;
        float hoverY = m_pos.y + m_size + 40.0f;
        float daggerY = m_pos.y + (hoverY - m_pos.y) * t;
        DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 80, 80, 255);
    }

    for (Projectile const& projectile : m_projectiles) {
        projectile.Draw();
    }
}

void Thrower::ChildUpdate(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies) {
    // Update direction vector, get dist bw enemy and player
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 dist = AEVec2Length(&m_enemyToPlayerDir);
    if (dist > 0.001f) AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);
   
    // Angle to draw gun/projectile
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // Check projectile collision / projectile movement
    UpdateProjectiles(dt, combat, player, enemies);
    // Remove inactive projectiles (eg hit wall, hit player, lifetime 0)
    CleanupProjectiles();

    if (m_CombatFlags.stunned) {
        return;
    }

    m_throwTimer -= dt;

    bool hasLOS = true;
    if (m_pMap) hasLOS = HasLineOfSight_Grid(m_pos, playerPos, *m_pMap);

    // Chase if too far OR if we still do not have a clear throw
    if (dist > m_maxThrowRange || !hasLOS) {
        MoveTowardTarget(playerPos, dt);
        return;
    }

    // Inside preferred range band and line of sight is clear → throw
    if (dist >= m_minThrowRange && dist <= m_maxThrowRange && m_throwTimer <= 0.0f) {
        ThrowProjectile(playerPos);
        m_throwTimer = m_throwCooldown;
        return;
    }

    // Otherwise hold position
    m_moveDir = { 0.0f, 0.0f };
}

// Start of projectile lifetime
void Thrower::ThrowProjectile(AEVec2 const& targetPos) {
    AEVec2 dir;
    AEVec2 target = targetPos;
    AEVec2Sub(&dir, &target, &m_pos);

    if (AEVec2Length(&dir) <= 0.001f) {
        return;
    }

    m_projectiles.emplace_back(
        m_pos,
        dir,
        m_projectileSpeed,
        m_projectileRadius,
        m_projectileDamage,
        m_projectileLifetime
    );

    m_projectiles.back().SetType(m_projectileType);
    m_projectiles.back().Init();
}

// Projectile collision check
void Thrower::UpdateProjectiles(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies)
{
    for (Projectile& projectile : m_projectiles)
    {
        AEVec2 prevPos = projectile.GetPosition();

        projectile.Update(dt, m_pMap);

        if (!projectile.IsActive())
            continue;

        AEVec2 currPos = projectile.GetPosition();

        // Projectile got sliced by parry during this frame
        if (!projectile.IsReflected() && !projectile.IsInReflectGrace() &&
            player.CanParryProjectileSweep(prevPos, currPos, projectile.GetRadius()))
        {
            player.GainAttackCharge();
            gAudio.PlayCombatSFX(COMBAT_PARRY);

            if (projectile.GetType() == ProjectileType::Normal)
            {
                projectile.Destroy();
            }
            else if (projectile.GetType() == ProjectileType::Reflect)
            {
                AEVec2 reflectDir;
                AEVec2FromAngle(&reflectDir, player.GetAimAngle());
                projectile.Reflect(reflectDir);
            }

            continue;
        }

        // Reflected projectile now damages enemies
        if (projectile.IsReflected())
        {
            for (auto& enemyPtr : enemies)
            {
                Enemy* enemy = enemyPtr.get();
                if (!enemy)
                    continue;

                if (!enemy->GetIsAlive())
                    continue;

                if (AreCirclesIntersecting(
                    currPos.x, currPos.y, projectile.GetRadius(),
                    enemy->GetX(), enemy->GetY(), enemy->GetSize()))
                {
                    combat.ApplyDamage(*enemy, projectile);
                    projectile.Destroy();
                    break;
                }
            }

            // Reflected projectile should never damage player
            continue;
        }

        // Normal hit on player if not parried / not reflected into an enemy
        if (AreCirclesIntersecting(
            currPos.x, currPos.y, projectile.GetRadius(),
            player.GetX(), player.GetY(), player.GetSize()))
        {
            combat.ApplyDamage(player, projectile);
            projectile.Destroy();
        }
    }
}

// Remove inactive projectile from container
void Thrower::CleanupProjectiles() {
    m_projectiles.erase(
        std::remove_if(m_projectiles.begin(), m_projectiles.end(),
            [](Projectile const& projectile) {
                return !projectile.IsActive();
            }),
        m_projectiles.end());
}
