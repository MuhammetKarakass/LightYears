#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h"

#include "gameConfigs/ability/GravityAnomalyConfig.h"

#include <cmath>
#include <variant>

namespace ly
{
	namespace
	{
		bool HasModifier(
			const AbilityLevelStep& step,
			const GameplayTag& attributeId,
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
		const AbilityData::GravityAnomaly::Settings* settings =
			AbilityData::GravityAnomaly::FindSettings(definition.abilityId);
		if (!settings || settings->cooldown <= 0.f || settings->chargeCount != 1 ||
			settings->castRange <= 0.f || settings->projectileSpeed <= 0.f ||
			settings->baseDuration <= 0.f || settings->baseRadius <= 0.f ||
			settings->pullStrength <= 0.f || settings->slowMagnitude <= 0.f ||
			settings->slowMagnitude >= 1.f || settings->radiusPerMaxHealth < 0.f ||
			settings->durationPerMaxHealth < 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Gravity Anomaly settings require valid delivery, field, and scaling values.";
			}
			return false;
		}

		if (definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
			definition.maxCharges != settings->chargeCount ||
			std::abs(definition.cooldown - settings->cooldown) > 0.0001f ||
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

		float cooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 7 ||
				!HasModifier(step, CommonAttributeIds::Cooldown, -settings->cooldownReductionPerLevel) ||
				!HasModifier(step, CommonAttributeIds::Duration, settings->durationPerLevel) ||
				!HasModifier(step, CommonAttributeIds::Radius, settings->radiusPerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::ActorSchema::PullStrength, settings->pullStrengthPerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::ActorSchema::SlowMagnitude, settings->slowMagnitudePerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::ActorSchema::ProjectileSpeed, settings->projectileSpeedPerLevel) ||
				!HasModifier(step, AbilityData::GravityAnomaly::ActorSchema::CastRange, settings->castRangePerLevel))
			{
				if (failureReason)
				{
					*failureReason = "Gravity Anomaly progression must contain exactly the configured incremental values.";
				}
				return false;
			}
			cooldown -= settings->cooldownReductionPerLevel;
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
