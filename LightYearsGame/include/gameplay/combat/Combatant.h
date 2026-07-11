#pragma once

#include "gameplay/damage/DamageContext.h"

namespace ly
{
	class Actor;
	class CombatRuntime;

	class Combatant
	{
	public:
		virtual ~Combatant() = default;
		virtual CombatRuntime& GetCombatRuntime() = 0;
		virtual const CombatRuntime& GetCombatRuntime() const = 0;
		virtual void ReceiveDamage(DamageContext context) = 0;
	};

	void ApplyCombatDamage(
		Actor& target,
		float damage,
		Actor* source = nullptr,
		const List<GameplayTag>& damageTags = {}
	);
}
