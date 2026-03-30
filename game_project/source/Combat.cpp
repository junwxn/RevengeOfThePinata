#include "pch.h"

#include "Combat.h"
#include "Player.h"
#include "Enemy.h"
#include "Camera.h"
#include "MathFunctions.h"
#include "EventSystem.h"
#include "AugmentData.h"
#include "AugmentEffects.h"
#include "Utils.h" // GRID_W, GRID_H
#include "Audio.h"


static std::ostream& operator<<(std::ostream& os, CombatOutcome outcome) {
	if (outcome == CombatOutcome::OUTCOME_HIT) return os << "OUTCOME_HIT";
	else if (outcome == CombatOutcome::OUTCOME_BLOCKED) return os << "OUTCOME_BLOCKED";
	else if (outcome == CombatOutcome::OUTCOME_PARRIED) return os << "OUTCOME_PARRIED";
	return os << static_cast<int>(outcome);
}

static std::ostream& operator<<(std::ostream& os, AEVec2 vector) {
	return os << vector.x << " " << vector.y << std::endl;
}

namespace Combat {
	double const System::ONE_FRAME{ 1.0f / 60.0f };

	f32 ComputeDamage(Player& attacker, Enemy& defender) {
		f32 damageDealt = attacker.GetCombatStats().attack - defender.GetCombatStats().defense;
		f32 result = defender.GetCombatFlag().blockOn ? damageDealt / 2 : damageDealt;
		// Amplified Damage augment: multiply damage if enemy is amplified
		if (defender.m_damageAmplified) {
			result *= defender.m_damageMultiplier;
		}
		return result;
	}

	f32 ComputeDamage(Enemy& attacker, Player& defender) {
		f32 damageDealt = attacker.GetCombatStats().attack - defender.GetCombatStats().defense;
		return defender.GetCombatFlag().blockOn ? damageDealt / 2 : damageDealt;
	}

	f32 ComputeDamage(Projectile& attacker, Player& defender) {
		f32 damageDealt = attacker.GetDamage() - defender.GetCombatStats().defense;
		return defender.GetCombatFlag().blockOn ? damageDealt / 2 : damageDealt;
	}

	f32 ComputeDamage(Projectile& attacker, Enemy& defender)
	{
		f32 damageDealt = attacker.GetDamage() - defender.GetCombatStats().defense;

		if (damageDealt < 1.0f)
		{
			damageDealt = 1.0f;
		}

		return damageDealt;
	}

	void System::Update(Player& player, Enemy& enemy, Camera& camera, float dt) {
			
		//if (enemy.IsStunned()) {
		//	
		//	stunFrameAccumulator += dt;
		//	
		//	while (stunFrameAccumulator >= ONE_FRAME)
		//	{
		//		++stunCurrentFrame;
		//		stunFrameAccumulator -= ONE_FRAME;
		//	}
		//	if (stunCurrentFrame >= stunRecoveryFrames) {
		//		enemy.ResetStunFlag();
		//		stunCurrentFrame = 0;
		//	}
		//	return;
		//}

		if (!player.GetIsAlive())
		{
			gAudio.PlayPlayerSFX(PLAYER_DEATH);
			gAudio.PlayEnemySFX(ENEMY_LAUGH);
			gAudio.PlayGeneralSFX(GENERAL_GAMEOVER);
		}
			
		
		if (enemy.IsGotHit()) 
		{
			camera.SetScreenShakeTimer(0.5f);
			defenderFrameAccumulator += dt;
			while (defenderFrameAccumulator >= ONE_FRAME)
			{
				++defenderCurrentFrame;
				defenderFrameAccumulator -= ONE_FRAME;
			}

			if (defenderCurrentFrame >= defenderStopFrames)
			{
				ApplyKnockbackReaction_Enemy(player, enemy, 2000.0);
				enemy.ResetGotHitFlag();
				defenderCurrentFrame = 0;
			}
		}

		if (enemy.IsParried())
		{
			camera.SetScreenShakeTimer(0.6f);
			parryFrameAccumulator += dt;
			while (parryFrameAccumulator >= ONE_FRAME)
			{
				++parryCurrentFrame;
				parryFrameAccumulator -= ONE_FRAME;
			}

			if (parryCurrentFrame >= parryStopFrames)
			{
				ApplyKnockbackReaction_Enemy(player, enemy, 2500.0);
				enemy.ResetParryFlag();
				parryCurrentFrame = 0;
				// Reward successful parry: skip remaining block recovery
				player.ResetCombatVariables();
			}
		}

		if (enemy.IsAttacking() && !enemy.GetCombatFlag().attackResolved) {
			// If the player moved out of range during wind-up, the attack whiffs
			if (!CanStartAttack_Enemy(player, enemy)) {
				enemy.MarkAttackResolved();
				return;
			}

			CombatOutcome outcome =
				EvaluateAttack(player, enemy, enemy.GetAttackProgress());
			std::cout << "Outcome: " << outcome << std::endl;

			switch (outcome)
			{
			case CombatOutcome::OUTCOME_PARRIED:
				std::cout << "IN PARRY" << std::endl;
				player.GainAttackCharge();
				enemy.SetParried(true);
				gAudio.PlayCombatSFX(COMBAT_PARRY);
				enemy.MarkAttackResolved();
				{
					// Fire ON_PARRY_SUCCESS event for augment effects
					EventData parryData;
					parryData.playerX = player.GetX();
					parryData.playerY = player.GetY();
					parryData.targetEnemy = &enemy;
					g_Events.Fire(GameEvent::ON_PARRY_SUCCESS, parryData);
				}
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
		double dx = player.GetX() - enemy.GetX();
		double dy = player.GetY() - enemy.GetY();

		double s_DistMagPE = Vectors::magnitude(dx, dy);

		return s_DistMagPE <= enemy.GetAttackRange();
	}

	bool System::IsEnemyInRange(const Player& player, const Enemy& enemy) const {
		//// Direction vector / Forward vector
		//AEVec2 s_VectorToEnemy = { enemy.GetX() - player.GetX(), enemy.GetY() - player.GetY() };

		//double s_DistMagPE = Vectors::magnitude(s_VectorToEnemy.x, s_VectorToEnemy.y); // Dist between mouse and player
		//// Normalize vectors (To get direction)
		//AEVec2 s_VectorNormalizedToEnemy = Vectors::normalize(s_DistMagPE, s_VectorToEnemy.x, s_VectorToEnemy.y); // Normalized vector between mouse and player

		//f32 dotProduct = (player.GetNormalizedVector().x * s_VectorNormalizedToEnemy.x 
		//	+ player.GetNormalizedVector().y * s_VectorNormalizedToEnemy.y);

		//return s_DistMagPE <= player.GetAttackRange() &&
		//	dotProduct >= player.GetConeThreshold();

		double dx = enemy.GetX() - player.GetX();
		double dy = enemy.GetY() - player.GetY();

		double dist = Vectors::magnitude(dx, dy);

		// Outside attack radius
		if (dist > player.GetAttackRange())
			return false;

		// Angle to enemy
		float enemyAngle = atan2f(dy, dx);

		float start = player.GetStartAngle();
		float current = player.GetCurrentAngle();

		auto Normalize = [](float a)
			{
				while (a > PI)  a -= 2.0f * PI;
				while (a < -PI) a += 2.0f * PI;
				return a;
			};

		enemyAngle = Normalize(enemyAngle);
		start = Normalize(start);
		current = Normalize(current);

		// Handle sweep direction
		if (start < current)
		{
			return enemyAngle >= start && enemyAngle <= current;
		}
		else
		{
			return enemyAngle <= start && enemyAngle >= current;
		}
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

	//void System::ApplyParryReaction_Enemy(Player& player, Enemy& enemy) {
	//	ColorIndicator(enemy, 255, 255, 0, 1);
	//	AEVec2 knockbackDir{ enemy.GetX() - player.GetX(), enemy.GetY() - player.GetY() };
	//	AEVec2Normalize(&knockbackDir, &knockbackDir);
	//	AEVec2Scale(&knockbackDir, &knockbackDir, 500.0);
	//	enemy.SetKnockbackVelocity(knockbackDir);
	//}

	void System::ApplyBlockReaction_Enemy(Player& player, Enemy& enemy) {
		std::cout << "CALLING APPLYBLOCKREACTION ENEMY" << std::endl;
		player.SetPosition(player.GetX() - 5.0f, player.GetY()); // Knock defender back
		player.DeductHealth(ComputeDamage(enemy, player));
	}

	//void System::ApplyGotHitReaction_Enemy(Player& player, Enemy& enemy)
	//{
	//	AEVec2 knockbackDir{ enemy.GetX() - player.GetX(), enemy.GetY() - player.GetY() };
	//	AEVec2Normalize(&knockbackDir, &knockbackDir);
	//	AEVec2Scale(&knockbackDir, &knockbackDir, 500.0);
	//	enemy.SetKnockbackVelocity(knockbackDir);
	//	//std::cout << enemy.GetEnemyDirectionVector();
	//}

	void System::ApplyKnockbackReaction_Enemy(Player& player, Enemy& enemy, double multiplier)
	{
		AEVec2 knockbackDir{ enemy.GetX() - player.GetX(), enemy.GetY() - player.GetY() };
		AEVec2Normalize(&knockbackDir, &knockbackDir);
		AEVec2Scale(&knockbackDir, &knockbackDir, multiplier);
		enemy.SetKnockbackVelocity(knockbackDir);

		gAudio.PlayEnemySFX(ENEMY_VOCAL);
		//std::cout << "KNOCKBACK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
	}

	void System::ApplyDamage(Player& player, Enemy& enemy) {
		// Shield Dash augment: if shield is active, reflect damage instead
		if (g_Augments.Has(AugmentID::SHIELD_DASH) && AugmentEffects_IsShieldActive()) {
			f32 reflectedDmg = ComputeDamage(enemy, player);
			enemy.DeductHealth(reflectedDmg);
			enemy.SetHDP(reflectedDmg);
			return;
		}
		player.DeductHealth(ComputeDamage(enemy, player));
		player.SetHDP(ComputeDamage(enemy, player));
		gAudio.PlayPlayerSFX(PLAYER_HURT);
	}
	void System::ApplyDamage(Player& player, Projectile& projectile) {
		player.DeductHealth(ComputeDamage(projectile, player));
		player.SetHDP(ComputeDamage(projectile, player));
		gAudio.PlayPlayerSFX(PLAYER_HURT);
	}

	void System::ApplyDamage(Enemy& enemy, Player& player) {
		f32 dmg = ComputeDamage(player, enemy);
		enemy.DeductHealth(dmg);
		enemy.SetHDP(dmg);
		// Damaging Mark augment: accumulate damage on marked enemies
		if (enemy.m_marked) {
			enemy.m_markAccumulatedDamage += dmg;
		}
		gAudio.PlayEnemySFX(ENEMY_HURT);
	}

	void System::ApplyDamage(Enemy& enemy, Projectile& projectile) {
		f32 dmg = ComputeDamage(projectile, enemy);

		enemy.DeductHealth(dmg);
		enemy.SetHDP(dmg);

		// Damaging Mark augment: accumulate damage on marked enemies
		if (enemy.m_marked)
		{
			enemy.m_markAccumulatedDamage += dmg;
		}

		gAudio.PlayEnemySFX(ENEMY_HURT);
	}

	void System::ColorIndicator(Enemy& enemy, f32 r, f32 g, f32 b, f32 a) {
		f32 isoHeight = enemy.GetSize() * (GRID_H / GRID_W);

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		DrawMesh(enemy.GetEnemyMesh(), enemy.GetSize(), isoHeight, enemy.GetX(), enemy.GetY(), 0.0f, r, g, b, a);
	}

	void System::ApplyProjectileDamage(Player& player, Enemy& enemy, f32 damage)
	{
		// Shield Dash augment: if shield is active, reflect projectile damage instead
		if (g_Augments.Has(AugmentID::SHIELD_DASH) && AugmentEffects_IsShieldActive()) {
			enemy.DeductHealth(damage);
			enemy.SetHDP(damage);
			return;
		}

		f32 finalDamage = damage;

		if (player.GetCombatFlag().blockOn) {
			finalDamage *= 0.5f;
		}

		finalDamage -= player.GetCombatStats().defense;

		if (finalDamage < 1.0f) {
			finalDamage = 1.0f;
		}

		player.DeductHealth(finalDamage);
		player.SetHDP(finalDamage);
		gAudio.PlayPlayerSFX(PLAYER_HURT);
	}

	//void System::Resolve(Player& player, Enemy& enemy, CombatOutcome outcome) {
	//	switch (outcome) 
	//	{
	//		case CombatOutcome::OUTCOME_PARRIED:
	//			ApplyKnockbackReaction_Enemy(player, enemy);
	//			player.GainAttackCharge();
	//			enemy.ResetParryFlag();
	//			enemy.MarkAttackResolved();
	//			break;
	//		case CombatOutcome::OUTCOME_BLOCKED:
	//			ApplyBlockReaction_Enemy(player, enemy);
	//			enemy.MarkAttackResolved();
	//			break;
	//		case CombatOutcome::OUTCOME_HIT:
	//			ApplyDamage(player, enemy);
	//			enemy.MarkAttackResolved();
	//			break;
	//	}
	//}
}
