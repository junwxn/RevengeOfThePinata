#include "Combat.h"
#include "Player.h"
#include "Enemy.h"
#include "MathFunctions.h"
#include <iostream>

static std::ostream& operator<<(std::ostream& os, CombatOutcome outcome) {
	if (outcome == CombatOutcome::OUTCOME_HIT) return os << "OUTCOME_HIT";
	else if (outcome == CombatOutcome::OUTCOME_BLOCKED) return os << "OUTCOME_BLOCKED";
	else if (outcome == CombatOutcome::OUTCOME_PARRIED) return os << "OUTCOME_PARRIED";
	return os << static_cast<int>(outcome);
}

namespace Combat {
	f32 ComputeDamage(Player& attacker, Enemy& defender) {
		f32 damageDealt = attacker.GetCombatStats().attack - defender.GetCombatStats().defense;
		return defender.GetCombatFlag().blockOn ? damageDealt / 2 : damageDealt;
	}
	f32 ComputeDamage(Enemy& attacker, Player& defender) {
		f32 damageDealt = attacker.GetCombatStats().attack - defender.GetCombatStats().defense;
		return defender.GetCombatFlag().blockOn ? damageDealt / 2 : damageDealt;
	}

	void System::Update(Player& player, Enemy& enemy, float dt) {
			
		if (enemy.IsStunned()) {
			stunDuration -= dt;
			//std::cout << stunDuration << std::endl;

			if (stunDuration <= 0) {
				enemy.ResetStunFlag();
				stunDuration = 2.0f;
			}
			return;
		}

		if (enemy.IsAttacking() && !enemy.GetCombatFlag().attackResolved) {
			CombatOutcome outcome =
				EvaluateAttack(player, enemy, enemy.GetAttackProgress());
			std::cout << "Outcome: " << outcome << std::endl;

			switch (outcome)
			{
			case CombatOutcome::OUTCOME_PARRIED:
				player.GainAttackCharge();
				ApplyParryReaction_Enemy(enemy);
				enemy.SetParried(true);
				enemy.SetStunned(true);
				std::cout << "Attack Charges: " << player.GetAttackCharges() << std::endl;

				break;

			case CombatOutcome::OUTCOME_BLOCKED:
				ApplyBlockReaction_Enemy(player, enemy);
				break;

			case CombatOutcome::OUTCOME_HIT:
				ApplyDamage(player, enemy);
				break;
			}
			
			if(!enemy.IsStunned()) enemy.MarkAttackResolved();
		}
		else return;

		
	}

	bool System::CanStartAttack_Enemy(const Player& player, const Enemy& enemy) const {
		// Direction vector / Forward vector
		AEVec2 s_VectorToPlayer = { player.GetX() - enemy.GetX(), player.GetY() - enemy.GetY() };

		f32 s_DistMagPE = Vectors::magnitude(s_VectorToPlayer.x, s_VectorToPlayer.y); // Dist between player and enemy
		// Normalize vectors (To get direction)
		AEVec2 s_VectorNormalizedToPlayer = Vectors::normalize(s_DistMagPE, s_VectorToPlayer.x, s_VectorToPlayer.y); // Normalized vector between mouse and player

		//f32 dotProduct = (enemy.GetNormalizedVector().x * s_VectorNormalizedToPlayer.x + enemy.GetNormalizedVector().y * s_VectorNormalizedToPlayer.y);

		return s_DistMagPE <= enemy.GetAttackRange();
	}

	bool System::IsEnemyInRange(const Player& player, const Enemy& enemy) const {
		// Direction vector / Forward vector
		AEVec2 s_VectorToEnemy = { enemy.GetX() - player.GetX(), enemy.GetY() - player.GetY() };

		f32 s_DistMagPE = Vectors::magnitude(s_VectorToEnemy.x, s_VectorToEnemy.y); // Dist between mouse and player
		// Normalize vectors (To get direction)
		AEVec2 s_VectorNormalizedToEnemy = Vectors::normalize(s_DistMagPE, s_VectorToEnemy.x, s_VectorToEnemy.y); // Normalized vector between mouse and player

		f32 dotProduct = (player.GetNormalizedVector().x * s_VectorNormalizedToEnemy.x 
			+ player.GetNormalizedVector().y * s_VectorNormalizedToEnemy.y);

		return s_DistMagPE <= player.GetAttackRange() &&
			dotProduct >= player.GetConeThreshold();
	}

	bool System::isPlayerParrying(const Player& player, const Enemy& enemy) const {
		return player.GetBlockStatus() && player.GetParryStatus();
	}

	CombatOutcome System::EvaluateAttack(const Player& player, const Enemy& enemy, float attackProgress) const {
		if (attackProgress < 0.5f) {
			if (player.GetParryStatus()) return CombatOutcome::OUTCOME_PARRIED;
		}
		else if (player.IsBlocking()) return CombatOutcome::OUTCOME_BLOCKED;

		return CombatOutcome::OUTCOME_HIT;
	}

	void System::ApplyParryReaction_Enemy(Enemy& enemy) {
		ColorIndicator(enemy, 255, 255, 0, 1);
		enemy.SetPosition(enemy.GetX() + 50.0f, enemy.GetY()); // Knock enemy back
	}

	void System::ApplyBlockReaction_Enemy(Player& player, Enemy& enemy) {
		player.SetPosition(player.GetX() - 5.0f, player.GetY()); // Knock defender back
		player.DeductHealth(ComputeDamage(enemy, player));
	}

	void System::ApplyDamage(Player& player, Enemy& enemy) {
		player.DeductHealth(ComputeDamage(enemy, player));
	}

	void System::ColorIndicator(Enemy& enemy, f32 r, f32 g, f32 b, f32 a) {
		f32 isoHeight = enemy.GetSize() * (GRID_H / GRID_W);

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		DrawMesh(enemy.GetEnemyMesh(), enemy.GetSize(), isoHeight, enemy.GetX(), enemy.GetY(), 0.0f, r, g, b, a);
	}


	void System::Resolve(Player& player, Enemy& enemy, CombatOutcome outcome) {
		switch (outcome) 
		{
			case CombatOutcome::OUTCOME_PARRIED:
				ApplyParryReaction_Enemy(enemy);
				player.GainAttackCharge();
				enemy.ResetParryFlag();
				enemy.MarkAttackResolved();
				break;

			case CombatOutcome::OUTCOME_BLOCKED:
				ApplyBlockReaction_Enemy(player, enemy);
				enemy.MarkAttackResolved();
				break;

			case CombatOutcome::OUTCOME_HIT:
				ApplyDamage(player, enemy);
				enemy.MarkAttackResolved();
				break;
		}
	}
}
