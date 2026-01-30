#pragma once
#include "Utils.h"

namespace Combat {

    struct CombatStats {
        f32 attack;
        f32 defense;
        f32 critChance;
        f32 critMultiplier;
        f32 attackMultiplier;
    };

    struct CombatFlags {
        bool attackHit;
        bool blockOn;
        bool parryOn;
    };

    f32 ComputeDamage(CombatStats& attacker, CombatStats& defender);
}