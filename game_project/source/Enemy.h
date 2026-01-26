#pragma once
#include "Utils.h"
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
    void Update(float dt, Player const &player);
    void Draw();
    void Free();

    void StartAttack();
    bool IsAttacking() const { return e_AttackActive; }

    // Getters allowing Game.cpp to access position for Camera/Collisions
    float GetX() const { return e_PosX; }
    float GetY() const { return e_PosY; }
    float GetSize() const { return e_Size; }

    // Setters if you need to teleport the player (e.g. respawning)
    void SetPosition(float x, float y) { e_PosX = x; e_PosY = y; }

private:
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
    // Attack Logic
    // --------------------
    bool  e_AttackActive = false;
    bool  e_AllowAttack = true;

    float e_AttackDuration = 0.15f;
    float e_AttackTimer = 0.0f;

    float e_AttackRange = 125.0f;
    float e_ConeHalfAngleDeg = 30.0f;
    float e_ConeThreshold;

    float e_StartAngle = 0.0f;
    float e_EndAngle = 0.0f;
    float e_CurrentAngle = 0.0f;

    // Attack Visual
    AEGfxVertexList* e_AttackRangeMesh = nullptr;
    //AEGfxVertexList* triangleMesh = nullptr;
    AEMtx33 atkScale, atkRot, atkTrans, atkTransform;
    //AEMtx33 pointScale, pointRot, pointTrans, pointTransform;


    // Mouse Aiming
    AEVec2 e_AimVector;
    float e_AimAngle;
};