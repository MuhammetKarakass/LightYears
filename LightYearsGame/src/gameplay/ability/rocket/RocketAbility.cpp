#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/rocket/RocketAbility.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include "gameConfigs/ability/offensive/RocketConfig.h"

#include <cmath>
#include <optional>
#include <variant>

namespace ly
{
	namespace
	{
		std::optional<float> FindActorAttribute(
			const AbilityActorDefinition& actor,
			const GameplayTag& attributeId
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(
				actor.attributes,
				attributeId
			);
			return attribute ? std::optional<float>{ attribute->baseValue } : std::nullopt;
		}

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const GameplayTag& attributeId,
			float magnitude)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::abs(modifier.magnitude - magnitude) <= 0.0001f)
				{
					return true;
				}
			}
			return false;
		}

		std::optional<float> FindModifierMagnitude(
			const AbilityLevelStep& step,
			const GameplayTag& attributeId
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add)
				{
					return modifier.magnitude;
				}
			}
			return std::nullopt;
		}

		bool HasBasicRocketSpawnAction(const GameAbilityDefinition& definition)
		{
			for (const AbilityActionSpec& action : definition.actions)
			{
				const SpawnActorAction* spawn = std::get_if<SpawnActorAction>(&action.action);
				if (action.phase == sas::AbilityActionPhase::OnActivate && spawn &&
					spawn->actorDefinitionId == AbilityData::Rocket::ActorProjectileBasic.actorDefinitionId &&
					spawn->spawnPolicy == sas::AbilitySpawnPolicy::OwnerForward &&
					spawn->directionPolicy == sas::AbilityDirectionPolicy::MouseWorld &&
					action.maxExecutions == 1)
				{
					return true;
				}
			}
			return false;
		}
	}

	bool RocketAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		const AbilityActorDefinition* actor = AbilityData::FindAbilityActorDefinition(
			AbilityData::Rocket::ActorProjectileBasic.actorDefinitionId
		);
		const std::optional<float> baseDamage = actor
			? FindActorAttribute(*actor, CommonAttributeIds::Damage)
			: std::nullopt;
		const std::optional<float> projectileSpeed = actor
			? FindActorAttribute(*actor, AbilityData::Rocket::ActorSchema::ProjectileSpeed)
			: std::nullopt;
		const std::optional<float> range = actor
			? FindActorAttribute(*actor, CommonAttributeIds::Range)
			: std::nullopt;
		const std::optional<float> explosionRadius = actor
			? FindActorAttribute(*actor, CommonAttributeIds::Radius)
			: std::nullopt;
		const std::optional<float> collisionRadius = actor
			? FindActorAttribute(*actor, CommonAttributeIds::CollisionRadius)
			: std::nullopt;
		if (!actor || !baseDamage || !projectileSpeed || !range ||
			!explosionRadius || !collisionRadius || definition.cooldown <= 0.f ||
			*baseDamage <= 0.f || *projectileSpeed <= 0.f || *range <= 0.f ||
			*explosionRadius <= 0.f || *collisionRadius <= 0.f ||
			actor->spawnDistance < 0.f || actor->lifeTime <= *range / *projectileSpeed)
		{
			if (failureReason)
			{
				*failureReason = "Rocket actor data requires positive delivery values and cleanup time.";
			}
			return false;
		}

		if (definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 ||
			definition.cooldown <= 0.f ||
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
			definition.scalingRules.front().operation != sas::AttributeModifierOperation::Add ||
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

		const std::optional<float> damagePerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Damage
		);
		const std::optional<float> cooldownReductionPerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Cooldown
		);
		const std::optional<float> explosionRadiusPerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Radius
		);
		if (!damagePerLevel || !cooldownReductionPerLevel ||
			!explosionRadiusPerLevel || *damagePerLevel <= 0.f ||
			*cooldownReductionPerLevel >= 0.f || *explosionRadiusPerLevel <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Rocket progression must contain positive damage and radius growth with cooldown reduction.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 3 ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, *damagePerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Cooldown, *cooldownReductionPerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Radius, *explosionRadiusPerLevel))
			{
				if (failureReason)
				{
					*failureReason = "Rocket progression may only increase damage and explosion radius while reducing cooldown.";
				}
				return false;
			}

			resolvedCooldown += *cooldownReductionPerLevel;
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
