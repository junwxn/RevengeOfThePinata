#pragma once
#include "Utils.h" // Access to AE system and Grid constants
#include "AugmentData.h"

class Augments
{
public:
    void SetAugmentSet(AugmentSet set);

    void Init();
    void Update(f32 playerX, f32 playerY, f32 dt, f32 cameraX, f32 cameraY);
    void Draw();
    void Free();

    //// Getters allowing Game.cpp to access position for Camera/Collisions
    //float GetX() const { return m_PosX; }
    //float GetY() const { return m_PosY; }
    //float GetSize() const { return m_Size; }

    //// Setters if you need to teleport the player (e.g. respawning)
    //void SetPosition(float x, float y) { m_PosX = x; m_PosY = y; }


    // takes choose from private
    bool GetChoose() const {
        return choose;
    }


    // types of Augments
    enum AugmentType {
        NONE = 0,
        DOUBLEDASH = 1,
        SHIELDDASH = 2,
        PARRYCHARGER = 3
    };

    void AddAugment(AugmentType type) { // adds chosen augment to end of vector
        chosenAugment.push_back(type);
    }

    // returns the entire vector
    const std::vector<AugmentType>& GetChosenAugments() const { return chosenAugment; }

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

    bool choose = false;

    bool startingAnimation = true;

    // Current chosen Augment vector
    std::vector<AugmentType> chosenAugment;
    AugmentSet m_currentSet = AugmentSet::SET_DASH;
    AugmentID m_cardIDs[3] = { AugmentID::NONE, AugmentID::NONE, AugmentID::NONE };

    // Visual Assets
    AEGfxVertexList* augmentMesh = nullptr;
    AEGfxVertexList* cardMesh = nullptr;
    s8 m_cardFont = -1;
};