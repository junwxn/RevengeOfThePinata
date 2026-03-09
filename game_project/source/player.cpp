#include "pch.h"

#include "Player.h"
#include "Enemy.h"
#include "Combat.h"
#include "Colors.h"
#include "MathFunctions.h"
#include "Map.h"
#include "EventSystem.h"
#include "AugmentData.h"
#include "Audio.h"

int g_PlayerAttackCharges = 100;

std::ostream& operator<<(std::ostream& os, PlayerState const& ps)
{
    if (ps == PlayerState::STATE_IDLE) return os << "STATE_IDLE";
    else if (ps == PlayerState::STATE_MOVING) return os << "STATE_MOVING";
    else if (ps == PlayerState::STATE_ATTACK) return os << "STATE_ATTACK";
    else if (ps == PlayerState::STATE_BLOCK) return os << "STATE_BLOCK";
    else if (ps == PlayerState::STATE_PARRY) return os << "STATE_PARRY";
    else if (ps == PlayerState::STATE_DEAD) return os << "STATE_DEAD";
    return os << "ERROR";
}

void Player::Init()
{
    // Initialize standard values
    m_PosX = 0.0f;
    m_PosY = 0.0f;
    m_Speed = 300.0f;
    m_Size = PLAYER_SIZE;
    m_DashCooldown_Default = 0.1f;
    m_DashCooldown = 0.1f;

    m_CurrentState = PlayerState::STATE_IDLE;

    // Reset combat state for fresh start
    m_CombatStats.health = m_CombatStats.maxHealth;
    m_CombatFlags.isAlive = true;
    m_healthDepletionPercentage = 0.0f;

    // --- Attack setup ---
    m_ConeThreshold = cos(AEDegToRad(m_ConeHalfAngleDeg));

    // Free any existing meshes before creating new ones (prevents leaks on restart)
    if (m_AttackRangeMesh) { AEGfxMeshFree(m_AttackRangeMesh); m_AttackRangeMesh = nullptr; }
    if (m_BlockRangeMesh)  { AEGfxMeshFree(m_BlockRangeMesh);  m_BlockRangeMesh  = nullptr; }
    if (m_pMesh)           { AEGfxMeshFree(m_pMesh);           m_pMesh           = nullptr; }
    if (m_playerHealthBarMesh) { AEGfxMeshFree(m_playerHealthBarMesh); m_playerHealthBarMesh = nullptr; }

    m_AttackRangeMesh = CreateLineMesh(m_AttackRange, Colors::bananaYellow);
    m_BlockRangeMesh = CreateLineMesh(m_AttackRange, Colors::red);
    m_pMesh = CreateCircleMesh(1.0f, 32, 0x50A655);
    m_playerHealthBarMesh = CreateRectMesh(0xFF0000);

    m_PlayerSprite.Sprite_Init();
    m_PlayerSpriteSheet = m_PlayerSprite.GetPlayerSpriteSheet();
    m_PlayerCombatSpriteSheet = m_PlayerSprite.GetPlayerCombatSpriteSheet();
}

void Player::Update(float dt, Combat::System& combat, std::vector<std::unique_ptr<Enemy>> const& wave, f32 camX, f32 camY, bool preventing_movement)
{

    EvaluateCurrentDirection();
    m_PlayerSprite.Sprite_Update(dt);

    if (m_CombatStats.health <= 0) m_CombatFlags.isAlive = false;
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

    //std::cout << m_CurrentState << std::endl;


    if (AEInputCheckTriggered(AEVK_LALT)) GainAttackCharge();
    // Toggle player attack on / off
    if (AEInputCheckTriggered(AEVK_TAB)) {
        m_AllowAttack = !m_AllowAttack;
        //std::cout << "m_AllowAttack: " << m_AllowAttack << std::endl;
    }

    if (m_AttackCharges <= 0 || !m_AttackState.recovered) m_AllowAttack = false;
    else m_AllowAttack = true;

    // Start attack
    if ((AEInputCheckTriggered(AEVK_LBUTTON) && m_AllowAttack && !m_BlockActive) || (m_CombatFlags.attackQueued && g_Augments.Has(AugmentID::CHAIN_ATTACK)))
    {
        std::cout << "ATTACK" << std::endl;
        m_AllowBlock = false;
        if (m_CombatFlags.attackQueued && g_Augments.Has(AugmentID::CHAIN_ATTACK)) StartAttack(m_AttackChain[m_AttackChainIterator], wave);
        else StartAttack(m_AttackBasic, wave);
        gAudio.PlayCombatSFX(COMBAT_SWING);

        //for (auto& enemy : wave) {
        //    if (combatSystem.IsEnemyInRange(*this, *enemy)) {
        //        std::cout << "ENEMY HIT!" << std::endl;
        //        m_CombatFlags.attackHit = true;
        //        m_AttackStopFrames = combatSystem.GetAttackerStopFrames();
        //    }
        //}
        std::cout << "Attack Charges Left: " << m_AttackCharges - 1 << std::endl;
    }
    //else m_CombatFlags.attackHit = false;

    if (AEInputCheckTriggered(AEVK_RBUTTON) && m_AllowBlock && !m_AttackActive)
    {
        std::cout << "BLOCK" << std::endl;
        m_AllowAttack = false;
        m_BlockState.held = true;
        StartBlock(m_BlockData, wave);
    }
    if (AEInputCheckReleased(AEVK_RBUTTON))
    {
        std::cout << "RELEASED" << std::endl;
        m_BlockState.held = false;

    }
    if (m_BlockActive && m_BlockState.recovered)
    {
        std::cout << "RESETTING" << std::endl;
        ResetCombatVariables();
    }
    //std::cout << "m_BlockData.held: " << m_BlockState.held << std::endl;
    //std::cout << "m_BlockData.recovered: " << m_BlockState.recovered << std::endl;

    //// Update attack
    //if (m_AttackActive)
    //{
    //    m_CurrentState = PlayerState::STATE_ATTACK;
    //    m_AttackFrameAccumulator += dt;

    //    while (m_AttackFrameAccumulator >= combatSystem.GetOneFPS() && m_AttackCurrentFrame <= m_AttackFrames.total) {
    //        ++m_AttackCurrentFrame;
    //        m_AttackFrameAccumulator -= combatSystem.GetOneFPS();
    //    }

    //    // For normalized value between 0.0 - 1.0 range
    //    // 0.0 = attack start
    //    // 0.5 = halfway through attack
    //    // 1.0 = attack complete
    //    
    //    /*float attackProgress{};*/
    //    std::cout << "m_AttackProgress: " << m_AttackProgress << std::endl;

    //    if (m_AttackCurrentFrame < m_AttackFrames.startUp)
    //    {
    //        // Start-up Phase
    //        std::cout << "START UP" << std::endl;
    //    }
    //    else if (m_AttackCurrentFrame < m_AttackFrames.startUp + m_AttackFrames.active)
    //    {
    //        // Active Phase
    //        int activeFrameIndex{ m_AttackCurrentFrame - m_AttackFrames.startUp }; // Gives the current active frame
    //        m_AttackProgress = float(activeFrameIndex) / (m_AttackFrames.active - 1);
    //        m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, m_AttackProgress);
    //    }
    //    else if (m_AttackCurrentFrame < m_AttackFrames.total)
    //    {
    //        // Recovery Phase
    //        std::cout << "RECOVERING" << std::endl;
    //    }
    //    else m_CombatFlags.recovered = true;

    //    //std::cout << m_AttackCurrentFrame << std::endl;
    //    //std::cout << "RECOVERED" << std::endl;
    //    //std::cout << m_CombatFlags.recovered << std::endl;

    //    //std::cout << "m_AttackCurrentFrame: " << m_AttackCurrentFrame << std::endl;
    //    if (m_AttackProgress >= 1.0f) 
    //    {
    //        m_AttackProgress = 1.0f;
    //        if (m_CombatFlags.recovered)
    //        {
    //            --m_AttackCharges;
    //            ResetCombatVariables();
    //        }
    //    }
    //}
    //std::cout << "m_AttackCharges: " << m_AttackCharges << std::endl;

    // CHAIN ATTACK TEST
    if (m_AttackActive)
    {
        if (m_AttackStopFrames > 0)
        {
            m_AttackStopFrames -= dt;
            if (g_Augments.Has(AugmentID::CHAIN_ATTACK)
                && AEInputCheckTriggered(AEVK_LBUTTON)
                && m_AttackChainIterator < m_AttackChain.size() - 1
                && m_AttackCharges > 1)
            {
                m_CombatFlags.attackQueued = true;
                --m_AttackCharges;
                m_AttackChainIterator++;
            }
        }
        //std::cout << "m_AttackChainIterator: " << m_AttackChainIterator << std::endl;
        //std::cout << "m_AttackID: " << m_AttackID << std::endl;

        m_CurrentState = PlayerState::STATE_ATTACK;

        if (m_AttackStopFrames <= 0) m_AttackFrameAccumulator += dt;

        while (m_AttackFrameAccumulator >= combatSystem.GetOneFPS() && m_AttackCurrentFrame <= m_AttackBasic.total) {
            ++m_AttackCurrentFrame;
            m_AttackFrameAccumulator -= combatSystem.GetOneFPS();
        }

        // For normalized value between 0.0 - 1.0 range
        // 0.0 = attack start
        // 0.5 = halfway through attack
        // 1.0 = attack complete

        if (m_AttackCurrentFrame < m_AttackBasic.startUp)
        {
            // Start-up Phase
            //std::cout << "START UP" << std::endl;
        }
        else if (m_AttackCurrentFrame < m_AttackBasic.startUp + m_AttackBasic.active)
        {
            // Active Phase
            int activeFrameIndex{ m_AttackCurrentFrame - m_AttackBasic.startUp }; // Gives the current active frame
            m_AttackProgress = float(activeFrameIndex) / (m_AttackBasic.active - 1);
            m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, m_AttackProgress);

            ///////////
            ///////////
            ///////////
            for (auto& enemy : wave)
            {
                if (!enemy->GetCombatFlag().gotHit
                    && enemy->GetLastAttackID() != m_AttackID
                    && combatSystem.IsEnemyInRange(*this, *enemy))
                {
                    enemy->SetGotHit(true);
                    enemy->SetLastAttackID(m_AttackID);

                    m_CombatFlags.attackHit = true;
                    m_AttackStopFrames = combatSystem.GetAttackerStopFrames();
                    combatSystem.ApplyDamage(*enemy, *this);

                    // Fire ON_ATTACK_HIT event for augment effects
                    EventData hitData;
                    hitData.playerX = m_PosX;
                    hitData.playerY = m_PosY;
                    hitData.damage = Combat::ComputeDamage(*this, *enemy);
                    hitData.targetEnemy = enemy.get();
                    g_Events.Fire(GameEvent::ON_ATTACK_HIT, hitData);
                    gAudio.PlayCombatSFX(COMBAT_HIT);
                }
            }
            m_CombatFlags.attackHit = false;
        }
        else if (m_AttackCurrentFrame < m_AttackBasic.total)
        {
            // Recovery Phase
            if (g_Augments.Has(AugmentID::CHAIN_ATTACK)
                && AEInputCheckTriggered(AEVK_LBUTTON)
                && m_AttackChainIterator < m_AttackChain.size() - 1
                && m_AttackCharges > 1)
            {
                m_CombatFlags.attackQueued = true;
                --m_AttackCharges;
                m_AttackChainIterator++;
            }
        }
        else m_AttackState.recovered = true;

        //std::cout << m_AttackCurrentFrame << std::endl;
        //std::cout << "RECOVERED" << std::endl;
        //std::cout << m_CombatFlags.recovered << std::endl;

        //std::cout << "m_AttackCurrentFrame: " << m_AttackCurrentFrame << std::endl;
        if (m_AttackCurrentFrame >= m_AttackBasic.total)
        {
            m_AttackProgress = 1.0f;
            if (m_AttackState.recovered)
            {
                --m_AttackCharges;
                //std::cout << "REACHED" << std::endl;
                ResetCombatVariables();
                //if (m_CombatFlags.attackQueued) StartAttack(m_AttackChain[m_AttackChainIterator]);
            }
        }

        //std::cout << "attackQueued: " << m_CombatFlags.attackQueued << std::endl;

    }

    // Update block
    if (m_BlockActive) {

        m_CurrentState = PlayerState::STATE_BLOCK;

        if (m_ParryStopFrames > 0) m_ParryStopFrames -= dt;

        if (m_ParryStopFrames <= 0) m_BlockFrameAccumulator += dt;

        if (m_BlockCurrentFrame < m_BlockData.startUp + m_BlockData.parry)
        {
            //std::cout << "WHILE 1" << std::endl;
            while (m_BlockFrameAccumulator >= combatSystem.GetOneFPS())
            {
                ++m_BlockCurrentFrame;
                m_BlockFrameAccumulator -= combatSystem.GetOneFPS();
            }
        }
        else/* if (m_BlockCurrentFrame < m_BlockData.total)*/
        {
            //std::cout << "WHILE 2" << std::endl;
            //if (!m_BlockState.held)
            //{
            //    std::cout << "NOT HOLDING" << std::endl;

            while (m_BlockFrameAccumulator >= combatSystem.GetOneFPS())
            {
                //std::cout << "RESTARTED" << std::endl;
                if (!m_BlockState.held) ++m_BlockCurrentFrame;
                m_BlockFrameAccumulator -= combatSystem.GetOneFPS();
            }
            //}
        }

        float blockProgress{};

        if (m_BlockCurrentFrame < m_BlockData.startUp)
        {

        }
        else if (m_BlockCurrentFrame < m_BlockData.startUp + m_BlockData.parry) {
            m_CurrentState = PlayerState::STATE_PARRY;
            m_ParryActive = true;
            m_CombatFlags.parryOn = true;

            for (auto& enemy : wave)
            {
                if (enemy->GetCombatFlag().parried) m_ParryStopFrames = combatSystem.GetParryStopFrames();

            }

            blockProgress = float(m_BlockCurrentFrame - m_BlockData.startUp) / (m_BlockData.parry - 1);

            m_CurrentAngle = Vectors::lerp(m_StartAngle, m_EndAngle, blockProgress);
            //std::cout << "blockProgress: " << blockProgress << std::endl;
        }
        else if (m_BlockCurrentFrame < m_BlockData.total)
        {
            // Recovery Phase
            blockProgress = 1.0f;
            //std::cout << "RECOVERING" << std::endl;
            m_CurrentState = PlayerState::STATE_IDLE;
        }
        else m_BlockState.recovered = true;


        if (m_BlockState.held)
        {
            blockProgress = 1.0f;
            if (!m_CombatFlags.parryOn) m_ParryActive = false;
            m_CombatFlags.parryOn = false;
        }
    }
    else {
        m_ParryActive = false;
        m_CombatFlags.blockOn = false;
        m_CombatFlags.parryOn = false;
    }


    // --- 1. Update Timers ---
    if (m_DashCooldown > 0.0f)
        m_DashCooldown -= dt;

    // --- 2. Gather Input ---
    s8 moveX = 0;
    s8 moveY = 0;

    if (m_AttackStopFrames <= 0)
    {

        if (!preventing_movement) {

            if (AEInputCheckCurr(AEVK_W)) moveY += 1;
            if (AEInputCheckCurr(AEVK_S)) moveY -= 1;
            if (AEInputCheckCurr(AEVK_A)) moveX -= 1;
            if (AEInputCheckCurr(AEVK_D)) moveX += 1;

        }

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

            // --- 5. Apply Velocity with isometric wall-sliding collision ---
            if (!m_DashActive)
            {
                float velX = dirX * m_Speed * m_SpeedMultiplier * dt;
                float velY = dirY * m_Speed * m_SpeedMultiplier * dt;
                if (m_pMap) {
                    ResolveCollision(m_PosX, m_PosY, velX, velY, m_Size, *m_pMap);
                }
                else {
                    m_PosX += velX;
                    m_PosY += velY;
                }
            }

            std::cout << "DASH: " << m_DashActive << std::endl;
            // --- 4. Dash Logic ---
            if (AEInputCheckTriggered(AEVK_SPACE) && m_DashCooldown <= 0.0f)
            {
                //// Store pre-dash position for poison trail
                //float preDashX = m_PosX;
                //float preDashY = m_PosY;

                //float blinkDist = 0.0f;

                //if (moveX != 0 && moveY != 0)
                //{
                //    // Diagonal: Move Hypotenuse of one tile
                //    float halfW = GRID_W * 0.5f;
                //    float halfH = GRID_H * 0.5f;
                //    blinkDist = sqrt(halfW * halfW + halfH * halfH);
                //}
                //else
                //{
                //    // Orthogonal: Move Width or Height
                //    blinkDist = (moveX != 0) ? GRID_W : GRID_H;
                //}

                //// Dash uses the same collision resolution as walking, stepped in
                //// small increments along the path so the player can't tunnel
                //// through thin walls.  If a step is blocked the player stops at
                //// the last clear position (wall-sliding still applies per step).
                //float dashVelX = dirX * blinkDist;
                //float dashVelY = dirY * blinkDist;
                //if (m_pMap) {
                //    const int steps = (int)(blinkDist / 16.0f) + 1;
                //    float stepVelX = dashVelX / steps;
                //    float stepVelY = dashVelY / steps;
                //    for (int i = 0; i < steps; ++i) 
                //    {
                //        float prevX = m_PosX, prevY = m_PosY;
                //        ResolveCollision(m_PosX, m_PosY, stepVelX, stepVelY, m_Size, *m_pMap);
                //        // If this step made no progress, stop early.
                //        if (m_PosX == prevX && m_PosY == prevY) break;
                //    }
                //}
                //else {
                //    m_PosX += dashVelX;
                //    m_PosY += dashVelY;
                //}

                //m_DashCooldown = m_DashCooldown_Default;

                //// Fire ON_DASH event for augment effects
                //EventData dashData;
                //dashData.playerX = m_PosX;
                //dashData.playerY = m_PosY;
                //dashData.dirX = dirX;
                //dashData.dirY = dirY;
                //g_Events.Fire(GameEvent::ON_DASH, dashData);

                //// --- 5. Apply Velocity with isometric wall-sliding collision ---
                //float velX = dirX * m_Speed * m_SpeedMultiplier * dt;
                //float velY = dirY * m_Speed * m_SpeedMultiplier * dt;
                //if (m_pMap) {
                //    ResolveCollision(m_PosX, m_PosY, velX, velY, m_Size, *m_pMap);
                //}
                //else {
                //    m_PosX += velX;
                //    m_PosY += velY;
                //}
                StartDash(moveX, moveY, dirX, dirY);
            }
        }

        // Update dash
        if (m_DashActive)
        {
            m_CurrentState = PlayerState::STATE_DASH;
            m_DashFrameAccumulator += dt;

            while (m_DashFrameAccumulator >= combatSystem.GetOneFPS() && m_DashCurrentFrame <= m_MovementData.total) {
                ++m_DashCurrentFrame;

                if (m_DashCurrentFrame > m_MovementData.total)
                    m_DashCurrentFrame = m_MovementData.total;

                m_DashFrameAccumulator -= combatSystem.GetOneFPS();

                if (m_DashCurrentFrame >= m_MovementData.startUp
                    && m_DashCurrentFrame < m_MovementData.startUp + m_MovementData.active)
                {
                    ApplyDashStep();
                }
            }
            if (!m_CombatFlags.dashResolved && m_DashCurrentFrame > m_MovementData.startUp + m_MovementData.active)
            {
                EventData dashData;
                dashData.playerX = m_PosX;
                dashData.playerY = m_PosY;
                dashData.dirX = m_DashDirX;
                dashData.dirY = m_DashDirY;

                g_Events.Fire(GameEvent::ON_DASH, dashData);
                m_CombatFlags.dashResolved = true;
            }
            if (m_DashCurrentFrame >= m_MovementData.total)
            {
                m_DashActive = false;
                m_DashCurrentFrame = 0;
                m_DashFrameAccumulator = 0.0f;
                m_DashCooldown = m_DashCooldown_Default;

                ResetCombatVariables();
            }

            std::cout << "DASH OVER" << std::endl;
        }
    }
}

void Player::Draw()
{
    f32 dt = (f32)AEFrameRateControllerGetFrameTime();

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

    // Player health bar
    f32 barWidth = m_Size * 2.0f * m_CombatStats.health / m_CombatStats.maxHealth;
    f32 barHeight = m_Size / 3.0f;
    f32 dbarWidth = m_Size * 2.0f * (m_CombatStats.health / m_CombatStats.maxHealth + m_healthDepletionPercentage / 100.0f);
    f32 dRate = 100.0f * dt;
    if (m_healthDepletionPercentage >= 0.0f) { m_healthDepletionPercentage -= dRate; };

    if (m_CombatStats.health >= 0) {
        DrawMesh(m_playerHealthBarMesh, dbarWidth, barHeight, m_PosX - m_Size, m_PosY + m_Size + barHeight / 2.0f + 5.0f, 0.0f, 255, 0, 0, 255); // Depleting bar
        DrawMesh(m_playerHealthBarMesh, barWidth, barHeight, m_PosX - m_Size, m_PosY + m_Size + barHeight / 2.0f + 5.0f, 0.0f, 80, 200, 120, 255); // Instant bar
    }

    if (!m_AttackActive)
    {
        DrawTexturePlayer(m_PlayerSprite, static_cast<int>(m_CurrentDirection), m_PlayerSprite.GetPlayerSpriteMesh(), m_PlayerSprite.GetPlayerSpriteSheet(), m_PlayerSprite.GetPixelScale(),
            m_PlayerSprite.GetPixelScale(), m_PosX, m_PosY, 0.0f);
    }
    else
    {
        DrawTexturePlayer(m_PlayerSprite, static_cast<int>(m_CurrentDirection), m_PlayerSprite.GetPlayerCombatSpriteMesh(), m_PlayerSprite.GetPlayerCombatSpriteSheet(), m_PlayerSprite.GetPixelScale(),
            m_PlayerSprite.GetPixelScale(), m_PosX, m_PosY, 0.0f);
    }
}

void Player::Free()
{
    if (m_pMesh) {
        AEGfxMeshFree(m_pMesh);
        m_pMesh = nullptr;
    }
    if (m_AttackRangeMesh) {
        AEGfxMeshFree(m_AttackRangeMesh);
        m_AttackRangeMesh = nullptr;
    }
    if (m_BlockRangeMesh) {
        AEGfxMeshFree(m_BlockRangeMesh);
        m_BlockRangeMesh = nullptr;
    }
    if (m_playerHealthBarMesh) {
        AEGfxMeshFree(m_playerHealthBarMesh);
        m_playerHealthBarMesh = nullptr;
    }
}

void Player::StartAttack(Combat::CombatData::AttackData& attackData, std::vector<std::unique_ptr<Enemy>> const& wave)
{
    std::cout << "NEW ATTACK STARTING" << std::endl;
    ++m_AttackID;
    m_AttackActive = true;
    m_AllowAttack = false;

    m_CombatFlags.attackHit = false;

    m_AttackState.recovered = false;
    m_AttackFrameAccumulator = 0.0f;
    m_AttackCurrentFrame = 0;
    m_AttackProgress = 0.0f;
    
    a_TotalFrames = a_StartUpFrames + a_ActiveFrames + a_RecoveryFrames;

    // 60-degree cone
    m_CurrentAngle = m_AimAngle;
    if (!m_CombatFlags.attackQueued) 
    {
        m_StartAngle = m_AimAngle - AEDegToRad(attackData.startAngle);
        m_EndAngle = m_AimAngle + AEDegToRad(attackData.endAngle);
    }
    else
    {
        m_StartAngle = m_AimAngle + AEDegToRad(attackData.startAngle);
        m_EndAngle = m_AimAngle - AEDegToRad(attackData.endAngle);
    }

    for (auto& enemy : wave) enemy->ResetGotHitFlag();

    m_CombatFlags.attackQueued = false;
}

void Player::StartBlock(Combat::CombatData::BlockData& blockData, std::vector<std::unique_ptr<Enemy>> const& wave)
{
    m_BlockActive = true;
    m_AllowBlock = false;
    m_BlockState.recovered = false;
    m_BlockState.held = true;
    m_BlockFrameAccumulator = 0.0f;
    m_BlockCurrentFrame = 0;

    std::cout << "blockData.recovered: " << m_BlockState.recovered << std::endl;
    std::cout << "blockData.held: " << m_BlockState.held << std::endl;

    // 60-degree cone
    m_CurrentAngle = m_AimAngle;
    m_StartAngle = m_AimAngle + AEDegToRad(blockData.startAngle);
    m_EndAngle = m_AimAngle - AEDegToRad(blockData.endAngle);
}

void Player::StartDash(float moveX, float moveY, float dirX, float dirY)
{
    m_DashActive = true;
    m_AllowDash = false;
    m_MovementState.recovered = false;
    m_DashFrameAccumulator = 0.0f;
    m_DashCurrentFrame = 0;

    m_DashDirX = dirX;
    m_DashDirY = dirY;

    // Store pre-dash position for poison trail
    float preDashX = m_PosX;
    float preDashY = m_PosY;

    float blinkDist = 0.0f;

    if (moveX != 0 && moveY != 0)
    {
        // Diagonal: Move Hypotenuse of one tile
        float halfW = GRID_W * 0.5f;
        float halfH = GRID_H * 0.5f;
        //blinkDist = sqrt(halfW * halfW + halfH * halfH);
        
        blinkDist = sqrt(halfW * halfW + halfH * halfH) * 1.75f;
    }
    else
    {
        // Orthogonal: Move Width or Height
        //blinkDist = (moveX != 0) ? GRID_W : GRID_H;

        // Extra dash length test
        blinkDist = (moveX != 0) ? GRID_W * 1.75f : GRID_H * 1.75f;
    }

    // Dash uses the same collision resolution as walking, stepped in
    // small increments along the path so the player can't tunnel
    // through thin walls.  If a step is blocked the player stops at
    // the last clear position (wall-sliding still applies per step).

    // Increase multiplier for smoother dash
    float dashSpeedMultiplier = 1.8f;

    float dashVelX = dirX * blinkDist * dashSpeedMultiplier;
    float dashVelY = dirY * blinkDist * dashSpeedMultiplier;

    // Divide steps by active frames
    m_DashStepX = dashVelX / m_MovementData.active;
    m_DashStepY = dashVelY / m_MovementData.active;
}

void Player::ApplyDashStep()
{
    float dashVelX = m_DashStepX;
    float dashVelY = m_DashStepY;

    if (m_pMap)
    {
        const int steps = 2;

        float stepVelX = m_DashStepX / steps;
        float stepVelY = m_DashStepY / steps;

        for (int i = 0; i < steps; ++i)
        {
            float prevX = m_PosX;
            float prevY = m_PosY;

            ResolveCollision(m_PosX, m_PosY, stepVelX, stepVelY, m_Size, *m_pMap);

            if (m_PosX == prevX && m_PosY == prevY)
                break;
        }
    }
    else
    {
        m_PosX += dashVelX;
        m_PosY += dashVelY;
    }
}

void Player::ResetCombatVariables()
{
    std::cout << "RESETTING" << std::endl;
    m_AttackActive = false;
    m_AllowAttack = true;

    m_AttackProgress = 0.0f;

    m_BlockActive = false;
    m_AllowBlock = true;

    m_DashActive = false;
    m_AllowDash = true;

    if (m_CombatFlags.attackQueued)
    {

    }
    else if(!m_CombatFlags.attackQueued || m_AttackChainIterator >= m_AttackChain.size())
    {
        m_AttackChainIterator = 0;
    }
}

void Player::EvaluateCurrentDirection()
{
    float angleDegrees = AERadToDeg(m_AimAngle);

    // rotate for isometric grid
    //angleDegrees += 45.0f;

    // normalize angle to 0 - 360
    if (angleDegrees < 0)
        angleDegrees += 360.0f;

    if (angleDegrees >= 360)
        angleDegrees -= 360.0f;

    if (angleDegrees >= 0 && angleDegrees < 45)
        m_CurrentDirection = PlayerDirection::DIRECTION_RIGHT;
    else if (angleDegrees >= 45 && angleDegrees < 90)
        m_CurrentDirection = PlayerDirection::DIRECTION_UP_RIGHT;
    else if (angleDegrees >= 90 && angleDegrees < 135)
        m_CurrentDirection = PlayerDirection::DIRECTION_UP;
    else if (angleDegrees >= 135 && angleDegrees < 180)
        m_CurrentDirection = PlayerDirection::DIRECTION_UP_LEFT;
    else if (angleDegrees >= 180 && angleDegrees < 225)
        m_CurrentDirection = PlayerDirection::DIRECTION_LEFT;
    else if (angleDegrees >= 225 && angleDegrees < 270)
        m_CurrentDirection = PlayerDirection::DIRECTION_DOWN_LEFT;
    else if (angleDegrees >= 270 && angleDegrees < 315)
        m_CurrentDirection = PlayerDirection::DIRECTION_DOWN;
    else
        m_CurrentDirection = PlayerDirection::DIRECTION_DOWN_RIGHT;
}