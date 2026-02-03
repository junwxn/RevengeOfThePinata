#include <iostream>
#include <cmath>

#include "Enemy.h"
#include "Player.h"
#include "MathFunctions.h"
#include "Utils.h"

//std::ostream& operator<<(std::ostream& os, CombatOutcome outcome) {
//    if (outcome == CombatOutcome::OUTCOME_HIT) return os << "OUTCOME_HIT";
//    else if (outcome == CombatOutcome::OUTCOME_BLOCKED) return os << "OUTCOME_BLOCKED";
//    else if (outcome == CombatOutcome::OUTCOME_PARRIED) return os << "OUTCOME_PARRIED";
//    return os << static_cast<int>(outcome);
//}

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
}

void Enemy::Init() {
    m_AttackRangeMesh = CreateAttackRangeMesh(m_AttackRange, 0xFF0000);
    m_enemyMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
}

void Enemy::BaseUpdate(f32 dt, Combat::System& combat, Player const& player) {

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
    if (!m_AttackActive && m_AllowAttack && m_combatSystem.CanStartAttack_Enemy(player, *this))
    {
        StartAttack();
    }
    //std::cout << "m_AttackActive: " << m_AttackActive << std::endl;

    // Update attack ------------------
    if (m_AttackActive)
    {
        m_AttackCooldown = 2.0f;
        m_AttackTimer += dt;

        // For normalized value between 0.0 - 1.0 range
        // 0.0 = attack start
        // 0.5 = halfway through attack
        // 1.0 = attack complete
        m_attackProgress = m_AttackTimer / m_AttackDuration;

        m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, m_attackProgress);

        //std::cout << "m_attackProgress: " << m_attackProgress << std::endl;

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

}

void Enemy::Draw() {
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Calculate isometric squashed height for drawing
    f32 isoHeight = m_size * (GRID_H / GRID_W);

    // Draw Meshes --------------------
    // Enemy
    if (!m_CombatFlags.stunned && m_AttackCooldown >= 0.5f) DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 44, 255, 255, 255);
    else if(m_AttackCooldown < 0.5f) DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 255, 0, 255);
    else DrawMesh(m_enemyMesh, m_size, isoHeight, m_pos.x, m_pos.y, 0.0f, 255, 0, 0, 255);

    // Enemy sword
    f32 swordAngle = m_AttackActive ? m_CurrentAngle : m_AimAngle;
    DrawMesh(m_AttackRangeMesh, 1.0f , 5.0f, m_pos.x, m_pos.y, swordAngle,
        44, 255, 255, 255);
}

void Enemy::StartAttack() {
    m_CombatFlags.attackResolved = false;
    m_CombatFlags.blockResolved = false;
    m_CombatFlags.parryResolved = false;

    m_AttackActive = true;
    m_AllowAttack = false;
    m_AttackTimer = 0.0f;

    // 60-degree cone
    m_CurrentAngle = m_AimAngle;
    m_StartAngle = m_AimAngle - AEDegToRad(30.0f);
    m_EndAngle = m_AimAngle + AEDegToRad(30.0f);
}

void Enemy::DamageInfo() {

}

//------------------------
 //| Child Class: Walker |
 //-----------------------
void Walker::ChildUpdate(f32 dt, Combat::System& combat, Player const& player) {
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2 enemyToPlayer;
    AEVec2Sub(&enemyToPlayer, &playerPos, &m_pos);
    AEVec2Normalize(&enemyToPlayer, &enemyToPlayer);

    // Point sword towards player
    m_AimAngle = atan2(-enemyToPlayer.y, -enemyToPlayer.x);

    // Seek player
    if (!AreCirclesIntersecting(player.GetX(), player.GetY(), player.GetSize(),
                                m_pos.x, m_pos.y, m_size)) {
        AEVec2Scale(&enemyToPlayer, &enemyToPlayer, m_speed);
        AEVec2Scale(&enemyToPlayer, &enemyToPlayer, dt);

        // Move enemy towards player
        if(!m_CombatFlags.stunned) AEVec2Add(&m_pos, &m_pos, &enemyToPlayer);
    }
}

// -----------------------
// | Child Class: Dasher |
// -----------------------
Dasher::Dasher(AEVec2 pos, f32 size, f32 hp, f32 speed, f32 dashCD)
    : Enemy(pos, size, hp, speed), m_dashCD{ dashCD } {}

void Dasher::ChildUpdate(f32 dt, Combat::System& combat, Player const& player) {
    AEVec2 playerPos{ player.GetX(), player.GetY() };
    AEVec2 enemyToPlayer;
    AEVec2Sub(&enemyToPlayer, &playerPos, &m_pos);
    AEVec2Normalize(&enemyToPlayer, &enemyToPlayer);

    // Point sword towards player
    m_AimAngle = atan2(-enemyToPlayer.y, -enemyToPlayer.x);
}