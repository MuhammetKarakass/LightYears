#include "gameplay/ability/railBurst/RailBurstAbility.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/railBurst/RailBurstContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <variant>

namespace ly
{
	namespace
	{
		std::optional<float> FindActorAttribute(
			const AbilityActorDefinition& actor,
			const sas::AttributeId& attributeId
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				actor.attributes,
				attributeId
			);
			return attribute ? std::optional<float>{ attribute->baseValue } : std::nullopt;
		}

		bool IsEnergyPowerDamageRule(const sas::AttributeScalingRule& rule)
		{
			return rule.targetAttributeId == CommonAttributeIds::Damage &&
				rule.sourceAttributeId == OwnerAttributeIds::EnergyPower &&
				rule.operation == sas::AttributeModifierOperation::Add &&
				std::isfinite(rule.coefficient) && rule.coefficient > 0.f;
		}

		bool HasValidProgressionStep(const AbilityLevelStep& step, float& cooldownDelta)
		{
			int damageModifiers = 0;
			int cooldownModifiers = 0;
			cooldownDelta = 0.f;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Damage &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::isfinite(modifier.magnitude) && modifier.magnitude > 0.f)
				{
					++damageModifiers;
				}
				else if (modifier.attributeId == CommonAttributeIds::Cooldown &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::isfinite(modifier.magnitude) && modifier.magnitude <= 0.f)
				{
					++cooldownModifiers;
					cooldownDelta += modifier.magnitude;
				}
				else
				{
					return false;
				}
			}
			return damageModifiers == 1 && cooldownModifiers <= 1 &&
				step.scalingRules.size() == 1 &&
				IsEnergyPowerDamageRule(step.scalingRules.front());
		}

		bool HasBasicSpawnAction(const GameAbilityDefinition& definition)
		{
			for (const AbilityActionSpec& action : definition.actions)
			{
				const SpawnActorAction* spawn = std::get_if<SpawnActorAction>(
					&action.action
				);
				if (action.phase == sas::AbilityActionPhase::OnActivate &&
					spawn &&
					spawn->actorDefinitionId ==
						AbilityData::RailBurst::Actor::Projectile::BasicDefinitionId &&
					spawn->spawnPolicy == sas::AbilitySpawnPolicy::OwnerForward &&
					spawn->directionPolicy == sas::AbilityDirectionPolicy::OwnerForward &&
					action.maxExecutions == 1)
				{
					return true;
				}
			}
			return false;
		}
	}

	bool RailBurstAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* actor = AbilityData::FindAbilityActorDefinition(
			AbilityData::RailBurst::Actor::Projectile::BasicDefinitionId
		);
		const std::optional<float> damage = actor
			? FindActorAttribute(*actor, CommonAttributeIds::Damage)
			: std::nullopt;
		const std::optional<float> speed = actor
			? FindActorAttribute(
				*actor,
				AbilityData::RailBurst::Actor::Projectile::ProjectileSpeed
			)
			: std::nullopt;
		const std::optional<float> range = actor
			? FindActorAttribute(*actor, CommonAttributeIds::Range)
			: std::nullopt;
		const std::optional<float> collisionRadius = actor
			? FindActorAttribute(*actor, CollisionAttributeIds::Radius)
			: std::nullopt;
		const std::optional<float> pierceLoss = actor
			? FindActorAttribute(*actor, CommonAttributeIds::PierceDamageLoss)
			: std::nullopt;
		const std::optional<float> minimumDamageMultiplier = actor
			? FindActorAttribute(
				*actor,
				AbilityData::RailBurst::Actor::Projectile::MinimumDamageMultiplier
			)
			: std::nullopt;

		if (!actor || !damage || !speed || !range || !collisionRadius ||
			!pierceLoss || !minimumDamageMultiplier ||
			*damage <= 0.f || *speed <= 0.f || *range <= 0.f ||
			*collisionRadius <= 0.f ||
			!std::isfinite(*pierceLoss) || *pierceLoss < 0.f || *pierceLoss >= 1.f ||
			!std::isfinite(*minimumDamageMultiplier) ||
			*minimumDamageMultiplier <= 0.f || *minimumDamageMultiplier > 1.f ||
			actor->spawnDistance < 0.f || actor->lifeTime <= *range / *speed)
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst actor data requires positive delivery values, valid pierce falloff, and cleanup time beyond range.";
			}
			return false;
		}

		if (definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			!HasBasicSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst requires one instant, owner-forward projectile spawn action and one charge.";
			}
			return false;
		}

		if (definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Energy ||
			definition.scalingRules.size() != 1 ||
			!IsEnergyPowerDamageRule(definition.scalingRules.front()))
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst requires Energy damage and additive EnergyPower scaling without AttackPower scaling.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst requires fourteen progression steps through level fifteen.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			float cooldownDelta = 0.f;
			if (!HasValidProgressionStep(step, cooldownDelta))
			{
				if (failureReason)
				{
					*failureReason =
						"Rail Burst progression must add damage and EnergyPower scaling; cooldown may only stay or decrease.";
				}
				return false;
			}

			resolvedCooldown += cooldownDelta;
			if (resolvedCooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason =
						"Rail Burst progression must keep cooldown positive at every level.";
				}
				return false;
			}
		}

		return true;
	}
}
