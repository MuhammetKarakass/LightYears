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
		ApplyCombatDamage(
			target,
			damage,
			source,
			damageTags,
			payload,
			sas::ContentId{},
			{}
		);
	}

	void ApplyCombatDamage(
		Actor& target,
		float damage,
		Actor* source,
		const List<GameplayTag>& damageTags,
		const DamagePayload& payload,
		const sas::ContentId& sourceAbilityId,
		const List<GameplayTag>& sourceAbilityTags
	)
	{
		if (damage <= 0.f)
		{
			return;
		}

		// Temporary protection is owned by the ability that registered it.
		// Keeping the gate at the shared damage entry point covers collision,
		// projectile, ability, and effect damage without coupling producers to
		// a concrete ability family.
		if (const auto* sourceCombatant = dynamic_cast<const Combatant*>(source))
		{
			if (sourceCombatant->GetCombatRuntime().BlocksOutgoingDamage())
			{
				return;
			}
		}
		if (const auto* targetCombatant = dynamic_cast<const Combatant*>(&target))
		{
			if (targetCombatant->GetCombatRuntime().BlocksIncomingDamage())
			{
				return;
			}
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
			context.sourceAbilityId = sourceAbilityId;
			context.sourceAbilityTags = sourceAbilityTags;
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
