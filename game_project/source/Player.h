#pragma once
#include "Utils.h" // Access to AE system and Grid constants
//#include "Enemy.h"

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
    void Update(float dt, Enemy const& enemy);
    void Draw();
    void Free();

    void StartAttack();
    bool IsAttacking() const { return m_AttackActive; }

    void StartBlock();
    bool IsBlocking() const { return m_BlockActive; }


    // Getters allowing Game.cpp to access position for Camera/Collisions
    float GetX() const { return m_PosX; }
    float GetY() const { return m_PosY; }
    float GetSize() const { return m_Size; }
    PlayerState GetState() const { return m_CurrentState; }

    // Getters for Combat related purposes
    bool GetBlockStatus() const { return m_BlockActive; }
    bool GetParryStatus() const { return m_ParryActive; }

    // Setters if you need to teleport the player (e.g. respawning)
    void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }

private:
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
    // Attack Logic
    // --------------------
    bool  m_AttackActive = false;
    bool  m_AllowAttack = true;

    float m_AttackDuration = 0.15f;
    float m_AttackTimer = 0.0f;

    float m_AttackRange = 200.0f;
    float m_ConeHalfAngleDeg = 30.0f;
    float m_ConeThreshold;

    float m_StartAngle = 0.0f;
    float m_EndAngle = 0.0f;
    float m_CurrentAngle = 0.0f;

    // Block Logic
    // ---------------------
    bool m_BlockActive = false;
    bool m_AllowBlock = true;
    bool m_ParryActive = false;

    float m_ParryDuration = 0.2f;

    float m_BlockTimer = 0.0f;

    // Attack Visual
    AEGfxVertexList* m_AttackRangeMesh = nullptr;
    AEGfxVertexList* m_BlockRangeMesh = nullptr;

    AEMtx33 atkScale, atkRot, atkTrans, atkTransform;
    AEMtx33 blockScale, blockRot, blockTrans, blockTransform;
    AEMtx33 pointScale, pointRot, pointTrans, pointTransform;

    // Mouse Aiming
    AEVec2 m_AimVector;
    float m_AimAngle;
};