#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"

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

		bool HasModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float magnitude
		)
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
			const sas::AttributeId& attributeId
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

		std::optional<float> FindScalingCoefficient(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& targetAttributeId
		)
		{
			for (const sas::AttributeScalingRule& rule : definition.scalingRules)
			{
				if (rule.targetAttributeId == targetAttributeId &&
					rule.sourceAttributeId == OwnerAttributeIds::MaxHealth &&
					rule.operation == sas::AttributeModifierOperation::Add)
				{
					return rule.coefficient;
				}
			}
			return std::nullopt;
		}

		bool HasProjectileSpawnAction(const GameAbilityDefinition& definition)
		{
			for (const AbilityActionSpec& action : definition.actions)
			{
				const SpawnActorAction* spawn = std::get_if<SpawnActorAction>(&action.action);
				if (action.phase == sas::AbilityActionPhase::OnActivate && spawn &&
					spawn->actorDefinitionId ==
						AbilityData::GravityAnomaly::ActorProjectileBasic.actorDefinitionId &&
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

	bool GravityAnomalyAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* projectile = AbilityData::FindAbilityActorDefinition(
			AbilityData::GravityAnomaly::ActorProjectileBasic.actorDefinitionId.ToString()
		);
		const AbilityActorDefinition* field = AbilityData::FindAbilityActorDefinition(
			AbilityData::GravityAnomaly::ActorFieldBasic.actorDefinitionId.ToString()
		);
		const std::optional<float> castRange = projectile
			? FindActorAttribute(*projectile, CommonAttributeIds::Range)
			: std::nullopt;
		const std::optional<float> projectileSpeed = projectile
			? FindActorAttribute(*projectile, AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed)
			: std::nullopt;
		const std::optional<float> baseDuration = field
			? FindActorAttribute(*field, CommonAttributeIds::Duration)
			: std::nullopt;
		const std::optional<float> baseRadius = field
			? FindActorAttribute(*field, CommonAttributeIds::Radius)
			: std::nullopt;
		const std::optional<float> pullStrength = field
			? FindActorAttribute(*field, AbilityData::GravityAnomaly::Actor::Field::PullStrength)
			: std::nullopt;
		const std::optional<float> slowMagnitude = field
			? FindActorAttribute(*field, AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude)
			: std::nullopt;
		const std::optional<float> radiusPerMaxHealth = FindScalingCoefficient(
			definition,
			CommonAttributeIds::Radius
		);
		const std::optional<float> durationPerMaxHealth = FindScalingCoefficient(
			definition,
			CommonAttributeIds::Duration
		);
		if (!projectile || !field || !castRange || !projectileSpeed ||
			!baseDuration || !baseRadius || !pullStrength || !slowMagnitude ||
			!radiusPerMaxHealth || !durationPerMaxHealth || definition.cooldown <= 0.f ||
			definition.maxCharges != 1 || *castRange <= 0.f ||
			*projectileSpeed <= 0.f || *baseDuration <= 0.f || *baseRadius <= 0.f ||
			*pullStrength <= 0.f || *slowMagnitude <= 0.f || *slowMagnitude >= 1.f ||
			*radiusPerMaxHealth < 0.f || *durationPerMaxHealth < 0.f ||
			projectile->spawnDistance < 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Gravity Anomaly settings require valid delivery, field, and scaling values.";
			}
			return false;
		}

		if (definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != 1 ||
			definition.cooldown <= 0.f ||
			!definition.damageTags.empty() || !HasProjectileSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason = "Gravity Anomaly requires one instant, non-damaging cursor projectile spawn.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Gravity Anomaly requires fourteen normal progression steps through level fifteen.";
			}
			return false;
		}

		const AbilityLevelStep& firstStep = definition.levelProgression.front();
		const std::optional<float> cooldownReductionPerLevel = FindModifierMagnitude(
			firstStep,
			CommonAttributeIds::Cooldown
		);
		const std::optional<float> durationPerLevel = FindModifierMagnitude(
			firstStep,
			CommonAttributeIds::Duration
		);
		const std::optional<float> radiusPerLevel = FindModifierMagnitude(
			firstStep,
			CommonAttributeIds::Radius
		);
		const std::optional<float> pullStrengthPerLevel = FindModifierMagnitude(
			firstStep,
			AbilityData::GravityAnomaly::Actor::Field::PullStrength
		);
		const std::optional<float> slowMagnitudePerLevel = FindModifierMagnitude(
			firstStep,
			AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude
		);
		const std::optional<float> projectileSpeedPerLevel = FindModifierMagnitude(
			firstStep,
			AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed
		);
		const std::optional<float> castRangePerLevel = FindModifierMagnitude(
			firstStep,
			CommonAttributeIds::Range
		);
		if (!cooldownReductionPerLevel || !durationPerLevel || !radiusPerLevel ||
			!pullStrengthPerLevel || !slowMagnitudePerLevel ||
			!projectileSpeedPerLevel || !castRangePerLevel ||
			*cooldownReductionPerLevel >= 0.f || *durationPerLevel <= 0.f ||
			*radiusPerLevel <= 0.f || *pullStrengthPerLevel <= 0.f ||
			*slowMagnitudePerLevel <= 0.f || *projectileSpeedPerLevel <= 0.f ||
			*castRangePerLevel <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Gravity Anomaly progression must contain valid delivery, field, and cooldown increments.";
			}
			return false;
		}

		float cooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 7 ||
				!HasModifier(step, CommonAttributeIds::Cooldown, *cooldownReductionPerLevel) ||
				!HasModifier(step, CommonAttributeIds::Duration, *durationPerLevel) ||
				!HasModifier(step, CommonAttributeIds::Radius, *radiusPerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::Actor::Field::PullStrength, *pullStrengthPerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude, *slowMagnitudePerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed, *projectileSpeedPerLevel) ||
				!HasModifier(step, CommonAttributeIds::Range, *castRangePerLevel))
			{
				if (failureReason)
				{
					*failureReason = "Gravity Anomaly progression must contain exactly the configured incremental values.";
				}
				return false;
			}
			cooldown += *cooldownReductionPerLevel;
			if (cooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Gravity Anomaly progression must keep cooldown positive.";
				}
				return false;
			}
		}

		return true;
	}
}
