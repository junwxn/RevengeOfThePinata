#pragma once
#include "Utils.h"
#include "Combat.h"
//#include "Player.h"

//-----------------------//
//---- Enemy States ----//
//-----------------------//
enum class EnemyState {
    STATE_IDLE,
    STATE_MOVING,
    STATE_ATTACK,
    STATE_PARRY,
    STATE_DEAD
};

class Player;

class Enemy
{
public:
    void Init();
    void Update(float dt, Combat::System& combat, const Player& playercomb);
    void Draw();
    void Free();

    void StartAttack();
    bool IsAttacking() const { return e_AttackActive; }
    void DamageInfo();

    // Getters allowing Game.cpp to access position for Camera/Collisions
    float GetX() const { return e_PosX; }
    float GetY() const { return e_PosY; }
    float GetSize() const { return e_Size; }

    AEVec2 GetNormalizedVector() const { return e_VectorNormalizedPE; }
    f32 GetDistMag() const { return e_DistMagPE; }
    AEVec2 GetAimVector() const { return e_AimVector; }
    float GetAimAngle() const { return e_AimAngle; }

    f32 GetAttackRange() const { return e_AttackRange; }
    f32 GetConeThreshold() const { return e_ConeThreshold; }

    f32 GetAttackProgress() const { return e_attackProgress; }

    bool CanAttack() const { return e_AllowAttack; }
    Combat::CombatFlags GetCombatFlag() const { return e_CombatFlags; }
    void ResetParryFlag() { e_CombatFlags.parried = false; }

    Combat::CombatStats GetCombatStats() const { return e_CombatStats; }

    // Setters if you need to teleport the player (e.g. respawning)
    void SetPosition(float x, float y) { e_PosX = x; e_PosY = y; }
    void SetAimVector(float x, float y) { e_AimVector.x = x, e_AimVector.y = y; }
    void SetAimAngle(float angle) { e_AimAngle = angle; }

    // Flag Setters
    void SetParried(bool set) { e_CombatFlags.parried = set; }
    
    void MarkAttackResolved() {
        e_CombatFlags.attackResolved = true;
        e_CombatFlags.parryResolved = true;
        e_CombatFlags.blockResolved = true;
    }

private:   
    Combat::System combatSystem;
    // Position & Stats
    float e_PosX, e_PosY;
    float e_Speed;
    float e_Size;

    // Dash Logic
    float e_DashCooldown;
    float e_DashCooldown_Default;

    // Visual Assets
    AEGfxVertexList* e_eMesh = nullptr;

    // -------------------------- //
    //      COMBAT VARIABLES      //
    // -------------------------- //
    Combat::CombatStats e_CombatStats{ 10.0f, 5.0f };
    Combat::CombatFlags e_CombatFlags{ false, false, false, false, false, false, false, false };
     
    // Attack Logic
    // --------------------
    bool  e_AttackActive = false;
    bool  e_AllowAttack = true;
    float e_AttackCooldown = 0.0f;

    float e_AttackDuration = 0.15f;
    float e_AttackTimer = 0.0f;

    float e_AttackRange = 125.0f;
    float e_ConeHalfAngleDeg = 30.0f;
    float e_ConeThreshold;

    float e_StartAngle = 0.0f;
    float e_EndAngle = 0.0f;
    float e_CurrentAngle = 0.0f;

    float e_attackProgress = 0.0f;

    // Damage Logic
    // --------------------

    // Attack Visual
    AEGfxVertexList* e_AttackRangeMesh = nullptr;
    //AEGfxVertexList* triangleMesh = nullptr;
    AEMtx33 atkScale, atkRot, atkTrans, atkTransform;
    //AEMtx33 pointScale, pointRot, pointTrans, pointTransform;


    // Mouse Aiming
    f32 e_DistMagPE;
    AEVec2 e_VectorNormalizedPE;
    AEVec2 e_AimVector;
    float e_AimAngle;
};