#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"

#include <algorithm>
#include <cmath>

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
		const auto allowsContactDamage = [&](const Actor& actor)
		{
			const Combatant* combatant = dynamic_cast<const Combatant*>(&actor);
			return !combatant || combatant->GetCombatRuntime()
				.GetContactDamageGuardRegistry().Allows(source, target);
		};

		return allowsContactDamage(source) && allowsContactDamage(target);
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

		// Temporary damage-suppression is owned by the ability that registered it.
		// Control effects, including Stun, deliberately do not belong here: they
		// stop player input and interrupt focus/ability execution, but must not
		// suppress direct, projectile, field, or ongoing damage already owned by
		// that combatant.
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

		const auto resolveCriticalMultiplier = [&]()
		{
			const auto* sourceCombatant = source ? dynamic_cast<const Combatant*>(source) : nullptr;
			return sourceCombatant ? sourceCombatant->GetCombatRuntime().GetCriticalDamageMultiplier() : 1.f;
		};
		float resolvedDamage = damage;
		bool wasCritical = false;
		if (payload.criticalPolicy != DamageCriticalPolicy::Disabled)
		{
			if (payload.criticalPolicy == DamageCriticalPolicy::Guaranteed)
			{
				resolvedDamage *= resolveCriticalMultiplier();
				wasCritical = true;
			}
			else if (payload.criticalPolicy == DamageCriticalPolicy::Random)
			{
				if (auto* sourceCombatant = source ? dynamic_cast<Combatant*>(source) : nullptr)
				{
					const float criticalChance = sourceCombatant->GetCombatRuntime().GetCriticalChance();
					if (RandRange(0.f, 1.f) < criticalChance)
					{
						resolvedDamage *= resolveCriticalMultiplier();
						wasCritical = true;
					}
				}
			}
		}
		if (payload.roundDamageUp)
		{
			resolvedDamage = std::ceil(resolvedDamage);
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

		target.ApplyDamage(resolvedDamage);
	}
}
