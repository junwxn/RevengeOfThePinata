#pragma once
#include "Utils.h" // Access to AE system and Grid constants
#include "AugmentData.h"

class Augments
{
public:
    void SetAugmentSet(AugmentSet set);

    void Init();
    void Update(f32 playerX, f32 playerY, f32 dt);
    void Draw(float camX, float camY);
    void Free();

    //// Getters allowing Game.cpp to access position for Camera/Collisions
    //float GetX() const { return m_PosX; }
    //float GetY() const { return m_PosY; }
    //float GetSize() const { return m_Size; }

    // Set the world position where the augment sphere spawns
    void SetPosition(float x, float y) { augPosX = x; augPosY = y; hoverPosY = y; }


    // takes choose from private
    bool GetChoose() const {
        return choose;
    }

    bool GetAugmentSelected() const {
        return augmentSelected;
    }

    void Reset();

private:
    bool augmentSelected = false;
    // Position & Stats
    float augPosX, augPosY;
    float augSize;
    float interactRange;
    float playerX, playerY;
    float deltaTime;

    float isoHeight, hoverPosY, hoverTime, hoverPower, hoverSpeed;

    float windowTintX, windowTintY;
    float cardWidth, cardHeight;
    float cards_y, cards_x1, cards_x2, cards_x3, distanceY;
    float choiceCameraX;
    float choiceCameraY;

    bool choose = false;

    bool startingAnimation = true;

    bool cardsInPosition = false;

    AugmentSet m_currentSet = AugmentSet::SET_DASH;
    AugmentID m_cardIDs[3] = { AugmentID::NONE, AugmentID::NONE, AugmentID::NONE };

    // Visual Assets
    AEGfxVertexList* augmentMesh = nullptr;
    AEGfxVertexList* cardMesh = nullptr;
    s8 m_cardFont = -1;
};