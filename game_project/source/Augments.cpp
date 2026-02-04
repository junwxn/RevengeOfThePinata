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
            printf("CHOOSE BRO\n");
            choose = true;
        }
    }
    else if (choose) {
        printf("Choosing...\n");
    }

}

//void Augments::Choosing() {
//
//    // tie rand seed to delt time
//    // choices of cards, pick and display
//    // clickbox for the cards, once picked set choose = false
//
//
//    if (choose) {
//        printf("Choosing...\n");
//    }
//
//
//}

void Augments::Draw(f32 playerX, f32 playerY, f32 dt) {
    // Ensure Color Mode is set
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    hoverTime += dt * hoverSpeed;

    //// Calculate isometric squashed height for drawing
    float isoHeight = augSize * (GRID_H / GRID_W);

    DrawMesh(augmentMesh, (augSize - 20) - sinf(hoverTime) * hoverPower, (isoHeight - 10) - (sinf(hoverTime) * hoverPower), augPosX, augPosY - 65, 0.0f, 44, 50, 150, 128);

    // Draw using Utils helper
    // Color: Black (0,0,0) with full alpha (255)
    DrawMesh(augmentMesh, augSize, augSize, augPosX, hoverPosY + sinf(hoverTime) * hoverPower, 0.0f, 44, 50, 150, 255);

    if (choose == true) {
        DrawMesh(cardMesh, 3200, 1800, playerX - 1600, playerY, 0.0f, 0, 0, 0, 100); // Tinted Window

        DrawMesh(cardMesh, 400, 600, playerX - 700, playerY, 0.0f, 0, 0, 0, 255); // First card
        DrawMesh(cardMesh, 400, 600, playerX - 200, playerY, 0.0f, 0, 0, 0, 255); // Second card
        DrawMesh(cardMesh, 400, 600, playerX + 300, playerY, 0.0f, 0, 0, 0, 255); // Third card
    }
}

void Augments::DrawShadow(f32 dt) {
    // Ensure Color Mode is set
    /*AEGfxSetRenderMode(AE_GFX_RM_COLOR);

    hoverTime += dt * hoverSpeed;*/

}

void Augments::Free() {

}