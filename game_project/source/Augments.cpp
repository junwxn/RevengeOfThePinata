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

    // initializing these card positions
    cards_y = 0;
    cards_x1 = 0;
    cards_x2 = 0;

    choose = false;

    augmentMesh = CreateCircleMesh(1, 16, 0x000000);
    cardMesh = CreateRectMesh(0x000000);
}

void Augments::Interact(f32 playerX, f32 playerY) {

    float dx = playerX;
    float dy = playerY;

    float playerballdist = sqrt(((dx - augPosX) * (dx - augPosX)) + ((dy - (augPosY - 65)) * (dy - (augPosY - 65))));

    //printf("Player x: %f\n", dx);
    //printf("Player y: %f\n", dy);
    //printf("Playerballdist: %f\n", playerballdist);


    if (playerballdist < interactRange && !choose) {
        //printf("PRESS X TO INTERACT\n");

        if (AEInputCheckTriggered(AEVK_X)) {
            printf("CHOOSE ONCE\n");
            // setting the cards OG positions
            cards_y = 1000;
            cards_x1 = playerX - 200;
            cards_x2 = playerX - 200;
            choose = true;
        }
    }
    else if (choose) {
        printf("Choosing...\n");
        // tie rand seed to THE CURRENT TIME (so that each choice is unique)
        // choices of cards, pick and display
        // clickbox for the cards, once picked set choose = false
    }

}

void Augments::Draw(f32 playerX, f32 playerY, f32 dt) {
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    hoverTime += dt * hoverSpeed;

    // Calculate isometric squashed height for drawing
    float isoHeight = augSize * (GRID_H / GRID_W);

    DrawMesh(augmentMesh, (augSize - 20) - sinf(hoverTime) * hoverPower, (isoHeight - 10) - (sinf(hoverTime) * hoverPower), augPosX, augPosY - 65, 0.0f, 44, 50, 150, 128);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    DrawMesh(augmentMesh, augSize, augSize, augPosX, hoverPosY + sinf(hoverTime) * hoverPower, 0.0f, 44, 50, 150, 255);

    if (choose == true) {

        DrawMesh(cardMesh, 3200, 1800, playerX - 1600, playerY, 0.0f, 0, 0, 0, 100); // Tinted Window


        float distanceY = playerY - cards_y;
        float distanceX1 = (playerX - 700) - cards_x1;
        float distanceX2 = (playerX + 300) - cards_x2;

        // Updates location, draw all at once in the end
        if (fabs(distanceY) > 2.f) {
            //DrawMesh(cardMesh, 400, 600, playerX - 200, playerY - cards_y, 0.0f, 255, 0, 0, 255); // Test

            //                       v speed at which the card travels
            cards_y += distanceY * 10.0f * dt;
        }
        else {
            DrawMesh(cardMesh, 400, 600, playerX - 200, playerY - cards_y, 0.0f, 255, 0, 0, 255); // Test

            if (fabs(distanceX1) > 2.f) {
                //DrawMesh(cardMesh, 400, 600, cards_x1, playerY - cards_y, 0.0f, 0, 255, 0, 255); // Test
                cards_x1 += distanceX1 * 8.0f * dt;
            }

            if (fabs(distanceX2) > 2.f) {
                //DrawMesh(cardMesh, 400, 600, cards_x2, playerY - cards_y, 0.0f, 0, 0, 255, 255); // Test
                cards_x2 += distanceX2 * 8.0f * dt;
            }
        }

        // drawing the cards and moving them to their picking positions
        DrawMesh(cardMesh, 400, 600, cards_x1, playerY - cards_y, 0.0f, 255, 0, 0, 255); // Red Card (Left)
        DrawMesh(cardMesh, 400, 600, cards_x2, playerY - cards_y, 0.0f, 0, 0, 255, 255); // Blue Card (Right)
        DrawMesh(cardMesh, 400, 600, playerX - 200, playerY - cards_y, 0.0f, 0, 255, 0, 255); // Green Card (Middle)
      

    }
}

void Augments::DrawShadow(f32 dt) {
    // Ensure Color Mode is set
    /*AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    hoverTime += dt * hoverSpeed;*/

}

void Augments::Free() {

}