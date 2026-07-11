#include "gameplay/combat/Combatant.h"
#include "framework/Actor.h"

namespace ly
{
	void ApplyCombatDamage(Actor& target, float damage, Actor* source, const List<GameplayTag>& damageTags)
	{
		if (damage <= 0.f)
		{
			return;
		}

		if (auto* combatant = dynamic_cast<Combatant*>(&target))
		{
			DamageContext context;
			context.source = source;
			context.target = &target;
			context.originalDamage = damage;
			context.remainingDamage = damage;
			context.damageTags = damageTags;
			combatant->ReceiveDamage(context);
			return;
		}

		// Non-combat actors keep the engine's lightweight damage path.
		target.ApplyDamage(damage);
	}
}
