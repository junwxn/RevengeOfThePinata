#pragma once
#include "Utils.h"
#include "Combat.h"
#include "Player.h"

// ----------------
// | Enemy States |
// ----------------
enum class EnemyState {
    STATE_IDLE,
    STATE_MOVING,
    STATE_ATTACK,
    STATE_PARRY,
    STATE_DEAD
};

// ---------------------
// | Base Class: Enemy |
// ---------------------
class Enemy {
public:
    Enemy(AEVec2 pos, f32 size, f32 hp, f32 speed); // Base constructor
    virtual ~Enemy(); // Base destructor

    void Init();
    void Update(f32 dt, Combat::System& combat, Player const& player) {
        BaseUpdate(dt, combat, player);
        ChildUpdate(dt, combat, player);
    }
    void Draw();

    void StartAttack();
    void DamageInfo();

    // Getters ------------------------
    f32 GetX() const { return m_pos.x; }
    f32 GetY() const { return m_pos.y; }
    f32 GetSize() const { return m_size; }

    AEVec2 GetNormalizedVector() const { return m_VectorNormalizedPE; }
    f32 GetDistMag() const { return m_DistMagPE; }
    AEVec2 GetAimVector() const { return m_AimVector; }
    f32 GetAimAngle() const { return m_AimAngle; }

    bool IsAttacking() const { return m_AttackActive; }
    bool CanAttack() const { return m_AllowAttack; }
    f32 GetAttackRange() const { return m_AttackRange; }
    f32 GetConeThreshold() const { return m_ConeThreshold; }
    f32 GetAttackProgress() const { return m_attackProgress; }

    Combat::CombatFlags GetCombatFlag() const { return m_CombatFlags; }
    Combat::CombatStats GetCombatStats() const { return m_CombatStats; }

    // Setters ------------------------
    void SetPosition(f32 x, f32 y) { m_pos.x = x; m_pos.y = y; }
    void SetAimVector(f32 x, f32 y) { m_AimVector.x = x, m_AimVector.y = y; }
    void SetAimAngle(f32 angle) { m_AimAngle = angle; }

    // Flag Setters
    void SetParried(bool set) { m_CombatFlags.parried = set; }
    void ResetParryFlag() { m_CombatFlags.parried = false; }
    
    void MarkAttackResolved() {
        m_CombatFlags.attackResolved = true;
        m_CombatFlags.parryResolved = true;
        m_CombatFlags.blockResolved = true;
    }

protected:
    // Enemy stats --------------------
    AEVec2 m_pos{};
    f32 m_hp{ 100.0f };
    f32 m_speed{ 300.0f };
    f32 m_size{ 40.0f };

    // Meshes -------------------------
    AEGfxVertexList* m_enemyMesh{ nullptr };
    AEGfxVertexList* m_AttackRangeMesh{ nullptr };

    // Attack Logic -------------------
    bool  m_AttackActive{ false };
    bool  m_AllowAttack{ true };
    f32 m_AttackCooldown{};
    f32 m_AttackDuration{ 0.15f };
    f32 m_AttackTimer{};
    f32 m_AttackRange{ 125.0f };
    f32 m_ConeHalfAngleDeg{ 30.0f };
    f32 m_ConeThreshold{};
    f32 m_StartAngle{};
    f32 m_EndAngle{};
    f32 m_CurrentAngle{};
    f32 m_attackProgress{};

    Combat::System m_combatSystem;
    Combat::CombatStats m_CombatStats{ 10.0f, 5.0f };
    Combat::CombatFlags m_CombatFlags{ false, false, false, false, false, false, false, false };

    // Damage Logic -------------------
    
    // Mouse Aiming -------------------
    f32 m_DistMagPE{};
    AEVec2 m_VectorNormalizedPE{};
    AEVec2 m_AimVector{};
    f32 m_AimAngle{};

    void BaseUpdate(f32 dt, Combat::System& combat, Player const& player);
    virtual void ChildUpdate(f32 dt, Combat::System& combat, Player const& player) = 0;
};

 //-----------------------
 //| Child Class: Walker |
 //-----------------------
class Walker : public Enemy {
public:
    using Enemy::Enemy; // Inherit base constructor



protected:
    void ChildUpdate(f32 dt, Combat::System& combat, Player const& player) override;
};


// -----------------------
// | Child Class: Dasher |
// -----------------------
class Dasher : public Enemy {
public:
    Dasher(AEVec2 pos, f32 size, f32 hp, f32 speed, f32 dashCD); // Child constructor

protected:
    f32 m_dashCD{ 0.1f };

    void ChildUpdate(f32 dt, Combat::System& combat, Player const& player) override;
};
