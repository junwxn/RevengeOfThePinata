/*************************************************************************
@file		Augments.cpp

@Author		Charles Yap, charles.y@digipen.edu

@Co-authors	Chiu Jun Wen j.chiu@digipen.edu (Help with the Augment values,
            Augment Select and for applying the Augments to the player
            and enemies)

@brief		This file contains the Augment Ball that spawns at the end
            of each round. Also the Sprites used with the Ball, including
            Cards, Card hitboxes and selection, and the physical spawning
            of the Ball itself

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h" // Has to be first file to include in .cpp
#include "Augments.h"
#include "AugmentData.h"
#include "Player.h"
#include <math.h> // For sqrt

// Setting the Augments
void Augments::SetAugmentSet(AugmentSet set) {
    m_currentSet = set;
    switch (set) {
    case AugmentSet::SET_DASH:
        m_cardIDs[0] = AugmentID::SHIELD_DASH;
        m_cardIDs[1] = AugmentID::POISON_TRAIL;
        m_cardIDs[2] = AugmentID::DASH_SPEED_BOOST;
        break;
    case AugmentSet::SET_ATTACK:
        m_cardIDs[0] = AugmentID::CHAIN_ATTACK;
        m_cardIDs[1] = AugmentID::DAMAGING_MARK;
        m_cardIDs[2] = AugmentID::ATTACK_SPEED_BOOST;
        break;
    case AugmentSet::SET_PARRY:
        m_cardIDs[0] = AugmentID::MORE_PARRY_CHARGES;
        m_cardIDs[1] = AugmentID::FASTER_PARRY;
        m_cardIDs[2] = AugmentID::AMPLIFIED_DAMAGE;
        break;
    }
}

// Initializing all values
void Augments::Init() {

    // Augment ball position
    augPosX = 0.f;
    augPosY = 0.f;
    augSize = 50.f;

    // Range at which player can interact w the ball
    interactRange = 100.f;
    playerballdist = 0;

    // The rate at which the ball hovers
    hoverPosY = augPosY;
    hoverTime = 0.f;
    hoverPower = 10.f;
    hoverSpeed = 2.f;
    deltaTime = 0;

    isoHeight = 0;

    // Card width and height
    cardWidth = 400;
    cardHeight = 600;

    // The tint of the window for card selection
    windowTintX = 0;
    windowTintY = 0;

    // Each individual card position
    cards_y = 0;
    cards_x1 = 0;
    cards_x2 = 0;
    cards_x3 = 0;
    distanceY = 0;

    // Booleans for animations
    spawn_anim = false; // The animation for ball spawn
    choose = false; // Is the player choosing cards?
    augmentSelected = false;
    startingAnimation = true;
    cardsInPosition = false;

    // The camera position to draw the cards
    choiceCameraX = 0;
    choiceCameraY = 0;

    // The animation for the beam
    beamStartY = augPosY + 1000.0f; // How high the beam spawns (off screen)
    beamX = 200;
    beamY = beamStartY;
    beamTargetY = augPosY; // Beam endpoint

    // Free any existing resources before creating new ones (prevents leaks on restart)
    if (interactMesh) { AEGfxMeshFree(interactMesh); interactMesh = nullptr; }
    if (augmentMesh) { AEGfxMeshFree(augmentMesh); augmentMesh = nullptr; }
    if (cardMesh)    { AEGfxMeshFree(cardMesh);    cardMesh    = nullptr; }
    if (beamMesh) { AEGfxMeshFree(beamMesh);    beamMesh = nullptr; }
    if (m_cardFont != -1) { AEGfxDestroyFont(m_cardFont); m_cardFont = -1; }
    if (m_candyTex) { AEGfxTextureUnload(m_candyTex); m_candyTex = nullptr; }
    if (m_amplifieddamageTex) { AEGfxTextureUnload(m_amplifieddamageTex); m_amplifieddamageTex = nullptr; }
    if (m_attackmomentumTex) { AEGfxTextureUnload(m_attackmomentumTex); m_attackmomentumTex = nullptr; }
    if (m_chainattackTex) { AEGfxTextureUnload(m_chainattackTex); m_chainattackTex = nullptr; }
    if (m_damagingmarkTex) { AEGfxTextureUnload(m_damagingmarkTex); m_damagingmarkTex = nullptr; }
    if (m_dashmomentumTex) { AEGfxTextureUnload(m_dashmomentumTex); m_dashmomentumTex = nullptr; }
    if (m_parrychargesTex) { AEGfxTextureUnload(m_parrychargesTex); m_parrychargesTex = nullptr; }
    if (m_poisontrailTex) { AEGfxTextureUnload(m_poisontrailTex); m_poisontrailTex = nullptr; }
    if (m_quickparryTex) { AEGfxTextureUnload(m_quickparryTex); m_quickparryTex = nullptr; }
    if (m_shielddashTex) { AEGfxTextureUnload(m_shielddashTex); m_shielddashTex = nullptr; }
    if (interactTex) { AEGfxTextureUnload(interactTex); interactTex = nullptr; }
    if (beamTex) { AEGfxTextureUnload(beamTex); beamTex = nullptr; }

    m_candyTex = AEGfxTextureLoad("Assets/Cards/candy.png");
    interactTex = AEGfxTextureLoad("Assets/Cards/pressXtointeract.png");
    beamTex = AEGfxTextureLoad("Assets/Cards/beam.png");

    // Card textures
    m_amplifieddamageTex = AEGfxTextureLoad("Assets/Cards/amplifieddamage2.png");
    m_attackmomentumTex = AEGfxTextureLoad("Assets/Cards/attackmomentum.png");
    m_chainattackTex = AEGfxTextureLoad("Assets/Cards/chainattack.png");
    m_damagingmarkTex = AEGfxTextureLoad("Assets/Cards/damagingmark.png");
    m_dashmomentumTex = AEGfxTextureLoad("Assets/Cards/dashmomentum2.png");
    m_parrychargesTex = AEGfxTextureLoad("Assets/Cards/parrycharges.png");
    m_poisontrailTex = AEGfxTextureLoad("Assets/Cards/poisontrail2.png");
    m_quickparryTex = AEGfxTextureLoad("Assets/Cards/quickparry.png");
    m_shielddashTex = AEGfxTextureLoad("Assets/Cards/shielddash.png");

    // Meshes for the textures
    augmentMesh = CreateCircleMesh(1, 16, 0x000000);
    candyMesh = CreateSpriteRectMesh(0x000000, 1.0, 1.0);
    interactMesh = CreateSpriteRectMesh(0x000000, 1.0f, 1.0f);
    beamMesh = CreateSpriteRectMesh(0x000000, 1.0f, 1.0f);

    cardMesh = CreateRectMesh(0x000000);
    m_cardFont = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 24);
    
    // Debug for if textures fail to load
    /*if (!m_shielddashTex) {
        std::cout << "Failed to load card texture!" << std::endl;
    }
    else {
        std::cout << "card texture success" << std::endl;
    }*/

    // The U and V for each texture, set like this so it actually draws
    interactSprite.SetTextureU();
    interactSprite.SetTextureV(0);

    candySprite.SetTextureU();
    candySprite.SetTextureV(0);

    cardSprite.SetSingleFrameTexture();

}

void Augments::Update(f32 pX, f32 pY, f32 dt) {

    playerX = pX;
    playerY = pY;
    deltaTime = dt;

    // Start beam animation
    if (startingAnimation) {
        float distance = beamTargetY - beamY;
        beamY += distance * 10.0f * deltaTime; // tweak speed

        // Debug
        //std::cout << "beamTargetY: " << beamTargetY << std::endl;
        //std::cout << "distance: " << distance << std::endl;
        //std::cout << "beamY: " << beamY << std::endl; // mesh draw
    }

    if (beamY < 25.f) {
        // Spawning augment ball
        spawn_anim = true;
    }

    // Augment ball spawn
    if (spawn_anim) {

        beamX -= 500.0f * deltaTime;
        if (beamX <= 0.0f)
        {
            beamX = 0.0f;
            startingAnimation = false;
        }

        float dx = playerX;
        float dy = playerY;

        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);

        float mouseWX = mouseX - AEGfxGetWindowWidth() * 0.5f;
        float mouseWY = AEGfxGetWindowHeight() * 0.5f - mouseY;

        mouseWX += playerX;
        mouseWY += playerY;

        // distance between ball and player
        playerballdist = sqrt(((dx - augPosX) * (dx - augPosX)) + ((dy - (augPosY - 65)) * (dy - (augPosY - 65))));

        // Debug
        //printf("Player x: %f\n", dx);
        //printf("Player y: %f\n", dy);
        //printf("Playerballdist: %f\n", playerballdist);

        // Drawing calculations
        hoverTime += deltaTime * hoverSpeed;

        // Calculate isometric squashed height for drawing
        isoHeight = augSize * (GRID_H / GRID_W);

        if (AEInputCheckTriggered(AEVK_0)) {
            std::cout << "mouseWX: " << mouseWX << std::endl;
            std::cout << "mouseWY: " << mouseWY << std::endl;
        }

        if (playerballdist < interactRange && !choose && !startingAnimation) {
            printf("PRESS X TO INTERACT\n");

            if (AEInputCheckTriggered(AEVK_X)) {
                printf("CHOOSE ONCE\n");
                // setting the cards OG positions
                choiceCameraX = playerX;
                choiceCameraY = playerY;

                std::cout << "choiceCameraX: " << choiceCameraX << std::endl;
                std::cout << "choiceCameraY: " << choiceCameraY << std::endl;

                cards_y = choiceCameraY - 1000;
                cards_x1 = choiceCameraX - 200;
                cards_x2 = choiceCameraX - 200;
                cards_x3 = choiceCameraX - 200;
                choose = true;
            }
        }
        else if (choose) {

            windowTintX = choiceCameraX - 1600;
            windowTintY = choiceCameraY;

            distanceY = choiceCameraY - cards_y;

            float distanceX1 = (choiceCameraX - 700) - cards_x1;
            float distanceX2 = (choiceCameraX + 300) - cards_x2;

            // cardsInPosition prevents picking all 3 at once
            if (cardsInPosition) {

                if (AEInputCheckTriggered(AEVK_LBUTTON)) {
                    if (IsMouseInside(mouseWX, mouseWY, cards_x1 + (cardWidth * 0.5f), cards_y, cardWidth, cardHeight))
                    {
                        std::cout << "Red picked\n";
                        g_Augments.Choose(m_currentSet, m_cardIDs[0]);
                        std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[0]) << std::endl;
                        augmentSelected = true;
                        choose = false;
                    }

                    if (IsMouseInside(mouseWX, mouseWY, cards_x2 + (cardWidth * 0.5f), cards_y, cardWidth, cardHeight))
                    {
                        std::cout << "Blue picked\n";
                        g_Augments.Choose(m_currentSet, m_cardIDs[1]);
                        std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[1]) << std::endl;
                        augmentSelected = true;
                        choose = false;
                    }

                    if (IsMouseInside(mouseWX, mouseWY, cards_x3 + (cardWidth * 0.5f), cards_y, cardWidth, cardHeight))
                    {
                        std::cout << "Green picked\n";
                        g_Augments.Choose(m_currentSet, m_cardIDs[2]);
                        std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[2]) << std::endl;
                        augmentSelected = true;
                        choose = false;
                    }
                }
            }


            // Updates location, draw all at once in the end
            if (fabs(distanceY) > 2.f) {

                //                       v speed at which the card travels
                cards_y += distanceY * 10.0f * deltaTime;
            }
            else {

                if (fabs(distanceX1) > 2.f) {
                    cards_x1 += distanceX1 * 8.0f * deltaTime;
                }

                if (fabs(distanceX2) > 2.f) {
                    cards_x2 += distanceX2 * 8.0f * deltaTime;
                }
            }

            cardsInPosition = fabs(distanceY) <= 2.f
                && fabs((playerX - 700) - cards_x1) <= 2.f
                && fabs((playerX + 300) - cards_x2) <= 2.f;
        }

    }

}

void Augments::Draw(float camX, float camY) {

    if (spawn_anim) {

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);

        // For debug
        /*if (!m_candyTex)
            std::cout << "Failed to load candy texture!" << std::endl;
        else
            std::cout << "Candy texture loaded successfully" << std::endl;*/

        DrawMesh(augmentMesh, (augSize - 20) - sinf(hoverTime) * hoverPower, (isoHeight - 10) - (sinf(hoverTime) * hoverPower), augPosX, (augPosY - 65), 0.0f, 44, 50, 150, 128);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(m_candyTex, 0.0f, 0.0f);

        float hoverOffset = sinf(hoverTime) * hoverPower;

        DrawTexture(
            candySprite,                // sprite object
            0,                          // currentDirection/frame (0 = one frame)
            candyMesh,                  // mesh
            m_candyTex,                 // candy texture
            augSize * 1.75f,            // width of candy
            augSize * 1.75f,            // height of candy
            augPosX,                    // x position
            hoverPosY + hoverOffset,    // y position with hover
            0.0f,                       // rotation (0 = no rotation)
            1.0f                        // size multiplier (1.0 = normal)
        );

        if (playerballdist < interactRange && !choose && !startingAnimation) {
            /*printf("I SUMMON THEE");*/
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxTextureSet(interactTex, 0.0f, 0.0f);

            DrawTexture(
                interactSprite,                  // sprite object
                0,                               // currentDirection/frame (0 = one frame)
                interactMesh,                    // mesh
                interactTex,                     // candy texture
                300,                             // width of candy
                50,                              // height of candy
                augPosX,                         // x position
                hoverPosY - 75 + hoverOffset,    // y position with hover
                0.0f,                            // rotation (0 = no rotation)
                1.0f                             // size multiplier (1.0 = normal)
            );
        }

        if (choose == true) {

            DrawMesh(cardMesh, 3200, 1800, windowTintX, windowTintY, 0.0f, 0, 0, 0, 100); // Tinted Window

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);

            for (int i = 0; i < 3; ++i) {
                float cardX = (i == 0 ? cards_x1 : (i == 1 ? cards_x2 : cards_x3));
                AEGfxTexture* tex = GetTextureForCard(m_cardIDs[i]);

                if (!tex) continue; // Skip if texture failed to load

                DrawTexture(
                    cardSprite,
                    0,
                    cardMesh,
                    tex,
                    cardWidth,
                    cardHeight,
                    cardX,
                    cards_y,
                    0.0f,
                    1.0f
                );
            }


        }


    }

    (void)camX;
    (void)camY;

    // The Beam animation
    if (beamX > 0) {
        startingAnimation = true;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxTextureSet(beamTex, 0.0f, 0.0f);

        DrawTexture(
            beamSprite,
            0,
            beamMesh,
            beamTex,
            beamX,
            1000,
            augPosX,
            augPosY + beamY + 400,
            0.0f,
            1.0f
        );
    }
}

// Free the assets and meshes
void Augments::Free() {
    if (augmentMesh) {
        AEGfxMeshFree(augmentMesh);
        augmentMesh = nullptr;
    }
    if (cardMesh) {
        AEGfxMeshFree(cardMesh);
        cardMesh = nullptr;
    }
    if (candyMesh) {
        AEGfxMeshFree(candyMesh);
        candyMesh = nullptr;
    }
    if (interactMesh) {
        AEGfxMeshFree(interactMesh);
        interactMesh = nullptr;
    }
    if (beamMesh) {
        AEGfxMeshFree(beamMesh);
        beamMesh = nullptr;
    }
    if (m_cardFont != -1) {
        AEGfxDestroyFont(m_cardFont);
        m_cardFont = -1;
    }

    if (m_candyTex) {
        AEGfxTextureUnload(m_candyTex);
        m_candyTex = nullptr;
    }
    if (m_amplifieddamageTex) {
        AEGfxTextureUnload(m_amplifieddamageTex);
        m_amplifieddamageTex = nullptr;
    }
    if (m_attackmomentumTex) {
        AEGfxTextureUnload(m_attackmomentumTex);
        m_attackmomentumTex = nullptr;
    }
    if (m_chainattackTex) {
        AEGfxTextureUnload(m_chainattackTex);
        m_chainattackTex = nullptr;
    }
    if (m_damagingmarkTex) {
        AEGfxTextureUnload(m_damagingmarkTex);
        m_damagingmarkTex = nullptr;
    }
    if (m_dashmomentumTex) {
        AEGfxTextureUnload(m_dashmomentumTex);
        m_dashmomentumTex = nullptr;
    }
    if (m_parrychargesTex) {
        AEGfxTextureUnload(m_parrychargesTex);
        m_parrychargesTex = nullptr;
    }
    if (m_poisontrailTex) {
        AEGfxTextureUnload(m_poisontrailTex);
        m_poisontrailTex = nullptr;
    }
    if (m_quickparryTex) {
        AEGfxTextureUnload(m_quickparryTex);
        m_quickparryTex = nullptr;
    }
    if (m_shielddashTex) {
        AEGfxTextureUnload(m_shielddashTex);
        m_shielddashTex = nullptr;
    }
    if (interactTex) {
        AEGfxTextureUnload(interactTex);
        interactTex = nullptr;
    }
    if (beamTex) {
        AEGfxTextureUnload(beamTex);
        beamTex = nullptr;
    }
}

// Reset on all the initialization values
void Augments::Reset() {
    augPosX = 0.f;
    augPosY = 0.f;
    augSize = 50.f;
    interactRange = 100.f;

    hoverPosY = augPosY;
    hoverTime = 0.f;
    hoverPower = 10.f;
    hoverSpeed = 2.f;
    deltaTime = 0;

    isoHeight = 0;
    cardWidth = 400;
    cardHeight = 600;

    windowTintX = 0;
    windowTintY = 0;

    cards_y = 0;
    cards_x1 = 0;
    cards_x2 = 0;
    cards_x3 = 0;
    distanceY = 0;

    playerballdist = 0;

    spawn_anim = false;
    choose = false;
    augmentSelected = false;
    startingAnimation = true;
}