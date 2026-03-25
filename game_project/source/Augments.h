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

    AEGfxTexture* GetTextureForCard(AugmentID id) {
        switch (id) {
        case AugmentID::AMPLIFIED_DAMAGE: return m_amplifieddamageTex;
        case AugmentID::ATTACK_SPEED_BOOST: return m_attackmomentumTex;
        case AugmentID::CHAIN_ATTACK: return m_chainattackTex;
        case AugmentID::DAMAGING_MARK: return m_damagingmarkTex;
        case AugmentID::DASH_SPEED_BOOST: return m_dashmomentumTex;
        case AugmentID::MORE_PARRY_CHARGES: return m_parrychargesTex;
        case AugmentID::POISON_TRAIL: return m_poisontrailTex;
        case AugmentID::FASTER_PARRY: return m_quickparryTex;
        case AugmentID::SHIELD_DASH: return m_shielddashTex;
        default: return nullptr;
        }
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
    AEGfxTexture* m_candyTex = nullptr;

    AEGfxTexture* m_amplifieddamageTex = nullptr;
    AEGfxTexture* m_attackmomentumTex = nullptr;
    AEGfxTexture* m_chainattackTex = nullptr;
    AEGfxTexture* m_damagingmarkTex = nullptr;
    AEGfxTexture* m_dashmomentumTex = nullptr;
    AEGfxTexture* m_parrychargesTex = nullptr;
    AEGfxTexture* m_poisontrailTex = nullptr;
    AEGfxTexture* m_quickparryTex = nullptr;
    AEGfxTexture* m_shielddashTex = nullptr;


    AEGfxVertexList* augmentMesh = nullptr;
    AEGfxVertexList* candyMesh = nullptr;
    AEGfxVertexList* cardMesh = nullptr;
    s8 m_cardFont = -1;

    Sprite candySprite;

    Sprite cardSprite;

};