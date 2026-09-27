#include "gameplay/ability/solarBombardment/SolarBombardmentAbility.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "framework/Actor.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			if (definition.abilityTags.size() != 2)
			{
				return false;
			}

			bool hasCategory = false;
			bool hasFamily = false;
			for (const GameplayTag& tag : definition.abilityTags)
			{
				hasCategory = hasCategory || tag.MatchesTagExact(
					AbilityData::SolarBombardment::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::SolarBombardment::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasAdditiveModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			bool positive
		)
		{
			return std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[&](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == attributeId &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						std::isfinite(modifier.magnitude) &&
						(positive ? modifier.magnitude > 0.f : modifier.magnitude < 0.f);
				}
			);
		}

		bool HasValidAttackPowerScaling(
			const List<sas::AttributeScalingRule>& rules,
			bool requirePositive
		)
		{
			return rules.size() == 1 &&
				rules.front().targetAttributeId == CommonAttributeIds::Damage &&
				rules.front().sourceAttributeId == OwnerAttributeIds::AttackPower &&
				rules.front().operation == sas::AttributeModifierOperation::Add &&
				std::isfinite(rules.front().coefficient) &&
				(requirePositive ? rules.front().coefficient > 0.f : rules.front().coefficient >= 0.f);
		}
	}

	bool SolarBombardmentAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* projectile =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::SolarBombardment::Actor::Projectile::BasicDefinitionId
			);
		const bool validIdentity =
			definition.abilityId == AbilityData::SolarBombardment::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::SolarBombardment &&
			HasExactAbilityTags(definition);
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 &&
			NearlyEqual(definition.duration, 0.f) &&
			std::isfinite(definition.cooldown) && definition.cooldown > 0.f;
		const bool validDamage = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Thermal);

		bool validActor = projectile != nullptr;
		if (validActor)
		{
			for (const sas::AttributeId& required : {
				CommonAttributeIds::Damage,
				CommonAttributeIds::Radius,
				CommonAttributeIds::Range,
				AbilityData::SolarBombardment::Attribute::InnerRadius,
				AbilityData::SolarBombardment::Attribute::InnerDamageMultiplier,
				AbilityData::SolarBombardment::Attribute::OuterDamageMultiplier,
				AbilityData::SolarBombardment::Attribute::InnerIgniteStacks,
				AbilityData::SolarBombardment::Attribute::OuterIgniteStacks,
				AbilityData::SolarBombardment::Attribute::MinTravelTime,
				AbilityData::SolarBombardment::Attribute::MaxTravelTime
			})
			{
				const sas::GameplayAttribute* attribute = sas::FindAttribute(
					projectile->attributes,
					required
				);
				if (!attribute || !std::isfinite(attribute->baseValue))
				{
					validActor = false;
					break;
				}
			}
		}

		if (validActor)
		{
			const float outerRadius = sas::FindAttributeValue(
				projectile->attributes, CommonAttributeIds::Radius, 0.f
			);
			const float innerRadius = sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::SolarBombardment::Attribute::InnerRadius,
				0.f
			);
			const float innerMultiplier = sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::SolarBombardment::Attribute::InnerDamageMultiplier,
				0.f
			);
			const float outerMultiplier = sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::SolarBombardment::Attribute::OuterDamageMultiplier,
				0.f
			);
			const float minTravel = sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::SolarBombardment::Attribute::MinTravelTime,
				0.f
			);
			const float maxTravel = sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::SolarBombardment::Attribute::MaxTravelTime,
				0.f
			);
			validActor = outerRadius > innerRadius && innerRadius > 0.f &&
				innerMultiplier > 0.f && outerMultiplier > 0.f && minTravel > 0.f &&
				maxTravel >= minTravel && projectile->lifeTime > maxTravel &&
				sas::FindAttributeValue(
					projectile->attributes,
					CommonAttributeIds::Range,
					0.f
				) > 0.f;
		}

		bool validProgression = !definition.levelProgression.empty() &&
			definition.levelUpgradeScrapCosts.size() == definition.levelProgression.size();
		if (validProgression)
		{
			float resolvedCooldown = definition.cooldown;
			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (step.attributeModifiers.size() != 2 || step.scalingRules.size() != 1 ||
					!step.unlockedUpgradeIds.empty() || !step.addedActions.empty() ||
					!step.addedTriggers.empty() ||
					!HasAdditiveModifier(step, CommonAttributeIds::Damage, true) ||
					!HasAdditiveModifier(step, CommonAttributeIds::Cooldown, false) ||
					!HasValidAttackPowerScaling(step.scalingRules, false))
				{
					validProgression = false;
					break;
				}
				const auto cooldownModifier = std::find_if(
					step.attributeModifiers.begin(), step.attributeModifiers.end(),
					[](const sas::AttributeModifier& modifier)
					{
						return modifier.attributeId == CommonAttributeIds::Cooldown;
					}
				);
				resolvedCooldown += cooldownModifier->magnitude;
				if (!std::isfinite(resolvedCooldown) || resolvedCooldown <= 0.f)
				{
					validProgression = false;
					break;
				}
			}
		}

		const bool validScaling = HasValidAttackPowerScaling(definition.scalingRules, true);

		if (!validIdentity || !validLifecycle || !validDamage || !validActor ||
			!validProgression || !validScaling || !definition.actions.empty())
		{
			if (failureReason)
			{
				*failureReason =
				"Solar Bombardment requires a cursor projectile, two valid explosion zones, Thermal damage, AttackPower scaling, and aligned progression costs.";
			}
			return false;
		}
		return true;
	}

	bool SolarBombardmentAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		const AbilityActorDefinition* projectile =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::SolarBombardment::Actor::Projectile::BasicDefinitionId
			);
		if (!world || !projectile)
		{
			return false;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAttributes(
				executionContext,
				nullptr,
				projectile->attributes,
				context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability)
			);
		const float castRange = std::max(
			0.f,
			sas::FindAttributeValue(values, CommonAttributeIds::Range, 0.f)
		);
		if (castRange <= 0.f)
		{
			return false;
		}

		const sf::Vector2f ownerLocation = context.owner.GetActorLocation();
		sf::Vector2f targetLocation = world->GetMouseWorldPosition();
		sf::Vector2f direction = targetLocation - ownerLocation;
		if (GetVectorLength(direction) <= 0.001f)
		{
			direction = context.owner.GetActorForwardDirection();
		}
		if (GetVectorLength(direction) <= 0.001f)
		{
			direction = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(direction);
		}

		const float targetDistance = std::min(
			castRange,
			std::max(0.f, GetVectorLength(targetLocation - ownerLocation))
		);
		targetLocation = ownerLocation + direction * targetDistance;

		return AbilityActorSpawner::SpawnToTarget(
			AbilityData::SolarBombardment::Actor::Projectile::BasicDefinitionId,
			executionContext,
			context.owner,
			direction,
			targetLocation
		).lock() != nullptr;
	}
}
