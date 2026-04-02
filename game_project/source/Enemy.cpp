/*************************************************************************
@file       Enemy.cpp
@Author     Nigel Lim, nigelkaiyu.lim@digipen.edu
@Co-authors nil
@brief      This file contains the implementation of the Enemy base class
            and its derived enemy types, including movement, combat,
            rendering, pathfinding, projectile handling, and boss phase
            behaviours.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

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

// static shared resource definitions
AEGfxVertexList* Enemy::s_enemyMesh = nullptr;
AEGfxVertexList* Enemy::s_healthBarMesh = nullptr;
AEGfxVertexList* Enemy::s_markMesh = nullptr;
AEGfxVertexList* Enemy::s_batMesh = nullptr;
AEGfxTexture* Enemy::s_batTexture = nullptr;
AEGfxVertexList* Enemy::s_detonateMesh = nullptr;
AEGfxTexture* Enemy::s_detonateTexture = nullptr;
int Enemy::s_sharedRefCount = 0;

// helper functions
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

// base constructor
Enemy::Enemy(AEVec2 pos, f32 size, f32 hp, f32 speed)
    : m_pos{ pos }, m_hp{ hp }, m_speed{ speed }, m_size{ size } {
    m_CombatStats.health = hp;
    m_CombatStats.maxHealth = hp;
}

// base destructor
Enemy::~Enemy() {
    // free per-instance resources
    if (m_AttackRangeMesh) {
        AEGfxMeshFree(m_AttackRangeMesh);
        m_AttackRangeMesh = nullptr;
    }

    // clear instance pointers to shared resources
    m_enemyMesh = nullptr;
    m_enemyHealthBarMesh = nullptr;
    m_markMesh = nullptr;
    m_BatMesh = nullptr;
    m_BatTexture = nullptr;
    m_DetonateMesh = nullptr;
    m_DetonateTexture = nullptr;

    // free shared resources only when the last enemy is destroyed
    --s_sharedRefCount;
    if (s_sharedRefCount <= 0) {
        s_sharedRefCount = 0;
        if (s_enemyMesh) { AEGfxMeshFree(s_enemyMesh); s_enemyMesh = nullptr; }
        if (s_healthBarMesh) { AEGfxMeshFree(s_healthBarMesh); s_healthBarMesh = nullptr; }
        if (s_markMesh) { AEGfxMeshFree(s_markMesh); s_markMesh = nullptr; }
        if (s_batMesh) { AEGfxMeshFree(s_batMesh); s_batMesh = nullptr; }
        if (s_batTexture) { AEGfxTextureUnload(s_batTexture); s_batTexture = nullptr; }
        if (s_detonateMesh) { AEGfxMeshFree(s_detonateMesh); s_detonateMesh = nullptr; }
        if (s_detonateTexture) { AEGfxTextureUnload(s_detonateTexture); s_detonateTexture = nullptr; }
    }
}

void Enemy::Init() {
    // create shared resources once and reuse across all enemies
    ++s_sharedRefCount;
    if (s_sharedRefCount == 1) {
        s_enemyMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
        s_healthBarMesh = CreateRectMesh(0xAEF359);
        s_markMesh = CreateRectMesh(0xFFFFFF);
        s_batMesh = CreateBatMesh(0xFFFFFFFF);
        s_batTexture = AEGfxTextureLoad("Assets/Sprites/BatBat.png");
        s_detonateMesh = CreateSpriteRectMesh(0xFFFFFFFF, 8.0f, 2.0f);
        s_detonateTexture = AEGfxTextureLoad("Assets/Sprites/Detonate_Spritesheet.png");
    }

    // point this enemy instance to shared resources
    m_enemyMesh = s_enemyMesh;
    m_enemyHealthBarMesh = s_healthBarMesh;
    m_markMesh = s_markMesh;
    m_BatMesh = s_batMesh;
    m_BatTexture = s_batTexture;
    m_DetonateMesh = s_detonateMesh;
    m_DetonateTexture = s_detonateTexture;

    // create per-instance attack range mesh
    if (m_AttackRangeMesh) {
        AEGfxMeshFree(m_AttackRangeMesh);
        m_AttackRangeMesh = nullptr;
    }
    m_AttackRangeMesh = CreateAttackRangeMesh(m_AttackRange, 0xFF0000);

    // initialize enemy sprites
    m_EnemySprite.Sprite_Init();
    m_EnemySpriteSheet = m_EnemySprite.GetSpriteSheet();
    m_EnemyWindupSpriteSheet = m_EnemySprite.GetEnemyWindupSpriteSheet();
    m_EnemyAttackSpriteSheet = m_EnemySprite.GetEnemyAttackSpriteSheet();

    m_DasherSprite.Sprite_Init();
    m_DasherSpriteSheet = m_DasherSprite.GetSpriteSheet();
    m_DasherWindupSpriteSheet = m_DasherSprite.GetEnemyWindupSpriteSheet();
    m_DasherAttackSpriteSheet = m_DasherSprite.GetEnemyAttackSpriteSheet();
}

void Enemy::BaseUpdate(f32 dt, Combat::System& combat, Player& player) {
    if (m_CombatStats.health <= 0) m_CombatFlags.isAlive = false;

    // apply knockback movement
    AEVec2 frameMove;
    AEVec2Scale(&frameMove, &m_KnockbackVelocity, dt);

    float knockbackSpeed = AEVec2Length(&m_KnockbackVelocity);

    // update facing and animation
    EvaluateCurrentDirection();
    m_EnemySprite.Sprite_Update(dt);
    m_DasherSprite.Sprite_Update(dt);

    if (!m_pMap || knockbackSpeed < 1.0f) {
        // no map or weak knockback, apply raw movement
        AEVec2Add(&m_pos, &m_pos, &frameMove);
    }
    else {
        float newX = m_pos.x + frameMove.x;
        float newY = m_pos.y + frameMove.y;

        if (!m_pMap->IsPositionBlocked(newX, newY, m_size)) {
            // full movement is clear
            m_pos.x = newX;
            m_pos.y = newY;
        }
        else {
            // reflect knockback when hitting walls
            bool xBlocked = m_pMap->IsPositionBlocked(m_pos.x + frameMove.x, m_pos.y, m_size);
            bool yBlocked = m_pMap->IsPositionBlocked(m_pos.x, m_pos.y + frameMove.y, m_size);

            if (xBlocked) m_KnockbackVelocity.x *= -0.5f;
            if (yBlocked) m_KnockbackVelocity.y *= -0.5f;
            if (!xBlocked && !yBlocked) {
                m_KnockbackVelocity.x *= -0.5f;
                m_KnockbackVelocity.y *= -0.5f;
            }

            // apply wall damage on strong impact
            constexpr float WALL_DAMAGE_SPEED_THRESHOLD = 500.0f;
            if (knockbackSpeed > WALL_DAMAGE_SPEED_THRESHOLD) {
                float wallDamage = m_CombatStats.maxHealth * 0.1f;
                m_CombatStats.health -= wallDamage;
                m_healthDepletionPercentage += wallDamage * 3.0f;
            }

            // resolve final safe position
            ResolveCollision(m_pos.x, m_pos.y, frameMove.x, frameMove.y, m_size, *m_pMap);
        }
    }

    (void)combat;
    AEVec2Scale(&m_KnockbackVelocity, &m_KnockbackVelocity, 0.85f);

    // update mark detonation timer
    if (m_markDetonating) {
        m_markDetonateTimer -= dt;
        if (m_markDetonateTimer <= 0.0f) {
            m_markDetonating = false;
            m_markDetonateTimer = 0.0f;
        }
    }

    // debug reposition
    if (AEInputCheckTriggered(AEVK_R)) {
        SetPosition(50.0f, 0.0f);
    }

    // update attack cooldown
    if (m_AttackCooldown <= 0.0f) {
        m_AttackCooldown = 0.0f;
        m_AllowAttack = true;
    }

    // start melee wind-up when attack is allowed
    if (m_EnableMelee &&
        m_CurrentState != EnemyState::STATE_DASH_WINDUP &&
        m_CurrentState != EnemyState::STATE_DASH &&
        !m_AttackActive && m_AllowAttack && !m_WindingUp &&
        m_CombatSystem.CanStartAttack_Enemy(player, *this))
    {
        m_WindingUp = true;
        m_WindUpTimer = m_WindUpDuration;
    }

    // process wind-up timer
    if (m_WindingUp) {
        m_WindUpTimer -= dt;
        if (m_WindUpTimer <= 0.0f) {
            m_WindingUp = false;
            m_WindUpTimer = 0.0f;
            StartAttack(this->m_AttackData);
        }
    }

    // update active attack frames
    if (m_AttackActive) {
        m_AttackCooldown = 2.0f;
        m_AttackFrameAccumulator += dt;

        while (m_AttackFrameAccumulator >= m_CombatSystem.GetOneFPS() &&
            m_AttackCurrentFrame != m_AttackTotalFrames) {
            ++m_AttackCurrentFrame;
            m_AttackFrameAccumulator -= static_cast<float>(m_CombatSystem.GetOneFPS());
        }

        m_attackProgress = 0.0f;

        if (m_AttackCurrentFrame < m_AttackStartUpFrames) {
            // startup phase
        }
        else if (m_AttackCurrentFrame < m_AttackStartUpFrames + m_AttackActiveFrames) {
            // active phase
            int activeFrameIndex{ m_AttackCurrentFrame - m_AttackStartUpFrames };
            m_attackProgress = float(activeFrameIndex) / (m_AttackActiveFrames - 1);
            m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, m_attackProgress);
        }
        else {
            // recovery phase
        }

        if (m_attackProgress >= 1.0f) {
            m_AttackActive = false;
            m_attackProgress = 1.0f;
        }
    }
    else {
        if (!m_CombatFlags.stunned) m_AttackCooldown -= dt;
        m_AllowAttack = false;
    }
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

    // set render mode
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // setup sprite scale and shadow
    float spriteScale = m_sizeMultiplier;
    float shadowY = m_pos.y;

    Shadow_Draw(m_pos.x, shadowY, m_size);

    // enemy sprite state selection
    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
        // flash red near the end of wind-up
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetEnemyAttackSpriteMesh(),
            m_EnemySprite.GetEnemyAttackSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
        // draw hit reaction frame
        m_EnemySprite.SetEnemyAttackSingleFrame(0, 6);

        DrawTexture(m_EnemySprite,
            m_EnemySprite.GetEnemyAttackSpriteMesh(),
            m_EnemySprite.GetEnemyAttackSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else {
        // draw normal enemy sprite
        DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection),
            m_EnemySprite.GetSpriteMesh(),
            m_EnemySprite.GetSpriteSheet(),
            m_EnemySprite.GetPixelScale(),
            m_EnemySprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }

    // draw melee weapon during active attack
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    if (m_AttackActive) DrawBat(swordAngle);

    // draw health bars
    f32 barWidth = m_size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth);
    f32 barHeight = m_size / 3.0f;
    f32 dbarWidth = m_size * 2.0f * AEClamp(
        (m_CombatStats.health / m_CombatStats.maxHealth) +
        (m_healthDepletionPercentage / 100.0f),
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

    // draw damaging mark visuals
    if (m_marked) {
        if (!m_markDetonating) {
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
                0
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
                1
            );
        }
    }

    // draw plus-one feedback on parry
    if (m_CombatFlags.parried) {
        m_EnemySprite.SetPlusOneFrame(0);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        AEGfxTextureSet(
            m_EnemySprite.GetPlusOneSpriteSheet(),
            m_EnemySprite.GetPlusOneU(),
            m_EnemySprite.GetPlusOneV()
        );

        AEMtx33 scale, rot, trans, final;
        AEMtx33Scale(&scale, 55.0f, 55.0f);
        AEMtx33Rot(&rot, 0.0f);
        AEMtx33Trans(&trans, m_pos.x, m_pos.y + m_size + 70.0f);

        AEMtx33Concat(&final, &rot, &scale);
        AEMtx33Concat(&final, &trans, &final);

        AEGfxSetTransform(final.m);
        AEGfxMeshDraw(m_EnemySprite.GetPlusOneSpriteMesh(), AE_GFX_MDM_TRIANGLES);
        m_EnemySprite.ResetPlusOneAnimation();
    }
}

void Enemy::StartAttack(Combat::CombatData::AttackData attackData) {
    // reset attack resolution flags
    m_CombatFlags.attackResolved = false;
    m_CombatFlags.blockResolved = false;
    m_CombatFlags.parryResolved = false;

    // initialize attack state
    m_AttackActive = true;
    m_AllowAttack = false;
    m_AttackFrameAccumulator = 0.0f;
    m_AttackCurrentFrame = 0;

    // setup attack cone angles
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
            return a.fCost > b.fCost;
        }
    };

    float Heuristic(GridPos const& a, GridPos const& b) {
        int dx = std::abs(a.col - b.col);
        int dy = std::abs(a.row - b.row);
        return static_cast<float>((std::max)(dx, dy)) +
            0.414f * static_cast<float>((std::min)(dx, dy));
    }
}

void Enemy::ComputePath(AEVec2 const& targetPos) {
    m_path.clear();
    m_pathIndex = 0;
    m_hasValidPath = false;
    m_lastTargetPos = targetPos;

    if (!m_pMap) return;

    // calculate tile clearance based on enemy size
    const float halfMin = (std::min)(GRID_W * 0.5f, GRID_H * 0.5f);
    int clearance = static_cast<int>(std::ceilf(m_size * 0.9f / halfMin)) - 1;
    if (clearance < 0) clearance = 0;

    GridPos start = m_pMap->WorldToTMX(m_pos.x, m_pos.y);
    GridPos goal = m_pMap->WorldToTMX(targetPos.x, targetPos.y);

    // adjust goal if blocked for this enemy size
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

    // adjust start if blocked for this enemy size
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

    const int maxIter = static_cast<int>(m_pMap->GetMapWidth()) *
        static_cast<int>(m_pMap->GetMapHeight());

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

        auto it = gScore.find(current.pos);
        if (it != gScore.end() && current.gCost > it->second)
            continue;

        for (int i = 0; i < 8; ++i) {
            GridPos neighbor{ current.pos.col + dc[i], current.pos.row + dr[i] };

            if (!m_pMap->IsWalkableForSize(neighbor.col, neighbor.row, clearance))
                continue;

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

    // reconstruct final path
    std::vector<GridPos> gridPath;
    GridPos cur = goal;
    while (cur != start) {
        gridPath.push_back(cur);
        cur = cameFrom[cur];
    }

    // convert grid path to world-space waypoints
    m_path.reserve(gridPath.size());
    for (int i = static_cast<int>(gridPath.size()) - 1; i >= 0; --i) {
        m_path.push_back(m_pMap->TMXToWorld(gridPath[i].col, gridPath[i].row));
    }

    m_pathIndex = 0;
    m_hasValidPath = true;
}

// local line-of-sight helper for path smoothing
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

    // skip waypoints already reached
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

    // look ahead for the furthest visible waypoint
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
        // no map, move directly toward the player
        m_pos.x += m_enemyToPlayerDir.x * m_speed * dt;
        m_pos.y += m_enemyToPlayerDir.y * m_speed * dt;
        return;
    }

    // recompute path when needed
    if (NeedsPathRecalc(targetPos, dt))
        ComputePath(targetPos);

    // detect if enemy is stuck and force path recalculation
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

    // follow current path
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

    // resolve movement against map collision
    float velX = dir.x * m_speed * dt;
    float velY = dir.y * m_speed * dt;
    float prevX = m_pos.x, prevY = m_pos.y;
    ResolveCollision(m_pos.x, m_pos.y, velX, velY, m_size, *m_pMap);

    // if blocked, try axis-by-axis sliding
    if (m_pos.x == prevX && velX != 0.0f) {
        ResolveCollision(m_pos.x, m_pos.y, 0.0f, velY, m_size, *m_pMap);
    }
    if (m_pos.y == prevY && velY != 0.0f) {
        ResolveCollision(m_pos.x, m_pos.y, velX, 0.0f, m_size, *m_pMap);
    }
}

// ------------------------
// | Child Class: Walker  |
// ------------------------
void Walker::ChildUpdate(f32 dt, Combat::System& combat, Player& player,
    std::vector<std::unique_ptr<Enemy>>& enemies) {
    (void)combat;
    (void)enemies;

    // compute direction to player
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2 toPlayer;
    AEVec2Sub(&toPlayer, &playerPos, &m_pos);

    f32 distToPlayer = AEVec2Length(&toPlayer);
    if (distToPlayer > 0.001f) {
        AEVec2Scale(&m_enemyToPlayerDir, &toPlayer, 1.0f / distToPlayer);
    }

    // rotate melee aim toward player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    const f32 desiredStopDistance = m_AttackRange * 0.6f;

    // chase until inside melee range
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
    // stagger dash timers so multiple dashers do not sync perfectly
    m_dashTimer = static_cast<f32>(rand()) / RAND_MAX * m_dashCD;
    m_CurrentState = EnemyState::STATE_IDLE;

    m_CombatStats.health = hp;
    m_CombatStats.maxHealth = hp;
}

void Dasher::StartDash(AEVec2 const& direction, f32 distToPlayer)
{
    // cancel wind-up and prepare dash state
    m_WindingUp = false;
    m_WindUpTimer = 0.0f;
    m_AllowAttack = false;

    m_moveDir = direction;
    m_dashActive = true;
    m_dashFrameAccumulator = 0.0f;
    m_dashCurrentFrame = 0;

    m_dashDirX = direction.x;
    m_dashDirY = direction.y;

    // stop within follow-up attack range instead of overshooting player
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

    // use easing to smooth dash speed over active frames
    f32 prevCurveT = EaseInOutSine(prevLinearT);
    f32 currCurveT = EaseInOutSine(currLinearT);

    f32 deltaT = currCurveT - prevCurveT;

    f32 dashVelX = m_dashTotalX * deltaT;
    f32 dashVelY = m_dashTotalY * deltaT;

    if (m_pMap)
    {
        // substep dash movement to avoid tunneling through walls
        const int steps = 8;
        f32 stepVelX = dashVelX / steps;
        f32 stepVelY = dashVelY / steps;

        for (int i = 0; i < steps; ++i)
        {
            f32 prevX = m_pos.x;
            f32 prevY = m_pos.y;

            ResolveCollision(m_pos.x, m_pos.y, stepVelX, stepVelY, m_size, *m_pMap);

            // stop dash early if blocked
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

    // advance dash animation frames
    while (m_dashFrameAccumulator >= m_CombatSystem.GetOneFPS() &&
        m_dashCurrentFrame <= m_dashTotalFrames)
    {
        ++m_dashCurrentFrame;
        m_dashFrameAccumulator -= static_cast<f32>(m_CombatSystem.GetOneFPS());

        if (m_dashCurrentFrame >= m_dashStartFrames &&
            m_dashCurrentFrame < m_dashStartFrames + m_dashActiveFrames)
        {
            ApplyDashStep(player);

            if (!m_dashActive)
                return;
        }
    }

    // dash finished, optionally queue follow-up attack
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

    // update direction to player
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 dist = AEVec2Length(&m_enemyToPlayerDir);

    if (dist > 0.001f)
        AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    if (m_CombatFlags.stunned)
        return;

    // dash cooldown only ticks while not already dashing
    if (m_CurrentState != EnemyState::STATE_DASH_WINDUP &&
        m_CurrentState != EnemyState::STATE_DASH)
    {
        m_dashTimer -= dt;
    }

    // dash wind-up state
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

    // active dash state
    if (m_CurrentState == EnemyState::STATE_DASH)
    {
        UpdateDash(dt, player);
        return;
    }

    // start dash if player is inside dash range
    if (m_dashTimer <= 0.0f &&
        dist >= m_dashMinRange &&
        dist <= m_dashRange &&
        !m_AttackActive &&
        !m_WindingUp)
    {
        m_lockedDashDir = m_enemyToPlayerDir;
        m_CurrentState = EnemyState::STATE_DASH_WINDUP;
        m_dashWindupTimer = m_dashWindupDuration;
        m_moveDir = { 0.0f, 0.0f };
        return;
    }

    // normal chase behaviour outside attack range
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

    // scale dasher sprite relative to base enemy size
    f32 spriteScale = m_sizeMultiplier * (m_size / ENEMY_SIZE);

    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    // draw dasher sprite based on state
    if (isAnyWindup && m_WindUpTimer < 0.35f && !isDashWindup) {
        DrawTexture(m_DasherSprite, static_cast<int>(m_CurrentDirection),
            m_DasherSprite.GetDasherAttackSpriteMesh(),
            m_DasherSprite.GetDasherAttackSpriteSheet(),
            m_DasherSprite.GetPixelScale(),
            m_DasherSprite.GetPixelScale(),
            m_pos.x, m_pos.y, 0.0f, spriteScale);
    }
    else if (m_CombatFlags.parried || m_CombatFlags.gotHit) {
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

    // overlay dash effect while actively dashing
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

    // draw melee weapon during active attack
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    if (m_AttackActive) DrawBat(swordAngle);

    // draw health bars
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

    // draw damaging mark visual
    if (m_marked) {
        if (!m_markDetonating) {
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
                0
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
                1
            );
        }
    }

    // draw plus-one feedback when parried
    if (m_CombatFlags.parried)
    {
        DrawTexture(
            m_EnemySprite,
            m_EnemySprite.GetPlusOneSpriteMesh(),
            m_EnemySprite.GetPlusOneSpriteSheet(),
            55.0f, 55.0f,
            m_pos.x,
            m_pos.y + m_size + 70.0f,
            0.0f,
            1.0f
        );
        m_DasherSprite.ResetPlusOneAnimation();
    }
}

// ---------------------
// | Child Class: Boss |
// ---------------------
Boss::Boss(AEVec2 pos, f32 size, f32 hp, f32 speed)
    : Enemy(pos, size, hp, speed)
{
    // initialize boss stats and phase growth values
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

    // prevent boss from dying before required phase transitions
    const f32 maxHP = m_CombatStats.maxHealth;
    const f32 phase3Threshold = maxHP * 0.2f;
    const f32 phase4Threshold = maxHP * 0.01f;

    // phase 1 / 2 protection
    if (!m_PhaseTwoTriggered && m_CombatStats.health <= 0.0f) {
        m_CombatStats.health = m_CombatStats.maxHealth * 0.5f;
        m_CombatFlags.isAlive = true;

        ForceGrowthHits(5);
        TriggerPhaseTwo(enemies);
        return;
    }

    // phase 2 / 3 protection
    if (m_PhaseTwoTriggered && !m_Phase3Triggered && m_CombatStats.health <= 0.0f) {
        m_CombatStats.health = phase3Threshold;
        m_CombatFlags.isAlive = true;
    }

    // phase 3 / 4 protection
    if (m_Phase3Triggered && !m_Phase4Triggered && m_CombatStats.health <= 0.0f) {
        m_CombatStats.health = phase4Threshold;
        m_CombatFlags.isAlive = true;
    }

    // phase 4 trigger must happen before phase 3 early return
    if (!m_Phase4Triggered &&
        m_Phase3Triggered &&
        m_CombatStats.health <= m_CombatStats.maxHealth * 0.01f) {
        TriggerPhaseFour(enemies);
        return;
    }

    // phase 4 owns boss behaviour completely
    if (m_Phase4Triggered) {
        UpdatePhaseFour(dt, player);
        return;
    }

    // phase 3 owns boss behaviour completely
    if (m_Phase3Triggered) {
        UpdatePhaseThree(dt, player, enemies);
        return;
    }

    // phase 2 transition blink state
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

    // register hit growth progression
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

    // update direction to player
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 distToPlayer = AEVec2Length(&m_enemyToPlayerDir);
    if (distToPlayer > 0.001f) {
        AEVec2Scale(&m_enemyToPlayerDir, &m_enemyToPlayerDir, 1.0f / distToPlayer);
    }

    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // move toward player until overlapping melee range
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
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // draw fake death phase 4 orb
    if (m_Phase4BallVisible) {
        Shadow_Draw(m_Phase4BallPos.x, m_Phase4BallPos.y + 10.0f, 22.0f);
        DrawMesh(m_enemyMesh, 26.0f, 26.0f, m_Phase4BallPos.x, m_Phase4BallPos.y, 0.0f,
            120, 255, 255, 255);
        return;
    }

    // hide during blink transitions
    if (m_Phase4Blinking && !m_Phase4BlinkVisible) {
        return;
    }
    if (m_Phase3Blinking && !m_Phase3BlinkVisible) {
        return;
    }
    if (m_PhaseTwoBlinking && !m_PhaseBlinkVisible) {
        return;
    }

    float spriteScale = m_sizeMultiplier * (m_size / 35.0f);

    // draw shadow lower to match larger boss body
    float shadowY = m_pos.y;
    shadowY += m_size * 0.75f;
    Shadow_Draw(m_pos.x, shadowY, m_size);

    bool isDashWindup = (m_CurrentState == EnemyState::STATE_DASH_WINDUP);
    bool isAnyWindup = (m_WindingUp || isDashWindup);

    if (m_UsePhaseTwoSprite) {
        // phase 2 onwards uses boss sprite set
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
        // before phase 2 uses base enemy sprite set
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

    // draw melee weapon when actively swinging
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    if (m_AttackActive) {
        DrawBat(swordAngle);
    }
}

void Enemy::EvaluateCurrentDirection()
{
    float angleDegrees = atan2(m_enemyToPlayerDir.y, m_enemyToPlayerDir.x) * 180.0f / PI;

    // rotate angle to match isometric facing directions
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

    // smoothly apply boss growth bonuses from hit milestones
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

    // rebuild attack range mesh when range changes
    if (m_AttackRangeMesh) {
        AEGfxMeshFree(m_AttackRangeMesh);
        m_AttackRangeMesh = nullptr;
    }
    m_AttackRangeMesh = CreateAttackRangeMesh(m_AttackRange, 0xFF0000);
}

void Boss::TriggerPhaseTwo(std::vector<std::unique_ptr<Enemy>>& enemies)
{
    (void)enemies;

    // begin phase 2 transition state
    m_PhaseTwoTriggered = true;
    m_PhaseTransitioning = true;

    m_PhaseTwoBlinking = true;
    m_PhaseBlinkTimer = 0.0f;
    m_PhaseBlinkVisible = true;

    if (!m_Phase2ChangeSFXPlayed) {
        gAudio.PlayGeneralSFX(GENERAL_BOSS_PHASE_CHANGE);
        m_Phase2ChangeSFXPlayed = true;
    }

    // delay sprite swap until blink transition completes
    m_UsePhaseTwoSprite = false;

    // stop all current actions
    m_moveDir = { 0.0f, 0.0f };
    m_AttackActive = false;
    m_WindingUp = false;
    m_AttackCooldown = 0.0f;
    m_KnockbackVelocity = { 0.0f, 0.0f };

    // heal boss to full
    m_CombatStats.health = m_CombatStats.maxHealth;
    m_healthDepletionPercentage = 0.0f;
}

void Boss::TriggerPhaseThree()
{
    if (m_Phase3Triggered) return;

    // begin phase 3 transition state
    m_Phase3Triggered = true;

    m_Phase3Transitioning = true;
    m_Phase3Blinking = true;
    m_Phase3BlinkVisible = true;

    m_RunToCenterPhase = true;
    m_Phase3ReachedCenter = false;
    m_IsThrowerPhase = false;

    m_Phase3BlinkTimer = 0.0f;
    m_Phase3HealTarget = m_CombatStats.maxHealth * 0.8f;

    // disable normal melee actions during transition
    m_EnableMelee = false;
    m_AttackActive = false;
    m_WindingUp = false;
    m_AllowAttack = false;
    m_moveDir = { 0.0f, 0.0f };
    m_KnockbackVelocity = { 0.0f, 0.0f };

    // setup run-to-center movement
    m_Phase3RunToCenterSpeed = 420.0f;
    m_Phase3ThrowerMoveSpeed = 5.0f;
    m_speed = m_Phase3RunToCenterSpeed;
}