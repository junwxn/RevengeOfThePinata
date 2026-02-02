#pragma once
#include "Utils.h"

class Player;
class Enemy;

enum class CombatOutcome {
    OUTCOME_NONE,
    OUTCOME_HIT,
    OUTCOME_BLOCKED,
    OUTCOME_PARRIED
};

namespace Combat {

    struct CombatStats {
        f32 health;
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
        bool blocked;
        bool parried;

        bool attackResolved;
        bool parryResolved;
        bool blockResolved;
    };

    f32 ComputeDamage(CombatStats& attacker, CombatStats& defender);

    class System 
    {
        public:
            void Update(Player& player, Enemy& enemy, float dt);
            void Resolve(Player& player, Enemy& enemy, CombatOutcome outcome);
            bool CanStartAttack_Enemy(const Player& player, const Enemy& enemy) const;
            bool IsEnemyInRange(const Player& player, const Enemy& enemy) const;
            bool isPlayerParrying(const Player& player, const Enemy& enemy) const;
            CombatOutcome EvaluateAttack(const Player& player, const Enemy& enemy, float attackProgress) const;

            // Combat
            void ApplyParryReaction_Enemy(Enemy& enemy);
            void ApplyBlockReaction_Enemy(Player& player, Enemy& enemy);
            void ApplyDamage(Player& player, Enemy& enemy);

            // Getters
            //AEVec2 GetVectorBetweenPE() const { return s_VectorBetweenPE; }
            //f32 GetDistMagPE() const { return s_DistMagPE; }
            //AEVec2 GetVectorNormalizedPE() const { return s_VectorNormalizedPE; }
            //f32 GetDotProduct() const { return s_DotProduct; }

        private:
            bool m_InRange;
            bool m_InCone;
            CombatOutcome outcome;
    };
}