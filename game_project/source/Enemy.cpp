#include "Enemy.h"
#include "Player.h"
#include "MathFunctions.h"
#include <iostream>
#include <math.h> // For sqrt

void Enemy::Init()
{
    // Initialize standard values
    e_PosX = 50.0f;
    e_PosY = 0.0f;
    e_Speed = 300.0f;
    e_Size = 40.0f;
    e_DashCooldown_Default = 0.1f;
    e_DashCooldown = 0.1f;

    // --- Attack setup ---
    //e_ConeThreshold = cos(AEDegToRad(e_ConeHalfAngleDeg));

    AEGfxMeshStart();
    AEGfxTriAdd(
        0.0f, 0.5f, 0xFFFF0000, 0, 0,
        0.0f, -0.5f, 0xFFFF0000, 0, 0,
        -e_AttackRange, 0.5f, 0xFFFF0000, 0, 0);

    AEGfxTriAdd(
        -e_AttackRange, 0.5f, 0xFFFF0000, 0, 0,
        -e_AttackRange, -0.5f, 0xFFFF0000, 0, 0,
        0.0f, -0.5f, 0xFFFF0000, 0, 0);

    e_AttackRangeMesh = AEGfxMeshEnd();

    e_eMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
}

void Enemy::Update(float dt, Player const& player)
{
    // Direction vector / Forward vector
    // Vector X and Y between
    e_AimVector = { e_PosX - player.GetX(), e_PosY - player.GetY() };

    // Distance magnitude between
    f32 distMag_PD = Vectors::magnitude(e_AimVector.x, e_AimVector.y); // Dist between mouse and player
    // Normalize vectors (To get direction)
    AEVec2 normalized_PD = Vectors::normalize(distMag_PD, e_AimVector.x, e_AimVector.y); // Normalized vector between mouse and player

    // Angle towards Player
    e_AimAngle = atan2(e_AimVector.y, e_AimVector.x);

    // Start attack
    if ((e_AttackRange >= distMag_PD) && e_AllowAttack)
    {
        StartAttack();
    }

    // Update attack
    if (e_AttackActive)
    {
        e_AttackTimer += dt;

        // For normalized value between 0.0 - 1.0 range
        // 0.0 = attack start
        // 0.5 = halfway through attack
        // 1.0 = attack complete
        float attackProgress = e_AttackTimer / e_AttackDuration;

        e_CurrentAngle = Vectors::lerp(e_StartAngle, e_EndAngle, attackProgress);

        if (attackProgress >= 1.0f)
        {
            e_AttackActive = false;
            e_AllowAttack = true;
            attackProgress = 1.0f;
        }
    }
}

void Enemy::Draw()
{
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);


    // Calculate isometric squashed height for drawing
    float isoHeight = e_Size * (GRID_H / GRID_W);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    DrawMesh(e_eMesh, e_Size, isoHeight, e_PosX, e_PosY, 0.0f, 44, 145, 57, 255);
    //DrawMesh(e_AttackRangeMesh, 1.0f, 5.0f, e_PosX, e_PosY, e_AimAngle, 44, 145, 57, 255);
    //AEGfxSetTransform(pointTransform.m);
    //AEGfxMeshDraw(triangleMesh, AE_GFX_MDe_TRIANGLES);

    AEMtx33Scale(&atkScale, 1.0f, 5.0f);
    AEMtx33Rot(&atkRot, e_AimAngle);
    AEMtx33Trans(&atkTrans, e_PosX, e_PosY);
    AEMtx33Concat(&atkTransform, &atkRot, &atkScale);
    AEMtx33Concat(&atkTransform, &atkTrans, &atkTransform);
    AEGfxSetTransform(atkTransform.m);
    AEGfxMeshDraw(e_AttackRangeMesh, AE_GFX_MDM_TRIANGLES);

    if (e_AttackActive)
    {
        AEMtx33Rot(&atkRot, e_CurrentAngle);
        AEMtx33Trans(&atkTrans, e_PosX, e_PosY);

        AEMtx33Concat(&atkTransform, &atkRot, &atkScale);
        AEMtx33Concat(&atkTransform, &atkTrans, &atkTransform);

        AEGfxSetTransform(atkTransform.m);
        AEGfxMeshDraw(e_AttackRangeMesh, AE_GFX_MDM_TRIANGLES);
    }
}

void Enemy::Free()
{
    if (e_eMesh)
        AEGfxMeshFree(e_eMesh);
}

void Enemy::StartAttack()
{
    e_AttackActive = true;
    e_AllowAttack = false;
    e_AttackTimer = 0.0f;

    // 60-degree cone
    e_CurrentAngle = e_AimAngle;
    e_StartAngle = e_AimAngle - AEDegToRad(30.0f);
    e_EndAngle = e_AimAngle + AEDegToRad(30.0f);
}