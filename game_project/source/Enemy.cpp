#include "pch.h"

#include "Enemy.h"
#include "Player.h"
#include "MathFunctions.h"
#include "Utils.h"
#include "Map.h"
#include <cmath>
#include <queue>
#include <unordered_map>
#include "Raycast.h"
#include "Sprite.h"

//std::ostream& operator<<(std::ostream& os, CombatOutcome outcome) {
//    if (outcome == CombatOutcome::OUTCOME_HIT) return os << "OUTCOME_HIT";
//    else if (outcome == CombatOutcome::OUTCOME_BLOCKED) return os << "OUTCOME_BLOCKED";
//    else if (outcome == CombatOutcome::OUTCOME_PARRIED) return os << "OUTCOME_PARRIED";
//    return os << static_cast<int>(outcome);
//}

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
    if (!m_AttackActive && m_AllowAttack && !m_WindingUp && m_CombatSystem.CanStartAttack_Enemy(player, *this))
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

void Enemy::Draw() {
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Calculate isometric squashed height for drawing
        //f32 isoHeight = m_size * (GRID_H / GRID_W); // Squashed
        f32 isoHeight = m_size; // Normal

    // Draw Meshes --------------------
    // Enemy
    if (m_WindingUp && m_WindUpTimer < 0.2f) {
        // Flash red just before attacking (last 0.2s of wind-up)
        float flash = sinf(m_WindUpTimer * 40.0f);
        float r = (flash > 0.0f) ? 255.0f : 180.0f;
        DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, r, 0, 0, 255);
    }
    else if (m_WindingUp) {
        // Winding up — tint yellow to show charging
        DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 200, 0, 255);
    }
    else if (m_CombatFlags.parried) {
        DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 0, 0, 255);
    }
    else {
        DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 44, 255, 255, 255);
    }

    // Enemy sword
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    DrawMesh(m_AttackRangeMesh, 1.0f , 5.0f, m_pos.x, m_pos.y, swordAngle,
        44, 255, 255, 255);

    // Enemy health bar
    f32 barWidth = m_size * 2.0f * m_CombatStats.health / m_CombatStats.maxHealth;
    f32 barHeight = m_size / 3.0f;
    f32 dbarWidth = m_size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth + m_healthDepletionPercentage / m_CombatStats.maxHealth);
    f32 dRate = m_CombatStats.maxHealth * dt;
    if (m_healthDepletionPercentage >= 0.0f) { m_healthDepletionPercentage -= dRate; };

    DrawMesh(m_enemyHealthBarMesh, dbarWidth, barHeight, m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 5.0f, 0.0f, 255, 175, 65, 255); // Depleting bar
    DrawMesh(m_enemyHealthBarMesh, barWidth, barHeight, m_pos.x - m_size, m_pos.y + m_size + barHeight / 2.0f + 5.0f, 0.0f, 210, 70, 75, 255); // Instant bar

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
    DrawTexture(m_EnemySprite, static_cast<int>(m_CurrentDirection), m_EnemySprite.GetSpriteMesh(), m_EnemySprite.GetSpriteSheet(), m_EnemySprite.GetPixelScale(),
                m_EnemySprite.GetPixelScale(), m_pos.x, m_pos.y, 0.0f);
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
        int lookAhead = (std::min)(m_pathIndex + 1, static_cast<int>(m_path.size()) - 1);
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
            m_pathTimer = 0.0f; // force recalc next frame
            m_stuckTimer = 0.0f;
        }
    } else {
        m_stuckTimer = 0.0f;
    }
    m_lastPos = m_pos;

    AEVec2 dir = FollowPath();
    float len = AEVec2Length(&dir);
    if (len < 0.001f) {
        // Fallback to direct seek if no valid path
        dir = m_enemyToPlayerDir;
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
    else if (m_pos.y == prevY && velY != 0.0f) {
        // Y was blocked, let X slide
        ResolveCollision(m_pos.x, m_pos.y, velX, 0.0f, m_size, *m_pMap);
    }
}

//------------------------
 //| Child Class: Walker |
 //-----------------------
void Walker::ChildUpdate(f32 dt, Combat::System& combat, Player& player) {
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

// -----------------------
// | Child Class: Dasher |
// -----------------------
Dasher::Dasher(AEVec2 pos, f32 size, f32 hp, f32 speed, f32 dashCD)
    : Enemy(pos, size, hp, speed), m_dashCD{ dashCD }
{
    // Stagger initial timers so grouped Dashers don't all lunge at once
    m_dashTimer = static_cast<f32>(rand()) / RAND_MAX * m_dashCD;
}

void Dasher::PerformDash(AEVec2 const& direction) {
    m_moveDir = direction;
    if (!m_pMap) {
        m_pos.x += direction.x * m_dashDistance;
        m_pos.y += direction.y * m_dashDistance;
        return;
    }

    // Step-wise collision (same pattern as player dash)
    const int steps = static_cast<int>(m_dashDistance / 16.0f) + 1;
    float stepVelX = direction.x * m_dashDistance / steps;
    float stepVelY = direction.y * m_dashDistance / steps;
    for (int i = 0; i < steps; ++i) {
        float prevX = m_pos.x, prevY = m_pos.y;
        ResolveCollision(m_pos.x, m_pos.y, stepVelX, stepVelY, m_size, *m_pMap);
        if (m_pos.x == prevX && m_pos.y == prevY) break;
    }
}

void Dasher::ChildUpdate(f32 dt, Combat::System& combat, Player& player) {
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 dist = AEVec2Length(&m_enemyToPlayerDir);
    if (dist > 0.001f)
        AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    // Point sword towards player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // Skip movement if stunned, winding up, or attacking
    if (m_CombatFlags.stunned || m_WindingUp || m_AttackActive) return;

    // Tick dash cooldown
    m_dashTimer -= dt;

    // Dash if cooldown ready and player in range
    if (m_dashTimer <= 0.0f && dist >= m_dashMinRange && dist <= m_dashRange) {
        PerformDash(m_enemyToPlayerDir);
        m_dashTimer = m_dashCD;
        return;
    }

    // Walk toward player via A* pathfinding (same as Walker/Boss)
    if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
                                m_pos.x, m_pos.y, m_size)) {
        MoveTowardTarget(playerPos, dt);
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

void Boss::ChildUpdate(f32 dt, Combat::System& combat, Player& player) {
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
}

void Thrower::Draw() {
    Enemy::Draw();

    for (Projectile const& projectile : m_projectiles) {
        projectile.Draw();
    }
}

void Thrower::ChildUpdate(f32 dt, Combat::System& combat, Player& player) {
    // Update direction vector, get dist bw enemy and player
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    f32 dist = AEVec2Length(&m_enemyToPlayerDir);
    if (dist > 0.001f) AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);
   
    // Angle to draw gun/projectile
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // Check projectile collision / projectile movement
    UpdateProjectiles(dt, combat, player);
    // Remove inactive projectiles (eg hit wall, hit player, lifetime 0)
    CleanupProjectiles();

    if (m_CombatFlags.stunned) {
        return;
    }

    m_throwTimer -= dt;

    bool hasLOS = true;
    if (m_pMap) hasLOS = HasLineOfSight_Grid(m_pos, playerPos, *m_pMap);

    if (dist > m_maxThrowRange) {
        MoveTowardTarget(playerPos, dt);
        return;
    }

    if (dist >= m_minThrowRange && hasLOS && m_throwTimer <= 0.0f) {
        ThrowProjectile(playerPos);
        m_throwTimer = m_throwCooldown;
    }
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

    m_projectiles.back().Init();
}

// Projectile collision check
void Thrower::UpdateProjectiles(f32 dt, Combat::System& combat, Player& player) {
    for (Projectile& projectile : m_projectiles) {
        projectile.Update(dt, m_pMap);

        if (!projectile.IsActive()) {
            continue;
        }

        AEVec2 projectilePos = projectile.GetPosition();

        if (AreCirclesIntersecting(projectilePos.x, projectilePos.y, projectile.GetRadius(),
            player.GetX(), player.GetY(), player.GetSize())) {
            combat.ApplyProjectileDamage(player, *this, projectile.GetDamage());
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
