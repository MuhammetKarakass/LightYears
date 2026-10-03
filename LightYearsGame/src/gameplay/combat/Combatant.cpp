#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include "framework/MathUtility.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

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

		const auto* sourceCombatant = source ? dynamic_cast<const Combatant*>(source) : nullptr;
		const CombatRuntime::OutgoingDamagePlan outgoingModulePlan = sourceCombatant && !payload.skipModuleDamageRules
			? sourceCombatant->GetCombatRuntime().ResolveOutgoingDamagePlan(target, sourceAbilityTags)
			: CombatRuntime::OutgoingDamagePlan{};
		const auto resolveCriticalMultiplier = [&]()
		{
			return sourceCombatant ? sourceCombatant->GetCombatRuntime().GetCriticalDamageMultiplier() : 1.f;
		};
		const float sourceOutgoingDamageMultiplier =
			sourceCombatant ? sourceCombatant->GetCombatRuntime().GetOutgoingDamageMultiplier() : 1.f;
		long double sourceResolvedDamage = static_cast<long double>(damage) * outgoingModulePlan.damageMultiplier * sourceOutgoingDamageMultiplier;
		bool wasCritical = false;
		if (payload.criticalPolicy != DamageCriticalPolicy::Disabled)
		{
			if (payload.criticalPolicy == DamageCriticalPolicy::Guaranteed)
			{
				sourceResolvedDamage *= resolveCriticalMultiplier();
				wasCritical = true;
			}
			else if (payload.criticalPolicy == DamageCriticalPolicy::Random)
			{
				if (auto* mutableSourceCombatant = source ? dynamic_cast<Combatant*>(source) : nullptr)
				{
					const float criticalChance = mutableSourceCombatant->GetCombatRuntime().GetCriticalChance();
					if (RandRange(0.f, 1.f) < criticalChance)
					{
						sourceResolvedDamage *= resolveCriticalMultiplier();
						wasCritical = true;
					}
				}
			}
		}
		float resolvedDamage = 0.f;
		if (std::isfinite(sourceResolvedDamage) && sourceResolvedDamage > 0.0L)
		{
			const long double maximum = static_cast<long double>(std::numeric_limits<float>::max());
			resolvedDamage = sourceResolvedDamage >= maximum
				? std::numeric_limits<float>::max()
				: static_cast<float>(sourceResolvedDamage);
		}
		if (payload.roundDamageUp)
		{
			resolvedDamage = std::min(std::ceil(resolvedDamage), std::numeric_limits<float>::max());
		}

		struct BonusHit
		{
			float damage = 0.f;
			List<GameplayTag> damageTags;
			DamagePayload payload;
		};
		List<BonusHit> bonusHits;
		for (const CombatRuntime::OutgoingDamageBonus& bonus : outgoingModulePlan.bonuses)
		{
			const long double bonusRawDamage = static_cast<long double>(damage) * outgoingModulePlan.damageMultiplier * bonus.fraction;
			const float bonusDamage = std::isfinite(bonusRawDamage) && bonusRawDamage > 0.f
				? static_cast<float>(std::min(bonusRawDamage, static_cast<long double>(std::numeric_limits<float>::max())))
				: 0.f;
			if (bonusDamage <= 0.f) continue;
			BonusHit hit;
			hit.damage = bonusDamage;
			hit.damageTags = { bonus.damageType };
			hit.payload = DamageTypeSystem::BuildPayload(hit.damageTags);
			hit.payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			hit.payload.skipModuleDamageRules = true;
			bonusHits.push_back(std::move(hit));
		}
		const sas::ContentId bonusSourceAbilityId = bonusHits.empty() ? sas::ContentId{} : sourceAbilityId;
		const List<GameplayTag> bonusSourceAbilityTags = bonusHits.empty() ? List<GameplayTag>{} : sourceAbilityTags;
		const std::shared_ptr<Actor> sourceLifetime = bonusHits.empty() || !source
			? std::shared_ptr<Actor>{}
			: source->GetWeakActor().lock();
		const std::shared_ptr<Actor> targetLifetime = bonusHits.empty()
			? std::shared_ptr<Actor>{}
			: target.GetWeakActor().lock();
		const std::shared_ptr<Actor> deliveryActorLifetime = bonusHits.empty() || !deliveryActor
			? std::shared_ptr<Actor>{}
			: deliveryActor->GetWeakActor().lock();

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
		}
		else
		{
			target.ApplyDamage(resolvedDamage);
		}

		if (bonusHits.empty() || target.GetIsPendingDestroy() || (source && source->GetIsPendingDestroy())) return;
		if (const HealthComponent* health = target.FindComponent<HealthComponent>(); health && health->GetHealth() <= 0.f) return;
		if (source)
		{
			if (const HealthComponent* health = source->FindComponent<HealthComponent>(); health && health->GetHealth() <= 0.f) return;
		}
		for (const BonusHit& hit : bonusHits)
		{
			Actor* const bonusSource = sourceLifetime ? sourceLifetime.get() : source;
			Actor* const bonusDeliveryActor = deliveryActorLifetime ? deliveryActorLifetime.get() : deliveryActor;
			Actor& bonusTarget = targetLifetime ? *targetLifetime : target;
			if (bonusSource && bonusSource->GetIsPendingDestroy()) break;
			ApplyCombatDamage(
				bonusTarget,
				hit.damage,
				bonusSource,
				hit.damageTags,
				hit.payload,
				bonusSourceAbilityId,
				bonusSourceAbilityTags,
				deliveryType,
				bonusDeliveryActor
			);
			if (bonusTarget.GetIsPendingDestroy() || (bonusSource && bonusSource->GetIsPendingDestroy())) break;
			if (const HealthComponent* health = bonusTarget.FindComponent<HealthComponent>(); health && health->GetHealth() <= 0.f) break;
		}
	}
}
