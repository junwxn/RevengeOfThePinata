#include "pch.h"

#include "Enemy.h"
#include "Player.h"
#include "MathFunctions.h"
#include "Utils.h"
#include "Map.h"

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
    m_AttackRangeMesh = CreateAttackRangeMesh(m_AttackRange, 0xFF0000);
    m_enemyMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
    m_enemyHealthBarMesh = CreateRectMesh(0xAEF359);
    m_markMesh = CreateRectMesh(0xFFFFFF);
}

void Enemy::BaseUpdate(f32 dt, Combat::System& combat, Player const& player) {

    if (m_CombatStats.health <= 0) m_CombatFlags.isAlive = false;
    //std::cout << "BASE RUNNING" << std::endl;

    // --- Knockback with wall-bounce ---
    AEVec2 frameMove;
    AEVec2Scale(&frameMove, &m_KnockbackVelocity, dt);

    float knockbackSpeed = AEVec2Length(&m_KnockbackVelocity);

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

//------------------------
 //| Child Class: Walker |
 //-----------------------
void Walker::ChildUpdate(f32 dt, Combat::System& combat, Player const& player) {
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    // Point sword towards player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // Seek player
    if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
                                m_pos.x, m_pos.y, m_size)) {
        float velX = m_enemyToPlayerDir.x * m_speed * dt;
        float velY = m_enemyToPlayerDir.y * m_speed * dt;

        if (m_pMap) {
            ResolveCollision(m_pos.x, m_pos.y, velX, velY, m_size, *m_pMap);
        } else {
            m_pos.x += velX;
            m_pos.y += velY;
        }
    }
}

// -----------------------
// | Child Class: Dasher |
// -----------------------
Dasher::Dasher(AEVec2 pos, f32 size, f32 hp, f32 speed, f32 dashCD)
    : Enemy(pos, size, hp, speed), m_dashCD{ dashCD } {}

void Dasher::ChildUpdate(f32 dt, Combat::System& combat, Player const& player) {
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    //AEVec2 enemyToPlayer;
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    // Point sword towards player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);
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

void Boss::ChildUpdate(f32 dt, Combat::System& combat, Player const& player) {
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2Sub(&m_enemyToPlayerDir, &playerPos, &m_pos);
    AEVec2Normalize(&m_enemyToPlayerDir, &m_enemyToPlayerDir);

    // Point sword towards player
    m_AimAngle = atan2(-m_enemyToPlayerDir.y, -m_enemyToPlayerDir.x);

    // Seek player (slower than Walker)
    if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
                                m_pos.x, m_pos.y, m_size)) {
        float velX = m_enemyToPlayerDir.x * m_speed * dt;
        float velY = m_enemyToPlayerDir.y * m_speed * dt;

        if (m_pMap) {
            ResolveCollision(m_pos.x, m_pos.y, velX, velY, m_size, *m_pMap);
        } else {
            m_pos.x += velX;
            m_pos.y += velY;
        }
    }
}