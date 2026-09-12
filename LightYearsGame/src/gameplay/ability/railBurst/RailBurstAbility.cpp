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

		bool HasExpectedModifier(
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

		if (!actor || !damage || !speed || !range || !collisionRadius ||
			*damage <= 0.f || *speed <= 0.f || *range <= 0.f ||
			*collisionRadius <= 0.f ||
			actor->spawnDistance < 0.f || actor->lifeTime <= *range / *speed)
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst actor data requires positive delivery values and cleanup time beyond range.";
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

		const auto hasDamageScaling = [&](const sas::AttributeId& sourceAttributeId,
			float coefficient)
		{
			return std::any_of(
				definition.scalingRules.begin(),
				definition.scalingRules.end(),
				[&](const sas::AttributeScalingRule& rule)
				{
					return rule.targetAttributeId == CommonAttributeIds::Damage &&
						rule.sourceAttributeId == sourceAttributeId &&
						rule.operation == sas::AttributeModifierOperation::Add &&
						std::abs(rule.coefficient - coefficient) <= 0.0001f;
				}
			);
		};
		if (definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Energy ||
			definition.scalingRules.size() != 2 ||
			!hasDamageScaling(OwnerAttributeIds::AttackPower, 1.50f) ||
			!hasDamageScaling(OwnerAttributeIds::EnergyPower, 0.20f))
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst requires Energy damage plus additive AttackPower and EnergyPower scaling.";
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

		const std::optional<float> damagePerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Damage
		);
		const std::optional<float> cooldownReductionPerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Cooldown
		);
		if (!damagePerLevel || !cooldownReductionPerLevel ||
			*damagePerLevel <= 0.f || *cooldownReductionPerLevel >= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Rail Burst progression must add damage and reduce cooldown.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 2 ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, *damagePerLevel) ||
				!HasExpectedModifier(
					step,
					CommonAttributeIds::Cooldown,
					*cooldownReductionPerLevel
				))
			{
				if (failureReason)
				{
					*failureReason =
						"Rail Burst progression may only increase damage and reduce cooldown.";
				}
				return false;
			}

			resolvedCooldown += *cooldownReductionPerLevel;
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
