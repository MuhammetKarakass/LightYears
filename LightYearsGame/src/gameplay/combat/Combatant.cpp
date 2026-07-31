#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"

#include <algorithm>

namespace ly
{
	LightYearsAbilitySystemComponent& Combatant::GetAbilitySystemComponent()
	{
		return GetCombatRuntime().GetAbilitySystemComponent();
	}

	const LightYearsAbilitySystemComponent&
	Combatant::GetAbilitySystemComponent() const
	{
		return GetCombatRuntime().GetAbilitySystemComponent();
	}

	void ApplyCombatDamage(Actor& target, float damage, Actor* source, const List<GameplayTag>& damageTags)
	{
		ApplyCombatDamage(target, damage, source, damageTags, {});
	}

	void ApplyCombatDamage(
		Actor& target,
		float damage,
		Actor* source,
		const List<GameplayTag>& damageTags,
		const DamagePayload& payload
	)
	{
		if (damage <= 0.f)
		{
			return;
		}

		float resolvedDamage = damage;
		bool wasCritical = false;
		if (payload.canCrit)
		{
			if (auto* sourceCombatant = source ? dynamic_cast<Combatant*>(source) : nullptr)
			{
				const float criticalChance = sourceCombatant->GetCombatRuntime().GetCriticalChance();
				if (RandRange(0.f, 1.f) < criticalChance)
				{
					resolvedDamage *= std::max(1.f, payload.criticalDamageMultiplier);
					wasCritical = true;
				}
			}
		}

		if (auto* combatant = dynamic_cast<Combatant*>(&target))
		{
			DamageContext context;
			context.source = source;
			context.target = &target;
			context.originalDamage = resolvedDamage;
			context.remainingDamage = resolvedDamage;
			context.wasCritical = wasCritical;
			context.damageTags = damageTags;
			context.payload = payload;
			combatant->ReceiveDamage(context);
			return;
		}

		// Non-combat actors keep the engine's lightweight damage path.
		target.ApplyDamage(resolvedDamage);
	}
}
