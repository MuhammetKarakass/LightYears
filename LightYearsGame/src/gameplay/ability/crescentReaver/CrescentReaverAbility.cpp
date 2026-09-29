#include "gameplay/ability/crescentReaver/CrescentReaverAbility.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/crescentReaver/CrescentReaverContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"

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

		bool IsPositiveAdditiveScalingRule(
			const sas::AttributeScalingRule& rule,
			const sas::AttributeId& targetAttributeId,
			const sas::AttributeId& sourceAttributeId
		)
		{
			return rule.targetAttributeId == targetAttributeId &&
				rule.sourceAttributeId == sourceAttributeId &&
				rule.operation == sas::AttributeModifierOperation::Add &&
				std::isfinite(rule.coefficient) && rule.coefficient > 0.f;
		}

		bool HasValidProgressionStep(const AbilityLevelStep& step)
		{
			int damageModifiers = 0;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Damage &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::isfinite(modifier.magnitude) && modifier.magnitude > 0.f)
				{
					++damageModifiers;
				}
				else
				{
					return false;
				}
			}
			return damageModifiers == 1 &&
				step.scalingRules.size() == 1 &&
				IsPositiveAdditiveScalingRule(
					step.scalingRules.front(),
					CommonAttributeIds::Damage,
					OwnerAttributeIds::AttackPower
				);
		}

		const sas::AttributeScalingRule* FindScalingRule(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& targetAttributeId
		)
		{
			for (const sas::AttributeScalingRule& rule : definition.scalingRules)
			{
				if (rule.targetAttributeId == targetAttributeId)
				{
					return &rule;
				}
			}
			return nullptr;
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
						AbilityData::CrescentReaver::Actor::Projectile::BasicDefinitionId &&
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

	bool CrescentReaverAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* actor = AbilityData::FindAbilityActorDefinition(
			AbilityData::CrescentReaver::Actor::Projectile::BasicDefinitionId
		);
		const std::optional<float> damage = actor
			? FindActorAttribute(*actor, CommonAttributeIds::Damage)
			: std::nullopt;
		const std::optional<float> speed = actor
			? FindActorAttribute(
				*actor,
				AbilityData::CrescentReaver::Actor::Projectile::ProjectileSpeed
			)
			: std::nullopt;
		const std::optional<float> bounceCount = actor
			? FindActorAttribute(
				*actor,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCount
			)
			: std::nullopt;
		const std::optional<float> damageGrowth = actor
			? FindActorAttribute(
				*actor,
				AbilityData::CrescentReaver::Actor::Projectile::BounceDamageGrowth
			)
			: std::nullopt;
		const std::optional<float> collisionRadius = actor
			? FindActorAttribute(*actor, CollisionAttributeIds::Radius)
			: std::nullopt;

		if (!actor || !damage || !speed || !bounceCount || !damageGrowth ||
			!collisionRadius || *damage <= 0.f ||
			*speed <= 0.f || *bounceCount < 0.f ||
			std::round(*bounceCount) != *bounceCount || *damageGrowth < 0.f ||
			*collisionRadius <= 0.f ||
			actor->spawnDistance < 0.f || actor->lifeTime != 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Crescent Reaver actor data requires positive speed/damage/collision values, integer bounce count, and no gameplay lifetime.";
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
					"Crescent Reaver requires one instant, owner-forward cursor-aimed projectile spawn action and one charge.";
			}
			return false;
		}

		const sas::AttributeScalingRule* damageScaling = FindScalingRule(
			definition,
			CommonAttributeIds::Damage
		);
		const sas::AttributeScalingRule* luckScaling = FindScalingRule(
			definition,
			AbilityData::CrescentReaver::Actor::Projectile::BounceCount
		);
		if (definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Kinetic ||
			definition.scalingRules.size() != 2 || !damageScaling || !luckScaling ||
			!IsPositiveAdditiveScalingRule(
				*damageScaling,
				CommonAttributeIds::Damage,
				OwnerAttributeIds::AttackPower
			) ||
			!IsPositiveAdditiveScalingRule(
				*luckScaling,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCount,
				OwnerAttributeIds::Luck
			))
		{
			if (failureReason)
			{
				*failureReason =
					"Crescent Reaver requires Kinetic damage, positive additive AttackPower damage scaling, and positive additive Luck bounce scaling.";
			}
			return false;
		}

		if (definition.levelProgression.empty() && definition.repeatingLevelProgression.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Crescent Reaver requires at least one progression step.";
			}
			return false;
		}

		List<AbilityLevelStep> levelSteps = definition.levelProgression;
		levelSteps.insert(levelSteps.end(), definition.repeatingLevelProgression.begin(),
			definition.repeatingLevelProgression.end());
		for (const AbilityLevelStep& step : levelSteps)
		{
			if (!HasValidProgressionStep(step))
			{
				if (failureReason)
				{
					*failureReason =
						"Crescent Reaver progression must add damage and AttackPower scaling.";
				}
				return false;
			}
		}

		return true;
	}
}
