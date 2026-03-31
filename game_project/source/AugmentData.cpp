/*************************************************************************
@file		AugmentData.cpp
@Author		Chiu Jun Wen j.chiu@digipen.edu
@Co-authors	nil
@brief		This file contains the implementation of augment-related data
            structures and functions, including the global augment state
            and a function to retrieve information about each augment.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/
#include "pch.h"
#include "AugmentData.h"

AugmentState g_Augments;

const AugmentInfo& GetAugmentInfo(AugmentID id) {
    static const AugmentInfo info[] = {
        { "Unknown",          "No augment selected" },          // NONE
        { "Shield Dash",      "Reflect damage while dashing" }, // SHIELD_DASH
        { "Poison Trail",     "Leave toxic clouds on dash" },   // POISON_TRAIL
        { "Dash Momentum",    "Speed boost after dashing" },    // DASH_SPEED_BOOST
        { "Chain Attack",     "Continue combo beyond 3 hits" }, // CHAIN_ATTACK
        { "Damaging Mark",    "Attacks mark, detonate in 3s" }, // DAMAGING_MARK
        { "Attack Momentum",  "Speed boost after hitting" },    // ATTACK_SPEED_BOOST
        { "Parry Charges",    "+2 attack charges on parry" },   // MORE_PARRY_CHARGES
        { "Quick Parry",      "Parry activates instantly" },    // FASTER_PARRY
        { "Amplified Damage", "Parried foes take 2x for 5s" }, // AMPLIFIED_DAMAGE
    };
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(sizeof(info) / sizeof(info[0])))
        idx = 0;
    return info[idx];
}
