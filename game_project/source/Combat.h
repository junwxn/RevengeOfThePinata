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
        bool stunned;
        //bool recovered;

        bool attackResolved;
        bool parryResolved;
        bool blockResolved;

        bool attackQueued;
    };

    struct CombatData {
        struct AttackData 
        {
            // Variables
            float startAngle;
            float endAngle;
            //bool recovered;

            // Frames
            int startUp;
            int active;
            int recovery;
            int total{ startUp + active + recovery };

            // Data Numbers
            int damage;
        };
        struct AttackState
        {
            bool recovered;
        };

        struct BlockData 
        {
            // Variables
            float startAngle;
            float endAngle;
            //bool held;
            //bool recovered;

            // Frames
            int startUp;
            int parry;
            int recovery;
            int total{ startUp + parry + recovery };

            // Data Numbers
            int block;
        };
        struct BlockState
        {
            bool held;
            bool recovered;
        };

        struct StunData
        {
            int recovery;
        };
    };



    f32 ComputeDamage(Player& attacker, Enemy& defender);
    f32 ComputeDamage(Enemy& attacker, Player& defender);

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
            void ColorIndicator(Enemy& enemy, f32 r, f32 g, f32 b, f32 a);
            double const GetOneFPS() const { return ONE_FRAME; };

            // Getters
            //AEVec2 GetVectorBetweenPE() const { return s_VectorBetweenPE; }
            //f32 GetDistMagPE() const { return s_DistMagPE; }
            //AEVec2 GetVectorNormalizedPE() const { return s_VectorNormalizedPE; }
            //f32 GetDotProduct() const { return s_DotProduct; }

        private:
            bool m_InRange { false };
            bool m_InCone{ false };

            float stunFrameAccumulator{};
            int stunCurrentFrame{};
            int stunRecoveryFrames{ 120 };
            Combat::CombatData::StunData stunFrames{ stunRecoveryFrames };
            
            f32 stunDuration{ 2.0f };
            CombatOutcome outcome;
            static double const ONE_FRAME;
    };
}