#pragma once
#include <vector>
#include <memory>

#include "Utils.h" // Access to AE system and Grid constants
#include "Combat.h"
#include "Enemy.h"

//-----------------------//
//---- Player States ----//
//-----------------------//
enum class PlayerState {
    STATE_IDLE,
    STATE_MOVING,
    STATE_ATTACK,
    STATE_BLOCK,
    STATE_PARRY,
    STATE_DEAD
};

class Enemy;

class Player
{
public:
    void Init();
    void Update(float dt, Combat::System& combat, std::vector<std::unique_ptr<Enemy>> const& wave, f32 camX, f32 camY);
    void Draw();
    void Free();

    void StartAttack();
    bool IsAttacking() const { return m_AttackActive; }

    void StartBlock();
    bool IsBlocking() const { return m_BlockActive; }
    void GainAttackCharge() {
        ++m_AttackCharges;
        if (m_AttackCharges > m_MaxAttackCharge) {
            m_AttackCharges = m_MaxAttackCharge;
        }
    }

    void ResetCombatVariables();

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

    bool GetBlockStatus() const { return m_BlockActive; }
    bool GetParryStatus() const { return m_ParryActive; }
    int GetAttackCharges() const { return m_AttackCharges; }

    Combat::CombatFlags GetCombatFlag() const { return m_CombatFlags; }
    Combat::CombatStats GetCombatStats() const { return m_CombatStats; }

    void DeductHealth(f32 damage) { m_CombatStats.health -= damage; }

    // Setters if you need to teleport the player (e.g. respawning)
    void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }
    void SetAimVector(float x, float y) { m_AimVector.x = x, m_AimVector.y = y; }
    void SetAimAngle(float angle) { m_AimAngle = angle; }

private:
    Combat::System combatSystem;

    // Position & Stats
    float m_PosX, m_PosY;
    float m_Speed;
    float m_Size;
    PlayerState m_CurrentState;

    // Dash Logic
    float m_DashCooldown;
    float m_DashCooldown_Default;

    // Visual Assets
    AEGfxVertexList* m_pMesh = nullptr;

    // -------------------------- //
    //      COMBAT VARIABLES      //
    // -------------------------- //
    Combat::CombatStats m_CombatStats{ 200.0f, 10.0f, 5.0f };
    Combat::CombatFlags m_CombatFlags{ false, false, false, false };

    int a_StartUpFrames{ 7 };
    int a_ActiveFrames{ 15 };
    int a_RecoveryFrames{ 15 };
    int a_TotalFrames{ a_StartUpFrames + a_ActiveFrames + a_RecoveryFrames };
    Combat::CombatFrames::AttackFrames m_AttackFrames{ a_StartUpFrames, a_ActiveFrames, a_RecoveryFrames };
    
    int b_StartUpFrames{ 2 };
    int b_ParryFrames{ 7 };
    //int b_ActiveFrames{ 15 };
    int b_TotalFrames{ b_StartUpFrames + b_ParryFrames };
    Combat::CombatFrames::BlockFrames b_BlockFrames{ b_StartUpFrames, b_ParryFrames };

    // Attack Logic
    // --------------------
    bool  m_AttackActive = false;
    bool  m_AllowAttack = true;

    int m_AttackCharges { 3 };
    int m_MaxAttackCharge { 5 };

    float m_AttackDuration{ 0.15f };
    float m_AttackFrameAccumulator{};
    int m_AttackCurrentFrame{};

    float m_AttackRange { 200.0f };
    float m_ConeHalfAngleDeg { 30.0f };
    float m_ConeThreshold{};

    float m_StartAngle{};
    float m_EndAngle{};
    float m_CurrentAngle{};

    // Block Logic
    // ---------------------
    bool m_BlockActive = false;
    bool m_AllowBlock = true;
    bool m_ParryActive = false;

    float m_ParryDuration{ 0.5f };
    float m_BlockFrameAccumulator{};
    int m_BlockCurrentFrame{};

    float m_BlockTimer{};

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
};