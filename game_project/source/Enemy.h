#pragma once
#include "Utils.h"
#include "Combat.h"
#include "Player.h"

class MapSystem; // Forward-declare to avoid a circular header chain

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

    void StartAttack(Combat::CombatData::AttackData);
    void DamageInfo();

    // Getters ------------------------
    f32 GetX() const { return m_pos.x; }
    f32 GetY() const { return m_pos.y; }
    f32 GetSize() const { return m_size; }
    AEVec2 GetEnemyPosition() const { return m_pos; }

    AEVec2 GetNormalizedVector() const { return m_VectorNormalizedPE; }
    f32 GetDistMag() const { return m_DistMagPE; }
    AEVec2 GetAimVector() const { return m_AimVector; }
    f32 GetAimAngle() const { return m_AimAngle; }
    AEVec2 GetEnemyDirectionVector() const { return m_enemyToPlayerDir; }

    bool IsAttacking() const { return m_AttackActive; }
    bool IsStunned() const { return m_CombatFlags.stunned; }
    bool IsParried() const { return m_CombatFlags.parried; }
    bool IsGotHit() const { return m_CombatFlags.gotHit; }
    bool CanAttack() const { return m_AllowAttack; }
    f32 GetAttackRange() const { return m_AttackRange; }
    f32 GetConeThreshold() const { return m_ConeThreshold; }
    f32 GetAttackProgress() const { return m_attackProgress; }
    int GetLastAttackID() const { return m_LastAttackID; }

    f32 GetHDP() const { return m_healthDepletionPercentage; }

    AEGfxVertexList* GetEnemyMesh() const { return m_enemyMesh; }

    Combat::CombatFlags GetCombatFlag() const { return m_CombatFlags; }
    Combat::CombatStats GetCombatStats() const { return m_CombatStats; }

    // Setters ------------------------
    void SetPosition(f32 x, f32 y) { m_pos.x = x; m_pos.y = y; }
    void SetAimVector(f32 x, f32 y) { m_AimVector.x = x, m_AimVector.y = y; }
    void SetAimAngle(f32 angle) { m_AimAngle = angle; }
    void SetKnockback(AEVec2 knockbackVec) { AEVec2Add(&m_pos, &m_pos, &knockbackVec); }
    void SetKnockbackVelocity(AEVec2 setVelocity) { m_KnockbackVelocity = setVelocity; }
    void SetLastAttackID(int newID) { m_LastAttackID = newID; }
    void SetHDP(f32 dmg) { m_healthDepletionPercentage += dmg; }

    // Flag Setters
    void SetParried(bool set) { m_CombatFlags.parried = set; }
    void SetStunned(bool set) { m_CombatFlags.stunned = set; }
    void SetGotHit(bool set) { m_CombatFlags.gotHit = set; }
    void ResetParryFlag() { m_CombatFlags.parried = false; }
    void ResetStunFlag() { m_CombatFlags.stunned = false; }
    void ResetGotHitFlag() { m_CombatFlags.gotHit = false; }
    
    void MarkAttackResolved() {
        m_CombatFlags.attackResolved = true;
        m_CombatFlags.parryResolved = true;
        m_CombatFlags.blockResolved = true;
    }

    // Combat -------------------------
    void DeductHealth(f32 damage) { m_CombatStats.health -= damage; }

    // Augment: Damaging Mark
    bool m_marked = false;
    float m_markTimer = 0.0f;
    float m_markAccumulatedDamage = 0.0f;

    // Augment: Damaging Mark (visual)
    float m_markDetonateTimer = 0.0f;
    bool  m_markDetonating = false;
    AEGfxVertexList* m_markMesh{ nullptr };

    // Augment: Amplified Damage
    bool m_damageAmplified = false;
    float m_amplifyTimer = 0.0f;
    float m_damageMultiplier = 1.0f;

    // Call once after the map is loaded so enemies can self-resolve wall collisions.
    void SetMap(const MapSystem* map) { m_pMap = map; }

protected:
    // Enemy stats --------------------
    AEVec2 m_pos{};
    f32 m_hp{ 100.0f }; // to be removed?
    f32 m_speed{ 300.0f };
    f32 m_size{ 40.0f };
    f32 m_healthDepletionPercentage{};
    // Meshes -------------------------
    AEGfxVertexList* m_enemyMesh{ nullptr };
    AEGfxVertexList* m_AttackRangeMesh{ nullptr };
    AEGfxVertexList* m_enemyHealthBarMesh{ nullptr };

    // Attack Logic -------------------
    int m_LastAttackID{ -1 };
    bool  m_AttackActive{ false };
    bool  m_AllowAttack{ true };

    AEVec2 m_KnockbackVelocity{};

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

    Combat::System m_CombatSystem;
    //Combat::CombatStats m_CombatStats{ 10.0f, 5.0f };
    Combat::CombatFlags m_CombatFlags
    { 
        false, 
        false, 
        false, 
        false, 
        false, 
        false, 
        false, 
        false, 
        false, 
        false 
    };

    int m_AttackStopFrames{};
    int m_DefendStopFrames{};
    Combat::CombatStats m_CombatStats{
    100.0f, // health
    30.0f, // attack
    5.0f, // defense
    0.0f, // crit chance
    0.0f, // crit multiplier
    0.0f, // attack multiplier
    100.0f // max health
    };
    // Combat::CombatFlags m_CombatFlags{ false, false, false, false, false, false, false, false, false };
    
    float m_AttackFrameAccumulator{};
    int m_AttackCurrentFrame{};
    
    float m_StartDegree{ 30.0f };
    float m_EndDegree{ 30.0f };
    bool m_Recovered{ true };

    // Attack wind-up
    bool  m_WindingUp{ false };
    float m_WindUpTimer{ 0.0f };
    float m_WindUpDuration{ 0.6f }; // seconds to charge before swinging
    int m_AttackStartUpFrames{ 7 };
    int m_AttackActiveFrames{ 15 };
    int m_AttackRecoveryFrames{ 15 };
    int m_AttackTotalFrames{ m_AttackStartUpFrames + m_AttackActiveFrames + m_AttackRecoveryFrames };
    int m_Damage{ 20 };
    Combat::CombatData::AttackData m_AttackData
    {
        m_StartDegree,           // startAngle
        m_EndDegree,             // endAngle
        m_AttackStartUpFrames,   // startUp
        m_AttackActiveFrames,    // active
        m_AttackRecoveryFrames,  // recovery
        m_AttackTotalFrames,     // total
        m_Damage                 // damage
    };

    // Damage Logic -------------------

    // Direction towards player -------
    AEVec2 m_enemyToPlayerDir{};

    // Mouse Aiming -------------------
    f32 m_DistMagPE{};
    AEVec2 m_VectorNormalizedPE{};
    AEVec2 m_AimVector{};
    f32 m_AimAngle{};

    // Non-owning pointer to the active map; set via SetMap().
    const MapSystem* m_pMap = nullptr;

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

// ---------------------
// | Child Class: Boss |
// ---------------------
class Boss : public Enemy {
public:
    Boss(AEVec2 pos, f32 size, f32 hp, f32 speed);

protected:
    void ChildUpdate(f32 dt, Combat::System& combat, Player const& player) override;
};
