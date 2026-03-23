#pragma once
#include "pch.h"

#include "Utils.h"
#include "Combat.h"
#include "Player.h"
#include "Sprite.h"
#include "Map.h"
#include "Projectile.h"

// ----------------
// | Enemy States |
// ----------------
enum class EnemyState : char
{
    STATE_IDLE,
    STATE_MOVING,
    STATE_ATTACK,
    STATE_PARRY,
    STATE_DEAD
};

enum class EnemyDirection : char
{
    DIRECTION_DOWN_RIGHT,
    DIRECTION_DOWN_LEFT,
    DIRECTION_UP_RIGHT,
    DIRECTION_UP_LEFT,
    DIRECTION_DOWN,
    DIRECTION_UP
};

// ---------------------
// | Base Class: Enemy |
// ---------------------
class Enemy
{
public:
    Enemy(AEVec2 pos, f32 size, f32 hp, f32 speed);
    virtual ~Enemy();

    void Init();
    void Update(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies) {
        BaseUpdate(dt, combat, player);
        ChildUpdate(dt, combat, player, enemies);
    }
    virtual void Draw();

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
    AEVec2 GetMoveDir() const { return m_moveDir; }

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
    bool GetIsAlive() const { return m_CombatFlags.isAlive; }

    void EvaluateCurrentDirection();

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

    void SetMap(const MapSystem* map) { m_pMap = map; }

    // Debug overlay getters
    const std::vector<AEVec2>& GetPath() const { return m_path; }
    int GetPathIndex() const { return m_pathIndex; }
    bool GetHasValidPath() const { return m_hasValidPath; }
    bool IsWindingUp() const { return m_WindingUp; }
    f32 GetSpeed() const { return m_speed; }

protected:
    float sizeMultiplier{ 1.5f };
    Sprite m_EnemySprite;
    AEGfxTexture* m_EnemySpriteSheet;
    AEGfxTexture* m_EnemyWindupSpriteSheet;
    AEGfxTexture* m_EnemyAttackSpriteSheet;

    // Enemy stats --------------------
    AEVec2 m_pos{};
    f32 m_hp{ 100.0f };
    f32 m_speed{ 270.0f };
    f32 m_size{ ENEMY_SIZE };
    f32 m_healthDepletionPercentage{};

    // Meshes -------------------------
    AEGfxVertexList* m_enemyMesh{ nullptr };
    AEGfxVertexList* m_AttackRangeMesh{ nullptr };
    AEGfxVertexList* m_enemyHealthBarMesh{ nullptr };

    // Attack Logic -------------------
    int m_LastAttackID{ -1 };
    bool m_AttackActive{ false };
    bool m_AllowAttack{ true };

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
    Combat::CombatFlags m_CombatFlags
    {
        true,
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
    Combat::CombatStats m_CombatStats
    {
        100.0f,
        30.0f,
        5.0f,
        0.0f,
        0.0f,
        0.0f,
        100.0f
    };

    float m_AttackFrameAccumulator{};
    int m_AttackCurrentFrame{};

    float m_StartDegree{ 30.0f };
    float m_EndDegree{ 30.0f };
    bool m_Recovered{ true };

    // Attack wind-up
    bool  m_WindingUp{ false };
    float m_WindUpTimer{ 0.0f };
    float m_WindUpDuration{ 0.6f };
    int m_AttackStartUpFrames{ 7 };
    int m_AttackActiveFrames{ 15 };
    int m_AttackRecoveryFrames{ 15 };
    int m_AttackTotalFrames{ m_AttackStartUpFrames + m_AttackActiveFrames + m_AttackRecoveryFrames };
    int m_Damage{ 20 };
    Combat::CombatData::AttackData m_AttackData
    {
        m_StartDegree,
        m_EndDegree,
        m_AttackStartUpFrames,
        m_AttackActiveFrames,
        m_AttackRecoveryFrames,
        m_AttackTotalFrames,
        m_Damage
    };



    // Animation
    EnemyDirection m_CurrentDirection;

    // Direction towards player -------
    AEVec2 m_enemyToPlayerDir{};
    AEVec2 m_moveDir{};

    // Mouse Aiming -------------------
    f32 m_DistMagPE{};
    AEVec2 m_VectorNormalizedPE{};
    AEVec2 m_AimVector{};
    f32 m_AimAngle{};

    const MapSystem* m_pMap = nullptr;

    // A* pathfinding
    std::vector<AEVec2> m_path;
    int m_pathIndex{ 0 };
    float m_pathTimer{ 0.0f };
    float m_pathRecalcInterval{ 0.5f };
    AEVec2 m_lastTargetPos{};
    bool m_hasValidPath{ false };

    // Stuck detection
    AEVec2 m_lastPos{};
    float m_stuckTimer{ 0.0f };
    static constexpr float STUCK_TIME_THRESHOLD = 0.3f;
    static constexpr float STUCK_DIST_THRESHOLD = 2.0f;

    void ComputePath(AEVec2 const& targetPos);
    AEVec2 FollowPath();
    bool NeedsPathRecalc(AEVec2 const& targetPos, f32 dt);
    void MoveTowardTarget(AEVec2 const& targetPos, f32 dt);

    void BaseUpdate(f32 dt, Combat::System& combat, Player& player);
    virtual void ChildUpdate(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies) = 0;
};

// -----------------------
// | Child Class: Walker |
// -----------------------
class Walker : public Enemy
{
public:
    using Enemy::Enemy;

protected:
    void ChildUpdate(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies) override;
};

// -----------------------
// | Child Class: Dasher |
// -----------------------
class Dasher : public Enemy
{
public:
    Dasher(AEVec2 pos, f32 size, f32 hp, f32 speed, f32 dashCD);

protected:
    f32 m_dashCD{ 3.0f };
    f32 m_dashTimer{ 0.0f };
    f32 m_dashRange{ 400.0f };
    f32 m_dashMinRange{ 80.0f };
    f32 m_dashDistance{ 200.0f };

    AEVec2 m_DashDir{};
    float m_DashStepX{};
    float m_DashStepY{};

    bool m_AllowDash{ true };
    bool m_DashActive{ false };
    float m_DashFrameAccumulator{};
    float m_DashCurrentFrame{};

    int m_StartFrames{ 2 };
    int m_ActiveFrames{ 4 };
    int m_RecoveryFrames{ 6 };
    bool m_Recovered{ true };
    int m_TotalFrames{ m_StartFrames + m_ActiveFrames + m_RecoveryFrames };
    Combat::CombatData::MovementData m_MovementData
    {
        m_StartFrames,
        m_ActiveFrames,
        m_RecoveryFrames,
        m_TotalFrames
    };
    Combat::CombatData::MovementState m_MovementState
    {
        m_Recovered
    };

    //void PerformDash(AEVec2 const& direction);
    void StartDash(AEVec2 const& direction);
    void ApplyDashStep();
    void UpdateDash(float dt, Combat::System& combat);
    void ChildUpdate(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies) override;
};

// ---------------------
// | Child Class: Boss |
// ---------------------
class Boss : public Enemy
{
public:
    Boss(AEVec2 pos, f32 size, f32 hp, f32 speed);

protected:
    void ChildUpdate(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies) override;
};

// ------------------------
// | Child Class: Thrower |
// ------------------------
class Thrower : public Enemy {
public:
    Thrower(AEVec2 pos, f32 size, f32 hp, f32 speed);
    void Draw() override;

protected:
    void ChildUpdate(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies) override;
    void ThrowProjectile(AEVec2 const& targetPos);
    void UpdateProjectiles(f32 dt, Combat::System& combat, Player& player,
        std::vector<std::unique_ptr<Enemy>>& enemies);
    void CleanupProjectiles();

    std::vector<Projectile> m_projectiles{};

    f32 m_throwCooldown{ 2.0f };
    f32 m_throwTimer{ 0.0f };
    f32 m_minThrowRange{ 0.0f };
    f32 m_maxThrowRange{ 500.0f };
    f32 m_projectileSpeed{ 500.0f };
    f32 m_projectileRadius{ 8.0f };
    f32 m_projectileDamage{ 10.0f };
    f32 m_projectileLifetime{ 3.0f };
    ProjectileType m_projectileType{ ProjectileType::Reflect };
};