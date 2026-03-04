#include "pch.h" // Has to be first file to include in .cpp
#include "Augments.h"
#include "Player.h"
#include <math.h> // For sqrt

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

    augmentMesh = CreateCircleMesh(1, 16, 0x000000);
    cardMesh = CreateRectMesh(0x000000);
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

    // distance of the player and ball
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
            cards_y = cameraY + 1000;
            cards_x1 = cameraX - 200;
            cards_x2 = cameraX - 200;
            cards_x3 = cameraX - 200;
            choose = true;
        }
    }
    else if (choose) {

        windowTintX = cameraX - 1600;
        windowTintY = cameraY;

        distanceY = cameraY - cards_y;

        float distanceX1 = (cameraX - 700) - cards_x1;
        float distanceX2 = (cameraX + 300) - cards_x2;
        //printf("Choosing...\n");
        // tie rand seed to THE CURRENT TIME (so that each choice is unique)
        // choices of cards, pick and display
        // clickbox for the cards, once picked set choose = false

        //std::cout << "mouseWX: " << mouseWX << std::endl;
        //std::cout << "mouseWY: " << mouseWY << std::endl;

        /*std::cout << "cards_y: " << cards_y
            << " | playerY: " << playerY
            << " | cameraY: " << cameraY
            << std::endl;*/


        if (IsMouseInside(mouseWX, mouseWY, cards_x1 + (cardWidth * 0.5), cards_y, cardWidth, cardHeight))
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

void Augments::Draw() {
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);


    DrawMesh(augmentMesh, (augSize - 20) - sinf(hoverTime) * hoverPower, (isoHeight - 10) - (sinf(hoverTime) * hoverPower), augPosX, augPosY - 65, 0.0f, 44, 50, 150, 128);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    DrawMesh(augmentMesh, augSize, augSize, augPosX, hoverPosY + sinf(hoverTime) * hoverPower, 0.0f, 44, 50, 150, 255);

    if (choose == true) {

        DrawMesh(cardMesh, 3200, 1800, windowTintX, windowTintY, 0.0f, 0, 0, 0, 100); // Tinted Window

        //// drawing the cards and moving them to their picking positions
        //DrawMesh(cardMesh, cardWidth, cardHeight, cards_x1, distanceY, 0.0f, 255, 0, 0, 255); // Red Card (Left)
        //DrawMesh(cardMesh, cardWidth, cardHeight, cards_x2, distanceY, 0.0f, 0, 0, 255, 255); // Blue Card (Right)
        //DrawMesh(cardMesh, cardWidth, cardHeight, cards_x3, distanceY, 0.0f, 0, 255, 0, 255); // Green Card (Middle)

        // drawing the cards and moving them to their picking positions
        DrawMesh(cardMesh, cardWidth, cardHeight, cards_x1, cards_y, 0.0f, 255, 0, 0, 255); // Red Card (Left)
        DrawMesh(cardMesh, cardWidth, cardHeight, cards_x2, cards_y, 0.0f, 0, 0, 255, 255); // Blue Card (Right)
        DrawMesh(cardMesh, cardWidth, cardHeight, cards_x3, cards_y, 0.0f, 0, 255, 0, 255); // Green Card (Middle)
      

    }
}

void Augments::Free() {

}