#include "Combat.h"

namespace Combat {
	f32 ComputeDamage(CombatStats& attacker, CombatStats& defender) {
		f32 damageDealt = attacker.attack - defender.defense;
		return damageDealt;
	}
}