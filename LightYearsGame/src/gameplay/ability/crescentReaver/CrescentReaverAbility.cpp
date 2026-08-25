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
		const std::optional<float> cooldownReduction = actor
			? FindActorAttribute(
				*actor,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCooldownReduction
			)
			: std::nullopt;
		const std::optional<float> collisionRadius = actor
			? FindActorAttribute(*actor, CollisionAttributeIds::Radius)
			: std::nullopt;

		if (!actor || !damage || !speed || !bounceCount || !damageGrowth ||
			!cooldownReduction || !collisionRadius || *damage <= 0.f ||
			*speed <= 0.f || *bounceCount < 0.f ||
			std::round(*bounceCount) != *bounceCount || *damageGrowth < 0.f ||
			*cooldownReduction < 0.f || *collisionRadius <= 0.f ||
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
			damageScaling->sourceAttributeId != OwnerAttributeIds::AttackPower ||
			damageScaling->operation != sas::AttributeModifierOperation::Add ||
			std::abs(damageScaling->coefficient - 0.80f) > 0.0001f ||
			luckScaling->sourceAttributeId != OwnerAttributeIds::Luck ||
			luckScaling->operation != sas::AttributeModifierOperation::Add ||
			std::abs(luckScaling->coefficient - 0.05f) > 0.0001f)
		{
			if (failureReason)
			{
				*failureReason =
					"Crescent Reaver requires Kinetic damage, AttackPower x0.80 damage scaling, and Luck x0.05 bounce scaling.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Crescent Reaver requires fourteen progression steps through level fifteen.";
			}
			return false;
		}

		const std::optional<float> damagePerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Damage
		);
		const std::optional<float> cooldownPerLevel = FindModifierMagnitude(
			definition.levelProgression.front(),
			CommonAttributeIds::Cooldown
		);
		if (!damagePerLevel || !cooldownPerLevel || *damagePerLevel <= 0.f ||
			*cooldownPerLevel >= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Crescent Reaver progression must add damage and reduce cooldown.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 2 ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, *damagePerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Cooldown, *cooldownPerLevel))
			{
				if (failureReason)
				{
					*failureReason =
						"Crescent Reaver progression may only increase damage and reduce cooldown.";
				}
				return false;
			}
			resolvedCooldown += *cooldownPerLevel;
			if (resolvedCooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason =
						"Crescent Reaver progression must keep cooldown positive at every level.";
				}
				return false;
			}
		}

		return true;
	}
}
