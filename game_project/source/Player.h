#pragma once
#include <vector>
#include <memory>

#include "Utils.h" // Access to AE system and Grid constants
#include "Combat.h"
#include "Enemy.h"
#include "Augments.h"
#include "AugmentData.h"
class MapSystem; // Forward-declare to avoid a circular header chain

// Persistent attack charges across levels
extern int g_PlayerAttackCharges;
inline constexpr int DEFAULT_ATTACK_CHARGES = 5;


//-----------------------//
//---- Player States ----//
//-----------------------//
enum class PlayerState : char {
    STATE_IDLE,
    STATE_MOVING,
    STATE_ATTACK,
    STATE_BLOCK,
    STATE_PARRY,
    STATE_DEAD,
    STATE_DASH
};

enum class PlayerDirection : char
{
    DIRECTION_DOWN_LEFT,
    DIRECTION_LEFT,
    DIRECTION_UP_LEFT,
    DIRECTION_UP,
    DIRECTION_UP_RIGHT,
    DIRECTION_RIGHT,
    DIRECTION_DOWN_RIGHT,
    DIRECTION_DOWN,
};

class Enemy;

class Player
{
public:
    void Init();
    void Update(float dt, Combat::System& combat, std::vector<std::unique_ptr<Enemy>> const& wave, f32 camX, f32 camY, bool preventing_movement);
    void Draw();
    void Free();

    bool IsAttacking() const { return m_AttackActive; }
    void StartAttack(Combat::CombatData::AttackData&, std::vector<std::unique_ptr<Enemy>> const&);

    bool IsBlocking() const { return m_BlockActive; }
    void StartBlock(Combat::CombatData::BlockData&, std::vector<std::unique_ptr<Enemy>> const&);

    bool IsDashing() const { return m_DashActive; }
    void StartDash(float moveX, float moveY, float dirX, float dirY);
    void ApplyDashStep();

    void GainAttackCharge() {
        ++m_AttackCharges;
        if (m_AttackCharges > m_MaxAttackCharge) {
            m_AttackCharges = m_MaxAttackCharge;
        }
        std::cout << "Attack Charges: " << m_AttackCharges << std::endl;
    }

    void ResetCombatVariables();

    // Projectile parry
    bool CanParryPoint(AEVec2 const& point) const;
    AEVec2 GetParryDirection() const;

    // Getters allowing Game.cpp to access position for Camera/Collisions
    float GetX() const { return m_PosX; }
    float GetY() const { return m_PosY; }
    float GetSize() const { return m_Size; }
    PlayerState GetState() const { return m_CurrentState; }

    // Getters for Combat related purposes
    // Combat Vectors
    AEVec2 GetNormalizedVector() const { return m_VectorNormalizedMP; }
    double GetDistMag() const { return m_DistMagMP; }
    AEVec2 GetAimVector() const { return m_AimVector; }
    float GetAimAngle() const { return m_AimAngle; }

    f32 GetAttackRange() const { return m_AttackRange; }
    f32 GetConeThreshold() const { return m_ConeThreshold; }
    f32 GetStartAngle() const { return m_StartAngle; }
    f32 GetCurrentAngle() const { return m_CurrentAngle; }

    bool GetBlockStatus() const { return m_BlockActive; }
    bool GetParryStatus() const { return m_ParryActive; }
    int GetAttackCharges() const { return m_AttackCharges; }
    int GetMaxAttackCharge() const { return m_MaxAttackCharge; }
    void SetAttackCharges(int charges) { m_AttackCharges = charges; }
    int   GetDashCharges()       const { return m_DashCharges; }
    int   GetMaxDashCharges()    const { return m_DashChargesMax; }
    float GetDashRechargeTimer() const { return m_DashRechargeTimer; }
    float GetDashRechargeTime()  const { return m_DashRechargeTime; }
    float GetSpeed() const { return m_Speed; }

    Combat::CombatFlags GetCombatFlag() const { return m_CombatFlags; }
    Combat::CombatStats GetCombatStats() const { return m_CombatStats; }
    bool GetIsAlive() const { return m_CombatFlags.isAlive; }

    void DeductHealth(f32 damage) { m_CombatStats.health -= damage; }

    // Setters if you need to teleport the player (e.g. respawning)
    void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }
    void SetAimVector(float x, float y) { m_AimVector.x = x, m_AimVector.y = y; }
    void SetAimAngle(float angle) { m_AimAngle = angle; }
    void SetHDP(f32 dmg) { m_healthDepletionPercentage += dmg; }

    // Speed multiplier for augment effects
    float m_SpeedMultiplier = 1.0f;
    void SetSpeedMultiplier(float mult) { m_SpeedMultiplier = mult; }
    void SetBlockFrames(int startup, int parry, int recovery) {
        b_StartUpFrames = startup;
        b_ParryFrames = parry;
        b_RecoveryFrames = recovery;
        b_TotalFrames = b_StartUpFrames + b_ParryFrames + b_RecoveryFrames;
        m_BlockData = {
            b_StartDegree, b_EndDegree,
            b_StartUpFrames, b_ParryFrames, b_RecoveryFrames, b_TotalFrames, b_Block
        };
    }

    // Call once after the map is loaded so the player can self-resolve wall collisions.
    void SetMap(const MapSystem* map) { m_pMap = map; }

    // Augments
    bool PreventMovement(bool notpreventing) const {
        return notpreventing;
    }

    void EvaluateCurrentDirection();

private:
    float sizeMultiplier{ 2.0f };
    Sprite m_PlayerSprite;
    AEGfxTexture* m_PlayerSpriteSheet;
    AEGfxTexture* m_PlayerCombatSpriteSheet;
    PlayerDirection m_CurrentDirection;

    Combat::System combatSystem;


    // Position & Stats
    float m_PosX{}, m_PosY{};
    float m_Speed{};
    float m_Size{};
    f32 m_healthDepletionPercentage{};
    PlayerState m_CurrentState{};

    // Dash Logic
    int   m_DashCharges{};
    int   m_DashChargesMax{};
    float m_DashRechargeTimer{};
    float m_DashRechargeTime{};

    // Visual Assets
    AEGfxVertexList* m_pMesh = nullptr;
    AEGfxVertexList* m_playerHealthBarMesh{ nullptr };


    // -------------------------- //
    //      COMBAT VARIABLES      //
    // -------------------------- //
    Combat::CombatStats m_CombatStats
    {
        200.0f, // health
        40.0f, // attack
        5.0f, // defense
        0.0f, // crit chance
        0.0f, // crit multiplier
        0.0f, // attack multiplier
        200.0f // max health
    };

    Combat::CombatFlags m_CombatFlags
    {
        true, // isAlive
        false, // attackHit
        false, // blockOn
        false, // parryOn
        false, // blocked
        false, // parried
        false, // stunned
        //true,  // recovered
       true,  // attackResolved
        true,  // parryResolved
       true,  // blockedResolved
       false,  // attackQueued
       false,
       true
    };
    int m_AttackStopFrames{};
    int m_ParryStopFrames{};
    int m_DefendStopFrames{};

    // Attack Logic
    // --------------------
    int m_AttackID{};
    bool  m_AttackActive = false;
    bool  m_AllowAttack = true;

    int m_AttackCharges { DEFAULT_ATTACK_CHARGES };
    int m_MaxAttackCharge { DEFAULT_ATTACK_CHARGES };

    float m_AttackDuration{ 0.15f };

    float m_AttackFrameAccumulator{};
    int m_AttackCurrentFrame{};

    int m_AttackChainIterator{};

    float m_AttackProgress{};

    float m_AttackRange { 200.0f };
    float m_ConeHalfAngleDeg { 30.0f };
    float m_ConeThreshold{};

    float m_StartAngle{};
    float m_EndAngle{};
    float m_CurrentAngle{};

    f32 a_StartDegree{ 30.0f };
    f32 a_EndDegree{ 30.0f };
    bool a_Recovered{ true };
    int a_StartUpFrames{ 7 };
    int a_ActiveFrames{ 10 };
    int a_RecoveryFrames{ 15 };
    int a_TotalFrames{ a_StartUpFrames + a_ActiveFrames + a_RecoveryFrames };
    int a_Damage{ 20 };
    Combat::CombatData::AttackData m_AttackBasic
    {
        a_StartDegree,      // startAngle
        a_EndDegree,        // endAngle
        a_StartUpFrames,    // startUp
        a_ActiveFrames,     // active
        a_RecoveryFrames,   // recovery
        a_TotalFrames,      // total
        a_Damage            // damage
    };
    Combat::CombatData::AttackState m_AttackState
    {
        a_Recovered,        // recovered
    };

    std::vector<Combat::CombatData::AttackData> m_AttackChain
    {
        {45.0f, 45.0f, 7, 15, 15, 37, 20},
        {30.0f, 30.0f, 5, 10, 15, 30, 20},
        {0.0f, 350.0f, 5, 10, 15, 12, 20}
    };

    // Block Logic
    // ---------------------
    bool m_BlockActive = false;
    bool m_AllowBlock = true;
    bool m_ParryActive = false;

    float m_ParryDuration{ 0.5f };
    float m_BlockFrameAccumulator{};
    int m_BlockCurrentFrame{};

    float m_BlockTimer{};

    f32 b_StartDegree{ 30.0f };
    f32 b_EndDegree{ 30.0f };
    bool b_Recovered{ true };
    bool b_Held{ false };
    int b_StartUpFrames{ 1 };
    int b_ParryFrames{ 12 };
    int b_RecoveryFrames{ 15 };
    //int b_ActiveFrames{ 15 };
    int b_TotalFrames{ b_StartUpFrames + b_ParryFrames + b_RecoveryFrames };
    int b_Block{ 10 };
    Combat::CombatData::BlockData m_BlockData
    {
        b_StartDegree,      // startAngle
        b_EndDegree,        // endAngle
        b_StartUpFrames,    // startUp
        b_ParryFrames,      // parry
        b_RecoveryFrames,   // recovery
        b_TotalFrames,      // total
        b_Block             // block 
    };
    Combat::CombatData::BlockState m_BlockState
    {
        b_Held,             // held
        b_Recovered         // recovered
    };

    // Movement Logic
    // Dash Logic
    float m_DashDirX = 0.0f;
    float m_DashDirY = 0.0f;

    float m_DashCooldown{};
    float m_DashCooldown_Default{};

    float m_DashStepX{};
    float m_DashStepY{};

    float m_DashDuration{ 0.15f };

    bool m_AllowDash{ true };
    float m_DashFrameAccumulator{};
    int m_DashCurrentFrame{};

    bool m_DashActive{ false };
    int m_StartFrames{ 0 };
    int m_ActiveFrames{ 4 };
    int m_RecoveryFrames{ 5 };
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

    // Attack Visual
    AEGfxVertexList* m_AttackRangeMesh = nullptr;
    AEGfxVertexList* m_BlockRangeMesh = nullptr;

    AEMtx33 atkScale{}, atkRot{}, atkTrans{}, atkTransform{};
    AEMtx33 blockScale{}, blockRot{}, blockTrans{}, blockTransform{};
    AEMtx33 pointScale{}, pointRot{}, pointTrans{}, pointTransform{};

    // Mouse Aiming
    double m_DistMagMP{};
    AEVec2 m_VectorNormalizedMP{};
    AEVec2 m_AimVector{};
    f32 m_AimAngle{};

    // Non-owning pointer to the active map; set via SetMap().
    const MapSystem* m_pMap = nullptr;
    // Augments
    bool preventing_movement;
};