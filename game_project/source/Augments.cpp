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

    augmentMesh = CreateCircleMesh(1, 16, 0x000000);
    cardMesh = CreateRectMesh(0x000000);
    m_cardFont = AEGfxCreateFont("Assets/liberation-mono.ttf", 24);
}

void Augments::Update(f32 playerX, f32 playerY, f32 dt, f32 cameraX, f32 cameraY) {

    float dx = playerX;
    float dy = playerY;
    deltaTime = dt;

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float mouseWX = mouseX - AEGfxGetWindowWidth() * 0.5f;
    float mouseWY = AEGfxGetWindowHeight() * 0.5f - mouseY;

    mouseWX += cameraX;
    mouseWY += cameraY;

    float playerballdist = sqrt(((dx - augPosX) * (dx - augPosX)) + ((dy - (augPosY - 65)) * (dy - (augPosY - 65))));

    //printf("Player x: %f\n", dx);
    //printf("Player y: %f\n", dy);
    //printf("Playerballdist: %f\n", playerballdist);

    // Drawing calculations
    hoverTime += deltaTime * hoverSpeed;

    // Calculate isometric squashed height for drawing
    isoHeight = augSize * (GRID_H / GRID_W);

    if (playerballdist < interactRange && !choose) {
        //printf("PRESS X TO INTERACT\n");

        if (AEInputCheckTriggered(AEVK_X)) {
            printf("CHOOSE ONCE\n");
            // setting the cards OG positions
            cards_y = 1000;
            cards_x1 = playerX - 200;
            cards_x2 = playerX - 200;
            cards_x3 = playerX - 200;
            choose = true;
        }
    }
    else if (choose) {

        windowTintX = playerX - 1600;
        windowTintY = playerY;

        distanceY = playerY - cards_y;

        float distanceX1 = (playerX - 700) - cards_x1;
        float distanceX2 = (playerX + 300) - cards_x2;
        //printf("Choosing...\n");
        // tie rand seed to THE CURRENT TIME (so that each choice is unique)
        // choices of cards, pick and display
        // clickbox for the cards, once picked set choose = false

        //std::cout << "mouseWX: " << mouseWX << std::endl;
        std::cout << "mouseWY: " << mouseWY << std::endl;

        //std::cout << "cards_x1: " << cards_x1 << std::endl;

        // card 1
        /*if (mouseWX > cards_x1 && mouseWX < (cards_x1 + 400) && mouseWY < (playerY - cards_y + 350) && mouseWY > (playerY - cards_y - 300)) {
            std::cout << "Red picked" << std::endl;
        } else if (mouseWX > cards_x2 && mouseWX < (cards_x2 + 400) && mouseWY < (playerY - cards_y + 350) && mouseWY >(playerY - cards_y - 300)) {
            std::cout << "Blue picked" << std::endl;
        } else if (mouseWX > (playerX - 200) && mouseWX < ((playerX - 200) + 400) && mouseWY < (playerY - cards_y + 350) && mouseWY >(playerY - cards_y - 300)) {
            std::cout << "Green picked" << std::endl;
        }*/

        if (AEInputCheckTriggered(AEVK_LBUTTON)) {
            if (IsMouseInside(mouseWX, mouseWY, cards_x1 + (cardWidth * 0.5), distanceY, cardWidth, cardHeight))
            {
                std::cout << "Red picked\n";
                g_Augments.Choose(m_currentSet, m_cardIDs[0]);
                std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[0]) << std::endl;
                augmentSelected = true;
                choose = false;
            }

            if (IsMouseInside(mouseWX, mouseWY, cards_x2 + (cardWidth * 0.5), distanceY, cardWidth, cardHeight))
            {
                std::cout << "Blue picked\n";
                g_Augments.Choose(m_currentSet, m_cardIDs[1]);
                std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[1]) << std::endl;
                augmentSelected = true;
                choose = false;
            }

            if (IsMouseInside(mouseWX, mouseWY, cards_x3 + (cardWidth * 0.5), distanceY, cardWidth, cardHeight))
            {
                std::cout << "Green picked\n";
                g_Augments.Choose(m_currentSet, m_cardIDs[2]);
                std::cout << "Augment chosen: " << static_cast<int>(m_cardIDs[2]) << std::endl;
                augmentSelected = true;
                choose = false;
            }
        }

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
    }

}

void Augments::Draw(f32 playerX, f32 playerY) {
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);


    DrawMesh(augmentMesh, (augSize - 20) - sinf(hoverTime) * hoverPower, (isoHeight - 10) - (sinf(hoverTime) * hoverPower), augPosX, augPosY - 65, 0.0f, 44, 50, 150, 128);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    DrawMesh(augmentMesh, augSize, augSize, augPosX, hoverPosY + sinf(hoverTime) * hoverPower, 0.0f, 44, 50, 150, 255);

    if (choose == true) {

        DrawMesh(cardMesh, 3200, 1800, windowTintX, windowTintY, 0.0f, 0, 0, 0, 100); // Tinted Window

        // drawing the cards and moving them to their picking positions
        DrawMesh(cardMesh, cardWidth, cardHeight, cards_x1, distanceY, 0.0f, 255, 0, 0, 255); // Red Card (Left)
        DrawMesh(cardMesh, cardWidth, cardHeight, cards_x2, distanceY, 0.0f, 0, 0, 255, 255); // Blue Card (Right)
        DrawMesh(cardMesh, cardWidth, cardHeight, cards_x3, distanceY, 0.0f, 0, 255, 0, 255); // Green Card (Middle)

        // Draw augment text only after cards have settled into position
        bool cardsInPosition = fabs(distanceY) <= 2.f
            && fabs((playerX - 700) - cards_x1) <= 2.f
            && fabs((playerX + 300) - cards_x2) <= 2.f;
        if (m_cardFont != -1 && cardsInPosition) {
            float cardCentersX[3] = {
                cards_x1 + cardWidth * 0.5f,
                cards_x2 + cardWidth * 0.5f,
                cards_x3 + cardWidth * 0.5f
            };

            for (int i = 0; i < 3; ++i) {
                const AugmentInfo& info = GetAugmentInfo(m_cardIDs[i]);

                // Convert world position to screen-normalized coords (-1 to 1)
                float screenX = cardCentersX[i] - playerX;
                float screenY = distanceY - playerY;

                float tw, th;

                // Title (near top of card)
                AEGfxGetPrintSize(m_cardFont, info.name, 1.0f, &tw, &th);
                float titleNX = screenX / 800.0f - tw * 0.5f;
                float titleNY = (screenY + cardHeight * 0.3f) / 450.0f;
                AEGfxPrint(m_cardFont, info.name, titleNX, titleNY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

                // Description (below title)
                AEGfxGetPrintSize(m_cardFont, info.description, 1.0f, &tw, &th);
                float descNX = screenX / 800.0f - tw * 0.5f;
                float descNY = (screenY + cardHeight * 0.15f) / 450.0f;
                AEGfxPrint(m_cardFont, info.description, descNX, descNY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

    }
}

void Augments::DrawShadow(f32 dt) {
    // Ensure Color Mode is set
    /*AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    hoverTime += dt * hoverSpeed;*/

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
	if (m_cardFont != -1) {
		AEGfxDestroyFont(m_cardFont);
		m_cardFont = -1;
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