#pragma once
#include "Utils.h"

class Player;
class Enemy;
class Camera;

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
        f32 maxHealth;
    };

    struct CombatFlags {
        bool isAlive;
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

        bool gotHit;
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

        struct MovementData
        {
            int startUp;
            int active;
            int recovery;
        };
        struct MovementState
        {
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
            void Update(Player& player, Enemy& enemy, Camera& camera, float dt);
            // void Resolve(Player& player, Enemy& enemy, CombatOutcome outcome);
            bool CanStartAttack_Enemy(const Player& player, const Enemy& enemy) const;
            bool IsEnemyInRange(const Player& player, const Enemy& enemy) const;
            bool isPlayerParrying(const Player& player, const Enemy& enemy) const;
            CombatOutcome EvaluateAttack(const Player& player, const Enemy& enemy, float attackProgress) const;

            // Combat
            //void ApplyParryReaction_Enemy(Enemy& enemy);
            void ApplyBlockReaction_Enemy(Player& player, Enemy& enemy);
            //void ApplyGotHitReaction_Enemy(Player& player, Enemy& enemy);
            void ApplyKnockbackReaction_Enemy(Player& player, Enemy& enemy, double multiplier);
            void ApplyDamage(Player& player, Enemy& enemy);
            void ApplyDamage(Enemy& enemy, Player& player);
            void ColorIndicator(Enemy& enemy, f32 r, f32 g, f32 b, f32 a);
            double const GetOneFPS() const { return ONE_FRAME; };

            // Getters
            //AEVec2 GetVectorBetweenPE() const { return s_VectorBetweenPE; }
            //f32 GetDistMagPE() const { return s_DistMagPE; }
            //AEVec2 GetVectorNormalizedPE() const { return s_VectorNormalizedPE; }
            //f32 GetDotProduct() const { return s_DotProduct; }
            int const GetAttackerStopFrames() const { return attackerStopFrames; };
            int const GetDefenderStopFrames() const { return defenderStopFrames; };
            int const GetParryStopFrames() const { return parryStopFrames; };

        private:
            bool m_InRange { false };
            bool m_InCone{ false };

            float stunFrameAccumulator{};
            int stunCurrentFrame{};
            int stunRecoveryFrames{ 120 };
            Combat::CombatData::StunData stunFrames{ stunRecoveryFrames };
            
            f32 stunDuration{ 2.0f };

            int attackerStopFrames{ 4 };

            f32 parryFrameAccumulator{};
            int parryCurrentFrame{};
            int parryStopFrames{ 4 };

            f32 defenderFrameAccumulator{};
            int defenderCurrentFrame{};
            int defenderStopFrames{ 10 };

            CombatOutcome outcome{ CombatOutcome::OUTCOME_NONE };
            static double const ONE_FRAME;
    };
}