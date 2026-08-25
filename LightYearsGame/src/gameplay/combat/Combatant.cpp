#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "gameplay/tags/GameplayTags.h"

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

	bool CanApplyContactDamage(const Actor& source, const Actor& target)
	{
		const auto blocksContactDamage = [](const Actor& actor)
		{
			const auto* combatant = dynamic_cast<const Combatant*>(&actor);
			return combatant && combatant->GetAbilitySystemComponent().HasOwnedTag(
				GameplayTags::State::Ability::EnergySpear::Traversing
			);
		};

		return !blocksContactDamage(source) && !blocksContactDamage(target);
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
		const List<GameplayTag>& sourceAbilityTags,
		DamageDeliveryType deliveryType,
		Actor* deliveryActor
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
			const auto& sourceTags = sourceCombatant->GetAbilitySystemComponent().GetOwnedTags();
			if (sourceTags.HasTag(GameplayTags::State::Effect::Control::Stunned) ||
				sourceCombatant->GetCombatRuntime().BlocksOutgoingDamage())
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
			context.deliveryType = deliveryType;
			context.deliveryActor = deliveryActor;
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
