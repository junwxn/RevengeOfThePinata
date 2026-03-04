#pragma once

enum class AugmentID {
    NONE = 0,
    // Set 1 - Dash
    SHIELD_DASH,
    POISON_TRAIL,
    DASH_SPEED_BOOST,
    // Set 2 - Attack
    CHAIN_ATTACK,
    DAMAGING_MARK,
    ATTACK_SPEED_BOOST,
    // Set 3 - Parry
    MORE_PARRY_CHARGES,
    FASTER_PARRY,
    AMPLIFIED_DAMAGE
};

enum class AugmentSet {
    SET_DASH = 1,
    SET_ATTACK = 2,
    SET_PARRY = 3
};

struct AugmentState {
    // Index 0 = Set1 (Dash), 1 = Set2 (Attack), 2 = Set3 (Parry)
    AugmentID chosen[3] = { AugmentID::NONE, AugmentID::NONE, AugmentID::NONE };

    bool Has(AugmentID id) const {
        for (int i = 0; i < 3; ++i) {
            if (chosen[i] == id) return true;
        }
        return false;
    }

    void Choose(AugmentSet set, AugmentID id) {
        int idx = static_cast<int>(set) - 1;
        if (idx >= 0 && idx < 3) {
            chosen[idx] = id;
        }
    }

    void Reset() {
        for (int i = 0; i < 3; ++i)
            chosen[i] = AugmentID::NONE;
    }
};

extern AugmentState g_Augments;

struct AugmentInfo {
    const char* name;
    const char* description;
};
const AugmentInfo& GetAugmentInfo(AugmentID id);
