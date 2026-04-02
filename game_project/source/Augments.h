/*************************************************************************
@file		Augments.h

@Author		Charles Yap, charles.y@digipen.edu

@Co-authors	Chiu Jun Wen j.chiu@digipen.edu (Help with the headers of the
            Augment values, Augment Select and for applying the Augments
            to the player and enemies)

@brief		This file contains the function declarations for the Augment Ball,
            including its initialization, updating, rendering, and cleanup
            for each round.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
#include "Utils.h" // Access to AE system and Grid constants
#include "AugmentData.h"

class Augments
{
public:
    void SetAugmentSet(AugmentSet set);

    // Initialization
    void Init();

    // Update
    void Update(f32 pX, f32 pY, f32 dt);

    // Draw
    void Draw(float camX, float camY);

    // Free
    void Free();

    // Set the world position where the Augment Ball spawns
    void SetPosition(float x, float y) { augPosX = x; augPosY = y; hoverPosY = y; }
    void SetSpawnAnim(bool active) { spawn_anim = active; }

    // Takes 'choose' from private
    bool GetChoose() const {
        return choose;
    }

    // Takes 'augmentSelected' from private
    bool GetAugmentSelected() const {
        return augmentSelected;
    }

    // Textures for the cards
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
    
    // If the augment has been selected
    bool augmentSelected = false;

    // Position & Stats
    float augPosX, augPosY;
    float augSize;
    float interactRange;
    float playerX, playerY;
    float deltaTime;

    // The 'hover' for the ball itself
    float isoHeight, hoverPosY, hoverTime, hoverPower, hoverSpeed;

    // The individual values for each aspect of the ball
    float windowTintX, windowTintY;
    float cardWidth, cardHeight;
    float cards_y, cards_x1, cards_x2, cards_x3, distanceY;
    float choiceCameraX;
    float choiceCameraY;

    // Distance between player and ball
    float playerballdist;

    // Boolean for the animation for spawning the ball
    bool spawn_anim = false;

    // If the player picked a card
    bool choose = false;

    // Starting the Card picking animation
    bool startingAnimation = false;

    // If cards are in place, allow the player to pick the card
    bool cardsInPosition = false;

    // Individual values for the Beam spawning animation right before the ball spawns
    float beamStartY;
    float beamX;
    float beamY;
    float beamTargetY;

    AugmentSet m_currentSet = AugmentSet::SET_DASH;
    AugmentID m_cardIDs[3] = { AugmentID::NONE, AugmentID::NONE, AugmentID::NONE };

    // Visual Assets
    AEGfxTexture* m_candyTex = nullptr;
    AEGfxTexture* interactTex = nullptr;
    AEGfxTexture* beamTex = nullptr;

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
    AEGfxVertexList* interactMesh = nullptr;
    AEGfxVertexList* beamMesh = nullptr;
    s8 m_cardFont = -1;

    Sprite interactSprite{};
    Sprite candySprite{};

    Sprite beamSprite{};

    Sprite cardSprite{};

};