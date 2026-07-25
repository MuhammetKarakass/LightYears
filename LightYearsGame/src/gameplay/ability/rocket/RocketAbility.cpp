#include "gameplay/ability/rocket/RocketAbility.h"

#include "gameConfigs/ability/RocketConfig.h"

#include <cmath>
#include <variant>

namespace ly
{
	namespace
	{
		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const GameplayTag& attributeId,
			float magnitude)
		{
			for (const AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == AttributeModifierOperation::Add &&
					std::abs(modifier.magnitude - magnitude) <= 0.0001f)
				{
					return true;
				}
			}
			return false;
		}

		bool HasBasicRocketSpawnAction(const AbilityDefinition& definition)
		{
			for (const AbilityActionSpec& action : definition.actions)
			{
				const SpawnActorAction* spawn = std::get_if<SpawnActorAction>(&action.action);
				if (action.phase == AbilityActionPhase::OnActivate && spawn &&
					spawn->actorDefinitionId == AbilityData::Rocket::ActorProjectileBasic.actorDefinitionId &&
					spawn->spawnPolicy == AbilitySpawnPolicy::OwnerForward &&
					spawn->directionPolicy == AbilityDirectionPolicy::MouseWorld &&
					action.maxExecutions == 1)
				{
					return true;
				}
			}
			return false;
		}
	}

	bool RocketAbility::Validate(
		const AbilityDefinition& definition,
		std::string* failureReason) const
	{
		const AbilityData::Rocket::Settings* settings =
			AbilityData::Rocket::FindSettings(definition.abilityId);
		if (!settings || settings->baseDamage <= 0.f || settings->cooldown <= 0.f ||
			settings->projectileSpeed <= 0.f || settings->range <= 0.f ||
			settings->explosionRadius <= 0.f || settings->projectileCount != 1 ||
			settings->collisionRadius <= 0.f || settings->spawnDistance < 0.f ||
			settings->cleanupGraceDuration <= 0.f || settings->damagePerLevel <= 0.f ||
			settings->cooldownReductionPerLevel <= 0.f || settings->explosionRadiusPerLevel <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Rocket settings require positive values and exactly one projectile.";
			}
			return false;
		}

		if (definition.activationPolicy != AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 ||
			std::abs(definition.cooldown - settings->cooldown) > 0.0001f ||
			!HasBasicRocketSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason = "Basic Rocket requires one instant, cursor-aimed projectile spawn action.";
			}
			return false;
		}

		if (definition.damageTags.size() != 1 || definition.damageTags.front() != DamageTypeSchema::Kinetic ||
			definition.scalingRules.size() != 1 ||
			definition.scalingRules.front().targetAttributeId != CommonAttributeIds::Damage ||
			definition.scalingRules.front().sourceAttributeId != OwnerAttributeIds::AttackPower ||
			definition.scalingRules.front().operation != AttributeModifierOperation::Add ||
			std::abs(definition.scalingRules.front().coefficient - 1.25f) > 0.0001f)
		{
			if (failureReason)
			{
				*failureReason = "Basic Rocket requires Kinetic damage and AttackPower x1.25 additive scaling.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Basic Rocket requires fourteen normal progression steps through level fifteen.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 3 ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, settings->damagePerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Cooldown, -settings->cooldownReductionPerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Radius, settings->explosionRadiusPerLevel))
			{
				if (failureReason)
				{
					*failureReason = "Rocket progression may only increase damage and explosion radius while reducing cooldown.";
				}
				return false;
			}

			resolvedCooldown -= settings->cooldownReductionPerLevel;
			if (resolvedCooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Rocket progression must keep cooldown positive at every level.";
				}
				return false;
			}
		}

		return true;
	}
}
