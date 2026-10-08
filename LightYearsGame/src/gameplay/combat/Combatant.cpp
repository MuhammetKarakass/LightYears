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
	namespace
	{
		struct BonusHit
		{
			float damage = 0.f;
			List<GameplayTag> damageTags;
			DamagePayload payload;
		};

		struct BonusHitBatch
		{
			List<BonusHit> hits;
			sas::ContentId sourceAbilityId;
			List<GameplayTag> sourceAbilityTags;
			std::shared_ptr<Actor> sourceLifetime;
			std::shared_ptr<Actor> targetLifetime;
			std::shared_ptr<Actor> deliveryActorLifetime;
		};

		BonusHitBatch BuildBonusHitBatch(
			float damage,
			const CombatRuntime::OutgoingDamagePlan& plan,
			Actor& target,
			Actor* source,
			Actor* deliveryActor,
			const sas::ContentId& sourceAbilityId,
			const List<GameplayTag>& sourceAbilityTags
		)
		{
			BonusHitBatch batch;
			for (const CombatRuntime::OutgoingDamageBonus& bonus : plan.bonuses)
			{
				const long double bonusRawDamage = static_cast<long double>(damage) * plan.damageMultiplier * bonus.fraction;
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
				batch.hits.push_back(std::move(hit));
			}
			if (batch.hits.empty()) return batch;
			batch.sourceAbilityId = sourceAbilityId;
			batch.sourceAbilityTags = sourceAbilityTags;
			batch.sourceLifetime = source ? source->GetWeakActor().lock() : std::shared_ptr<Actor>{};
			batch.targetLifetime = target.GetWeakActor().lock();
			batch.deliveryActorLifetime = deliveryActor ? deliveryActor->GetWeakActor().lock() : std::shared_ptr<Actor>{};
			return batch;
		}

		bool IsDefeated(const Actor& actor)
		{
			const HealthComponent* health = actor.FindComponent<HealthComponent>();
			return actor.GetIsPendingDestroy() || (health && health->GetHealth() <= 0.f);
		}

		// Bonus hits run after the main hit and stop as soon as either side is defeated.
		// The batch keeps both actors alive across the nested damage calls.
		void ApplyBonusHitBatch(
			const BonusHitBatch& batch,
			Actor& target,
			Actor* source,
			DamageDeliveryType deliveryType,
			Actor* deliveryActor
		)
		{
			if (batch.hits.empty() || IsDefeated(target) || (source && IsDefeated(*source))) return;
			Actor* const bonusSource = batch.sourceLifetime ? batch.sourceLifetime.get() : source;
			Actor* const bonusDeliveryActor = batch.deliveryActorLifetime ? batch.deliveryActorLifetime.get() : deliveryActor;
			Actor& bonusTarget = batch.targetLifetime ? *batch.targetLifetime : target;
			for (const BonusHit& hit : batch.hits)
			{
				if (bonusSource && bonusSource->GetIsPendingDestroy()) break;
				ApplyCombatDamage(bonusTarget, hit.damage, bonusSource, hit.damageTags, hit.payload,
					batch.sourceAbilityId, batch.sourceAbilityTags, deliveryType, bonusDeliveryActor);
				if (IsDefeated(bonusTarget) || (bonusSource && bonusSource->GetIsPendingDestroy())) break;
			}
		}
	}

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

	void ApplyCombatDamage(Actor& target, float damage, Actor* source, const List<GameplayTag>& damageTags, const DamagePayload& payload, const sas::ContentId& sourceAbilityId, const List<GameplayTag>& sourceAbilityTags, DamageDeliveryType deliveryType, Actor* deliveryActor)
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
			? sourceCombatant->GetCombatRuntime().ResolveOutgoingDamagePlan(target, sourceAbilityTags, sourceAbilityId, deliveryType, payload.moduleTriggerPolicy != DamageModuleTriggerPolicy::Suppressed)
			: CombatRuntime::OutgoingDamagePlan{};
		const auto resolveCriticalMultiplier = [&]()
		{
			return sourceCombatant ? sourceCombatant->GetCombatRuntime().GetCriticalDamageMultiplier() : 1.f;
		};
		const float sourceOutgoingDamageMultiplier =
			sourceCombatant && payload.sourceScalingPolicy == DamageSourceScalingPolicy::Normal ? sourceCombatant->GetCombatRuntime().GetOutgoingDamageMultiplier() : 1.f;
		long double sourceResolvedDamage = (static_cast<long double>(damage) + outgoingModulePlan.additionalDamage) * outgoingModulePlan.damageMultiplier * sourceOutgoingDamageMultiplier;
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

		BonusHitBatch bonusHits = BuildBonusHitBatch(
			damage, outgoingModulePlan, target, source, deliveryActor, sourceAbilityId, sourceAbilityTags
		);

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
			context.moduleFirstHitClaim = outgoingModulePlan.firstHitClaim;
			if (!context.payload.hitGroup && sourceCombatant && !sourceAbilityId.ToString().empty())
				if (const auto* ability = sourceCombatant->GetAbilitySystemComponent().GetAbilityById(sourceAbilityId.ToString())) context.payload.hitGroup = ability->GetDamageHitGroup();
			combatant->ReceiveDamage(context);
		}
		else
		{
			target.ApplyDamage(resolvedDamage);
		}

		ApplyBonusHitBatch(bonusHits, target, source, deliveryType, deliveryActor);
	}
}
