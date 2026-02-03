#include "Player.h"
#include "Enemy.h"
#include "Combat.h"
#include "Game.h"
#include "Colors.h"
#include "MathFunctions.h"
#include <math.h> // For sqrt
#include <iostream>

void Player::Init()
{
    // Initialize standard values
    m_PosX = 0.0f;
    m_PosY = 0.0f;
    m_Speed = 300.0f;
    m_Size = 40.0f;
    m_DashCooldown_Default = 0.1f;
    m_DashCooldown = 0.1f;

    m_CurrentState = PlayerState::STATE_IDLE;

    // --- Attack setup ---
    m_ConeThreshold = cos(AEDegToRad(m_ConeHalfAngleDeg));

    m_AttackRangeMesh = CreateLineMesh(m_AttackRange, Colors::bananaYellow);
    m_BlockRangeMesh = CreateLineMesh(m_AttackRange, Colors::red);
    // Create a local mesh for the player
    // (Assuming CreateCircleMesh is defined in Utils.h/cpp)
    m_pMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
}

void Player::Update(float dt, Combat::System& combat, std::vector<std::unique_ptr<Enemy>> const& wave, f32 camX, f32 camY)
{
    // Attack / Combat Logic
    // Mouse input
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float halfWindowW = (float)AEGfxGetWindowWidth() / 2.0f;
    float halfWindowH = (float)AEGfxGetWindowHeight() / 2.0f;

    // Calculate Mouse Offset from Center (in pixels/screen units)
    // Center is (0,0)
    float mouseScreenX = (float)mouseX - halfWindowW;
    float mouseScreenY = halfWindowH - (float)mouseY; // Standard Y-flip for 2D coords

    // Convert to World Position by adding the Camera Position (passed from Game.cpp)
    float mouseWorldX = mouseScreenX + camX;
    float mouseWorldY = mouseScreenY + camY;

    // Vector from Player to Mouse World Position
    m_AimVector = { mouseWorldX - m_PosX, mouseWorldY - m_PosY };
  
    //// Distance magnitude between
    m_DistMagMP = Vectors::magnitude(m_AimVector.x, m_AimVector.y); // Dist between mouse and player
    //// Normalize vectors (To get direction)
    m_VectorNormalizedMP = Vectors::normalize(m_DistMagMP, m_AimVector.x, m_AimVector.y); // Normalized vector between mouse and player

    // Angle
    // Returns the angle at which quadrant the mouse is at
    // based on the vector between the mouse and the player
    //		atan2(y1-y2, x1-x2)
    // Depending on if the difference is positive or negative
    // atan2 can determine the angle of the direction vector
    //									|
    // 									|
    //	  90 - 180 deg = Top left		|		0 - 90 deg = Top right
    // 									|
    // 	-----------------------------------------------------------------
    // 									|
    //  180 - 270 deg = Btm left		|		270 - 360 deg = Btm right
    //									|
    // 									|
    m_AimAngle = atan2(m_AimVector.y, m_AimVector.x);

    // Dot product between AIM and TARGET direction
    // Eg. How wide is the angle?
    //			 /
    //			/  30�
    //-------- > Aim direction(mouse)
    //			\  30�
    //			 \

    //AEVec2 vectorBtw_PD { enemy.GetX() - m_PosX, enemy.GetY() - m_PosY };
    //f32 distMag_PD = Vectors::magnitude(vectorBtw_PD.x, vectorBtw_PD.y);// Dist between dummy and player
    //AEVec2 normalized_PD = Vectors::normalize(Vectors::magnitude(enemy.GetAimVector().x, enemy.GetAimVector().y), enemy.GetAimVector().x, enemy.GetAimVector().y);
    //f32 dotProduct = normalized_MP.x * normalized_PD.x + normalized_MP.y * normalized_PD.y;

    // Convert mouse to world space if needed (camera offset later)

    // Toggle player attack on / off
    if (AEInputCheckTriggered(AEVK_TAB)) {
        m_AllowAttack = !m_AllowAttack;
        //std::cout << "m_AllowAttack: " << m_AllowAttack << std::endl;
    }

    if (m_AttackCharges <= 0) m_AllowAttack = false;
    else m_AllowAttack = true;

    // Start attack
    if (AEInputCheckTriggered(AEVK_LBUTTON) && m_AllowAttack && !m_BlockActive)
    {
        std::cout << "ATTACK" << std::endl;
        m_AllowBlock = false;
        StartAttack();
        for (auto& enemy : wave) {
            if (combatSystem.IsEnemyInRange(*this, *enemy)) {
                std::cout << "ENEMY HIT!" << std::endl;
                m_CombatFlags.attackHit = true;
            }
        }
		std::cout << "Attack Charges Left: " << m_AttackCharges - 1 << std::endl;
    }
    else m_CombatFlags.attackHit = false;

    if (AEInputCheckTriggered(AEVK_RBUTTON) && m_AllowBlock && !m_AttackActive)
    {
        std::cout << "BLOCK" << std::endl;
        m_AllowAttack = false;
        StartBlock();
    }
    if (AEInputCheckReleased(AEVK_RBUTTON)) ResetCombatVariables();

    // Update attack
    if (m_AttackActive)
    {
        m_CurrentState = PlayerState::STATE_ATTACK;
        m_AttackTimer += dt;

        // For normalized value between 0.0 - 1.0 range
        // 0.0 = attack start
        // 0.5 = halfway through attack
        // 1.0 = attack complete
        float attackProgress = m_AttackTimer / m_AttackDuration;

        m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, attackProgress);

        if (attackProgress >= 1.0f)
        {
            /*m_AttackActive = false;
            m_AllowAttack = true;*/
            --m_AttackCharges;
            ResetCombatVariables();
            attackProgress = 1.0f;
        }
    }
    //std::cout << "m_AttackCharges: " << m_AttackCharges << std::endl;

    // Update block
    if (m_BlockActive) {
        m_CurrentState = PlayerState::STATE_PARRY;
        m_ParryActive = true;
        m_CombatFlags.parryOn = true;
        m_BlockTimer += dt;

        float blockProgress = m_BlockTimer / m_ParryDuration;
        if (m_ParryActive) {
            m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, blockProgress);
        }


        if (m_BlockTimer >= m_ParryDuration) {
            m_CurrentState = PlayerState::STATE_BLOCK;
            m_ParryActive = false;
            m_CombatFlags.parryOn = false;
            blockProgress = 1.0f;
        }

        m_CombatFlags.blockOn = true;
        //std::cout << "Player.blocking: " << m_CombatFlags.blockOn << std::endl;
        //std::cout << "Player.parryOn: " << m_CombatFlags.parryOn << std::endl;
        //std::cout << "m_BlockTimer: " << m_BlockTimer << std::endl;
    }
    else {
        m_ParryActive = false;
        m_CombatFlags.blockOn = false;
        m_CombatFlags.parryOn = false;
    }

    //std::cout << "Player.blocking: " << m_CombatFlags.blockOn << std::endl;
    //std::cout << "Player.parryOn: " << m_CombatFlags.parryOn << std::endl;
    //std::cout << "m_BlockTimer: " << m_BlockTimer << std::endl;
    //std::cout << "m_AttackActive: " << m_AttackActive << std::endl;
    //std::cout << "m_BlockActive: " << m_BlockActive << std::endl;

    // --- 1. Update Timers ---
    if (m_DashCooldown > 0.0f)
        m_DashCooldown -= dt;

    // --- 2. Gather Input ---
    s8 moveX = 0;
    s8 moveY = 0;

    if (AEInputCheckCurr(AEVK_W)) moveY += 1;
    if (AEInputCheckCurr(AEVK_S)) moveY -= 1;
    if (AEInputCheckCurr(AEVK_A)) moveX -= 1;
    if (AEInputCheckCurr(AEVK_D)) moveX += 1;

    // --- 3. Execute Movement Logic ---
    if (moveX != 0 || moveY != 0)
    {
        float dirX = 0.0f;
        float dirY = 0.0f;

        // Determine Direction
        if (moveX != 0 && moveY != 0)
        {
            // Diagonal Normalization
            float halfW = GRID_W * 0.5f;
            float halfH = GRID_H * 0.5f;
            float length = sqrt(halfW * halfW + halfH * halfH);

            float isoStepX = halfW / length;
            float isoStepY = halfH / length;

            dirX = (moveX > 0 ? isoStepX : -isoStepX);
            dirY = (moveY > 0 ? isoStepY : -isoStepY);
        }
        else
        {
            // Orthogonal Normalization
            dirX = (float)moveX;
            dirY = (float)moveY;
        }

        // --- 4. Dash Logic ---
        if (AEInputCheckTriggered(AEVK_SPACE) && m_DashCooldown <= 0.0f)
        {
            float blinkDist = 0.0f;

            if (moveX != 0 && moveY != 0)
            {
                // Diagonal: Move Hypotenuse of one tile
                float halfW = GRID_W * 0.5f;
                float halfH = GRID_H * 0.5f;
                blinkDist = sqrt(halfW * halfW + halfH * halfH);
            }
            else
            {
                // Orthogonal: Move Width or Height
                blinkDist = (moveX != 0) ? GRID_W : GRID_H;
            }

            // Apply Dash Position
            m_PosX += dirX * blinkDist;
            m_PosY += dirY * blinkDist;

            m_DashCooldown = m_DashCooldown_Default;
        }

        // --- 5. Apply Velocity ---
        m_PosX += dirX * m_Speed * dt;
        m_PosY += dirY * m_Speed * dt;
    }
}

void Player::Draw()
{
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    // Calculate isometric squashed height for drawing
    float isoHeight = m_Size * (GRID_H / GRID_W);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    // Player Mesh
    DrawMesh(m_pMesh, m_Size, isoHeight, m_PosX, m_PosY, 0.0f, 44, 145, 57, 255);
    
    // Aiming Pointer
    if (!m_AttackActive) {
        DrawMesh(m_AttackRangeMesh, 1.0f, 5.0f, m_PosX, m_PosY, m_AimAngle, 255, 255, 53, 255);
    }

    else if (m_AttackActive)
    {
        DrawMesh(m_AttackRangeMesh, 1.0f, 5.0f, m_PosX, m_PosY, m_CurrentAngle, 255, 255, 53, 255);
    }

    if (m_BlockActive)
    {
        if (m_ParryActive) DrawMesh(m_BlockRangeMesh, 1.0f, 5.0f, m_PosX, m_PosY, m_CurrentAngle, 255, 0, 0, 255);   
    }
}

void Player::Free()
{
    if (m_pMesh)
        AEGfxMeshFree(m_pMesh);
    if (m_AttackRangeMesh)
        AEGfxMeshFree(m_AttackRangeMesh);
    if (m_BlockRangeMesh)
        AEGfxMeshFree(m_BlockRangeMesh);
}

void Player::StartAttack()
{
    m_AttackActive = true;
    m_AllowAttack = false;
    m_AttackTimer = 0.0f;

    // 60-degree cone
    m_CurrentAngle = m_AimAngle;
    m_StartAngle = m_AimAngle - AEDegToRad(30.0f);
    m_EndAngle = m_AimAngle + AEDegToRad(30.0f);
}

void Player::StartBlock()
{
    m_BlockActive = true;
    m_AllowBlock = false;
    m_BlockTimer = 0.0f;

    // 60-degree cone
    m_CurrentAngle = m_AimAngle;
    m_StartAngle = m_AimAngle + AEDegToRad(30.0f);
    m_EndAngle = m_AimAngle - AEDegToRad(30.0f);
}

void Player::ResetCombatVariables()
{
    m_AttackActive = false;
    m_AllowAttack = true;

    m_BlockActive = false;
    m_AllowBlock = true;
}