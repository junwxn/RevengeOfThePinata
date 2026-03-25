#include "pch.h" // Has to be first file to include in .cpp
#include "Augments.h"
#include "AugmentData.h"
#include "Player.h"
#include <math.h> // For sqrt

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

void Augments::Init() {
    augPosX = 0.f;
    augPosY = 0.f;
    augSize = 50.f;
    interactRange = 100.f;

    hoverPosY = augPosY;
    hoverTime = 0.f;
    hoverPower = 10.f;
    hoverSpeed = 2.f;
    deltaTime = 0;

    // initializing these card positions
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

    choose = false; // is choosing cards?
    augmentSelected = false;
    startingAnimation = true;
    cardsInPosition = false;

    choiceCameraX = 0;
    choiceCameraY = 0;

    // Free any existing resources before creating new ones (prevents leaks on restart)
    if (augmentMesh) { AEGfxMeshFree(augmentMesh); augmentMesh = nullptr; }
    if (cardMesh)    { AEGfxMeshFree(cardMesh);    cardMesh    = nullptr; }
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

    m_candyTex = AEGfxTextureLoad("Assets/Cards/candy.png");

    m_amplifieddamageTex = AEGfxTextureLoad("Assets/Cards/amplifieddamage2.png"); // 2 versions
    m_attackmomentumTex = AEGfxTextureLoad("Assets/Cards/attackmomentum.png");
    m_chainattackTex = AEGfxTextureLoad("Assets/Cards/chainattack.png");
    m_damagingmarkTex = AEGfxTextureLoad("Assets/Cards/damagingmark.png");
    m_dashmomentumTex = AEGfxTextureLoad("Assets/Cards/dashmomentum2.png");
    m_parrychargesTex = AEGfxTextureLoad("Assets/Cards/parrycharges.png");
    m_poisontrailTex = AEGfxTextureLoad("Assets/Cards/poisontrail2.png");
    m_quickparryTex = AEGfxTextureLoad("Assets/Cards/quickparry.png");
    m_shielddashTex = AEGfxTextureLoad("Assets/Cards/shielddash.png");

    augmentMesh = CreateCircleMesh(1, 16, 0x000000);
    candyMesh = CreateSpriteRectMesh(0x000000, 1.0, 1.0);

    cardMesh = CreateRectMesh(0x000000);
    m_cardFont = AEGfxCreateFont("Assets/fonts/Stick-Regular.ttf", 24);
    
    if (!m_shielddashTex) {
        std::cout << "Failed to load card texture!" << std::endl;

    }
    else {
        std::cout << "card texture success" << std::endl;
    }
    candySprite.SetTextureU();
    candySprite.SetTextureV(0);

    cardSprite.SetSingleFrameTexture();

}

void Augments::Update(f32 playerX, f32 playerY, f32 dt) {

    float dx = playerX;
    float dy = playerY;
    deltaTime = dt;

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWX = mouseX - AEGfxGetWindowWidth() * 0.5f;
    float mouseWY = AEGfxGetWindowHeight() * 0.5f - mouseY;

    mouseWX += playerX;
    mouseWY += playerY;

    // distance between ball and player
    float playerballdist = sqrt(((dx - augPosX) * (dx - augPosX)) + ((dy - (augPosY - 65)) * (dy - (augPosY - 65))));

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

    if (playerballdist < interactRange && !choose) {
        //printf("PRESS X TO INTERACT\n");

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
        
        //printf("Choosing...\n");
        // tie rand seed to THE CURRENT TIME (so that each choice is unique)
        // choices of cards, pick and display
        // clickbox for the cards, once picked set choose = false

        //std::cout << "mouseWX: " << mouseWX << std::endl;
        //std::cout << "mouseWY: " << mouseWY << std::endl;

        //std::cout << "cards_x1: " << cards_x1 << std::endl;

        // card 1
        /*if (mouseWX > cards_x1 && mouseWX < (cards_x1 + 400) && mouseWY < (playerY - cards_y + 350) && mouseWY > (playerY - cards_y - 300)) {
            std::cout << "Red picked" << std::endl;
        } else if (mouseWX > cards_x2 && mouseWX < (cards_x2 + 400) && mouseWY < (playerY - cards_y + 350) && mouseWY >(playerY - cards_y - 300)) {
            std::cout << "Blue picked" << std::endl;
        } else if (mouseWX > (playerX - 200) && mouseWX < ((playerX - 200) + 400) && mouseWY < (playerY - cards_y + 350) && mouseWY >(playerY - cards_y - 300)) {
            std::cout << "Green picked" << std::endl;
        }*/

        if (cardsInPosition) {
            // cardsInPosition prevents picking all 3 at once

            if (AEInputCheckTriggered(AEVK_LBUTTON)) {
                if (IsMouseInside(mouseWX, mouseWY, cards_x1 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
                {
                    std::cout << "Red picked\n";
                    g_Augments.Choose(m_currentSet, m_cardIDs[0]);
                    std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[0]) << std::endl;
                    augmentSelected = true;
                    choose = false;
                }

                if (IsMouseInside(mouseWX, mouseWY, cards_x2 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
                {
                    std::cout << "Blue picked\n";
                    g_Augments.Choose(m_currentSet, m_cardIDs[1]);
                    std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[1]) << std::endl;
                    augmentSelected = true;
                    choose = false;
                }

                if (IsMouseInside(mouseWX, mouseWY, cards_x3 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
                {
                    std::cout << "Green picked\n";
                    g_Augments.Choose(m_currentSet, m_cardIDs[2]);
                    std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[2]) << std::endl;
                    augmentSelected = true;
                    choose = false;
                }
            }
        }

        // temp
        /*if (IsMouseInside(mouseWX, mouseWY, cards_x1 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
        {
            std::cout << "Red picked\n";
        }

        if (IsMouseInside(mouseWX, mouseWY, cards_x2 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
        {
            std::cout << "Blue picked\n";
        }

        if (IsMouseInside(mouseWX, mouseWY, cards_x3 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
        {
            std::cout << "Green picked\n";
        }*/

        //DrawMesh(cardMesh, 400, 600, cards_x1, playerY - cards_y, 0.0f, 255, 0, 0, 255); // Red Card (Left)
        //DrawMesh(cardMesh, 400, 600, cards_x2, playerY - cards_y, 0.0f, 0, 0, 255, 255); // Blue Card (Right)
        //DrawMesh(cardMesh, 400, 600, playerX - 200, playerY - cards_y, 0.0f, 0, 255, 0, 255); // Green Card (Middle)


        // Updates location, draw all at once in the end
        if (fabs(distanceY) > 2.f) {
            //DrawMesh(cardMesh, 400, 600, playerX - 200, playerY - cards_y, 0.0f, 255, 0, 0, 255); // Test

            //                       v speed at which the card travels
            cards_y += distanceY * 10.0f * deltaTime;
        }
        else {
            //DrawMesh(cardMesh, 400, 600, playerX - 200, playerY - cards_y, 0.0f, 255, 0, 0, 255); // Test

            if (fabs(distanceX1) > 2.f) {
                //DrawMesh(cardMesh, 400, 600, cards_x1, playerY - cards_y, 0.0f, 0, 255, 0, 255); // Test
                cards_x1 += distanceX1 * 8.0f * deltaTime;
            }

            if (fabs(distanceX2) > 2.f) {
                //DrawMesh(cardMesh, 400, 600, cards_x2, playerY - cards_y, 0.0f, 0, 0, 255, 255); // Test
                cards_x2 += distanceX2 * 8.0f * deltaTime;
            }
        }

        //std::cout << fabs((cameraX + 300) - cards_x2) << std::endl;
        cardsInPosition = fabs(distanceY) <= 2.f
            && fabs((playerX - 700) - cards_x1) <= 2.f
            && fabs((playerX + 300) - cards_x2) <= 2.f;
    }

}

void Augments::Draw(float camX, float camY) {

    // AUGMENT BALL DROPS DOWN FROM THE SKY
    // AUGMENT SPAWNS AFTER LAST ENEMY DEATH (store enemy last location (wave is a vector) when size of wave = 1)
    // Ensure Color Mode is set


    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    /*if (!m_candyTex)
        std::cout << "Failed to load candy texture!" << std::endl;
    else
        std::cout << "Candy texture loaded successfully" << std::endl;*/

    DrawMesh(augmentMesh, (augSize - 20) - sinf(hoverTime) * hoverPower, (isoHeight - 10) - (sinf(hoverTime) * hoverPower), augPosX, (augPosY - 65), 0.0f, 44, 50, 150, 128);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(m_candyTex, 0.0f, 0.0f);

    //AEGfxMeshDraw(candyMesh, AE_GFX_MDM_TRIANGLES);

    //DrawMesh(augmentMesh, augSize, augSize, augPosX, (hoverPosY + sinf(hoverTime) * hoverPower), 0.0f, 44, 50, 150, 255);

    float hoverOffset = sinf(hoverTime) * hoverPower;

    DrawTexture(
        candySprite,                // sprite object
        0,                          // currentDirection/frame (0 = one frame)
        candyMesh,                  // mesh
        m_candyTex,                 // candy texture
        augSize * 1.75,             // width of candy
        augSize * 1.75,             // height of candy
        augPosX,                    // x position
        hoverPosY + hoverOffset,    // y position with hover
        0.0f,                       // rotation (0 = no rotation)
        1.0f                        // size multiplier (1.0 = normal)
    );

    //DrawMesh(candyMesh, augSize, augSize, augPosX, augPosY, 0.0f, 255, 255, 255, 255);

    if (choose == true) {

        DrawMesh(cardMesh, 3200, 1800, windowTintX, windowTintY, 0.0f, 0, 0, 0, 100); // Tinted Window

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // drawing the cards and moving them to their picking positions
        //DrawMesh(cardMesh, cardWidth, cardHeight, cards_x1, cards_y, 0.0f, 255, 0, 0, 255); // Red Card (Left)
        //DrawMesh(cardMesh, cardWidth, cardHeight, cards_x2, cards_y, 0.0f, 0, 0, 255, 255); // Blue Card (Right)
        //DrawMesh(cardMesh, cardWidth, cardHeight, cards_x3, cards_y, 0.0f, 0, 255, 0, 255); // Green Card (Middle)

        for (int i = 0; i < 3; ++i) {
            float cardX = (i == 0 ? cards_x1 : (i == 1 ? cards_x2 : cards_x3));
            AEGfxTexture* tex = GetTextureForCard(m_cardIDs[i]);

            if (!tex) continue; // skip if texture failed to load

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

        // Draw augment text only after cards have settled into position
        //if (m_cardFont != -1 && cardsInPosition) {
        //    float cardCentersX[3] = {
        //        (cards_x1 - camX) + cardWidth * 0.5f,
        //        (cards_x2 - camX) + cardWidth * 0.5f,
        //        (cards_x3 - camX) + cardWidth * 0.5f
        //    };

        //    for (int i = 0; i < 3; ++i) {
        //        const AugmentInfo& info = GetAugmentInfo(m_cardIDs[i]);

        //        // Convert screen position to normalized coords (-1 to 1)
        //        float screenX = cardCentersX[i];
        //        float screenY = cards_y - camY;

        //        float tw, th;

        //        // Title (near top of card)
        //        AEGfxGetPrintSize(m_cardFont, info.name, 1.0f, &tw, &th);
        //        float titleNX = screenX / 800.0f - tw * 0.5f;
        //        float titleNY = (screenY + cardHeight * 0.3f) / 450.0f;
        //        AEGfxPrint(m_cardFont, info.name, titleNX, titleNY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        //        // Description (below title)
        //        AEGfxGetPrintSize(m_cardFont, info.description, 1.0f, &tw, &th);
        //        float descNX = screenX / 800.0f - tw * 0.5f;
        //        float descNY = (screenY + cardHeight * 0.15f) / 450.0f;
        //        AEGfxPrint(m_cardFont, info.description, descNX, descNY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        //    }
        //}


    }
}

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
}

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

    choose = false;
    augmentSelected = false;
    startingAnimation = true;
}