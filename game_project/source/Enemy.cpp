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

    if (m_BatMesh) {
        AEGfxMeshFree(m_BatMesh);
        m_BatMesh = nullptr;
    }
    if (m_BatTexture) {
        AEGfxTextureUnload(m_BatTexture);
        m_BatTexture = nullptr;
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
            m_AttackFrameAccumulator -= m_CombatSystem.GetOneFPS();
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

void Enemy::Draw() {
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();
    Shadow_Draw(m_pos.x, m_pos.y, m_size);

    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Calculate isometric squashed height for drawing
        //f32 isoHeight = m_size * (GRID_H / GRID_W); // Squashed
        f32 isoHeight = m_size; // Normal

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
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
    }
    //else if (isAnyWindup) {
    //    // Winding up — tint yellow to show charging
    //    //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 200, 0, 255);
    //    DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
    //        m_EnemySprite.GetEnemyWindupSpriteMesh(),
    //        m_EnemySprite.GetEnemyWindupSpriteSheet(),
    //        m_EnemySprite.GetPixelScale(),
    //        m_EnemySprite.GetPixelScale(),
    //        m_pos.x, m_pos.y, 0.0f, sizeMultiplier);

    //    DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
    //        m_DasherSprite.GetEnemyWindupSpriteMesh(),
    //        m_DasherSprite.GetEnemyWindupSpriteSheet(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
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
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
    }
    else {
        //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 44, 255, 255, 255);
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetSpriteMesh(),
            m_EnemySprite.GetSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);

        DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
            m_DasherSprite.GetSpriteMesh(),
            m_DasherSprite.GetSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
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
    if (m_marked && !m_markDetonating && m_markMesh) {
        // Hovering dagger with gentle bob
        float bobOffset = sinf(m_markTimer * 5.0f) * 4.0f;
        float daggerY = m_pos.y + m_size + 40.0f + bobOffset;
        DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 255, 255, 255);
    }
    else if (m_markDetonating && m_markMesh) {
        // Dagger drops from hover position down to enemy
        float t = m_markDetonateTimer / 0.3f; // 1.0 = top, 0.0 = enemy
        float hoverY = m_pos.y + m_size + 40.0f;
        float daggerY = m_pos.y + (hoverY - m_pos.y) * t;
        DrawMesh(m_markMesh, 14.0f, 20.0f, m_pos.x, daggerY, 0.0f, 255, 80, 80, 255);
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
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
    }
    //else if (isAnyWindup) {
    //    DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
    //        m_DasherSprite.GetDasherWindupSpriteMesh(),
    //        m_DasherSprite.GetDasherWindupSpriteSheet(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_DasherSprite.GetPixelScale(),
    //        m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
    //}
    else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
        //DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 0, 0, 255);
        m_DasherSprite.SetDasherAttackSingleFrame(0, 6);

        DrawTexture(m_DasherSprite,
            m_DasherSprite.GetDasherAttackSpriteMesh(),
            m_DasherSprite.GetDasherAttackSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
    }
    else {
        DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
            m_DasherSprite.GetDasherSpriteMesh(),
            m_DasherSprite.GetDasherSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
    }

    if (m_dashActive && m_dashCurrentFrame >= m_dashStartFrames &&
        m_dashCurrentFrame < m_dashStartFrames + m_dashActiveFrames)
    {
            DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
                m_DasherSprite.GetDasherWindupSpriteMesh(),
                m_DasherSprite.GetDasherWindupSpriteSheet(),
                m_DasherSprite.GetPixelScale(),
                m_DasherSprite.GetPixelScale(),
                m_pos.x, m_pos.y, 0.0f, sizeMultiplier);
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
    // Boss has higher stats
    m_CombatStats.health = hp;
    m_CombatStats.maxHealth = hp;
    m_CombatStats.attack = 50.0f;
    m_CombatStats.defense = 10.0f;
    m_AttackRange = 175.0f;
    m_Damage = 40;
    m_AttackData.damage = 40;
    m_AttackStartUpFrames = 10;
    m_AttackActiveFrames = 20;
    m_AttackRecoveryFrames = 20;
    m_AttackTotalFrames = m_AttackStartUpFrames + m_AttackActiveFrames + m_AttackRecoveryFrames;
    m_AttackData.startUp = m_AttackStartUpFrames;
    m_AttackData.active = m_AttackActiveFrames;
    m_AttackData.recovery = m_AttackRecoveryFrames;
    m_AttackData.total = m_AttackTotalFrames;
    m_WindUpDuration = 1.0f; // Boss winds up longer
}

void Boss::ChildUpdate(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies) {
    (void)enemies;
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    // Point sword towards player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // Seek player via A* pathfinding
    if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
                                m_pos.x, m_pos.y, m_size)) {
        MoveTowardTarget(playerPos, dt);
    }
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
    Enemy::Draw();

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
