#include "gameplay/ability/ionStorm/IonStormAbility.h"

#include "attributes/AttributeSystem.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr std::size_t RequiredProgressionStepCount = 14;
		constexpr float BaseCooldown = 10.f;
		constexpr float BaseDuration = 4.f;
		constexpr float BaseDamage = 12.f;
		constexpr float BaseTickInterval = 0.25f;
		constexpr float BaseCastRange = 900.f;
		constexpr float BaseProjectileSpeed = 2000.f;
		constexpr float BaseInnerCoreRadius = 250.f;
		constexpr float BaseOuterMinRadius = 250.f;
		constexpr float BaseOuterMaxRadius = 335.f;
		constexpr float BaseBoundaryPointCount = 20.f;
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		const sas::GameplayAttribute* FindDefinitionAttribute(
			const List<sas::GameplayAttribute>& attributes,
			const sas::AttributeId& id
		)
		{
			return sas::FindAttribute(attributes, id);
		}

		float FindValue(
			const List<sas::GameplayAttribute>& attributes,
			const sas::AttributeId& id,
			float fallback
		)
		{
			const sas::GameplayAttribute* attribute = FindDefinitionAttribute(attributes, id);
			return attribute && std::isfinite(attribute->baseValue)
				? attribute->baseValue
				: fallback;
		}

		bool HasValidAttribute(
			const List<sas::GameplayAttribute>& attributes,
			const sas::AttributeId& id,
			float minimum,
			bool wholeNumber = false
		)
		{
			const sas::GameplayAttribute* attribute = FindDefinitionAttribute(attributes, id);
			if (!attribute || !std::isfinite(attribute->baseValue) ||
				attribute->baseValue < minimum)
			{
				return false;
			}
			return !wholeNumber ||
				std::round(attribute->baseValue) == attribute->baseValue;
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
					AbilityData::IonStorm::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::IonStorm::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasExpectedScaling(const GameAbilityDefinition& definition)
		{
			if (definition.scalingRules.size() != 1)
			{
				return false;
			}

			bool hasEnergyPower = false;
			for (const sas::AttributeScalingRule& rule : definition.scalingRules)
			{
				if (rule.targetAttributeId != CommonAttributeIds::Damage ||
					rule.operation != sas::AttributeModifierOperation::Add)
				{
					continue;
				}
				hasEnergyPower = rule.sourceAttributeId == OwnerAttributeIds::EnergyPower &&
					std::isfinite(rule.coefficient) && rule.coefficient > 0.f;
			}
			return hasEnergyPower;
		}

		bool HasModifierWithSign(
			const AbilityLevelStep& step,
			const sas::AttributeId& id,
			bool positive
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == id &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::isfinite(modifier.magnitude) &&
					(positive ? modifier.magnitude > 0.f : modifier.magnitude < 0.f))
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
				const SpawnActorAction* spawn = std::get_if<SpawnActorAction>(
					&action.action
				);
				if (action.phase == sas::AbilityActionPhase::OnActivate && spawn &&
					spawn->actorDefinitionId ==
						AbilityData::IonStorm::Actor::Projectile::BasicDefinitionId &&
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

	bool IonStormAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* projectile = AbilityData::FindAbilityActorDefinition(
			AbilityData::IonStorm::Actor::Projectile::BasicDefinitionId
		);
		const AbilityActorDefinition* field = AbilityData::FindAbilityActorDefinition(
			AbilityData::IonStorm::Actor::Field::BasicDefinitionId
		);
		const bool validIdentity =
			definition.abilityId == AbilityData::IonStorm::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::IonStorm &&
			HasExactAbilityTags(definition);
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 &&
			NearlyEqual(definition.cooldown, BaseCooldown) &&
			NearlyEqual(definition.duration, 0.f);

		const bool validDamageTags = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(
				DamageTypeSchema::Electric
			);

		const bool validProjectile = projectile &&
			HasValidAttribute(
				projectile->attributes,
				AbilityData::IonStorm::Attribute::ProjectileSpeed,
				0.01f
			) &&
			HasValidAttribute(
				projectile->attributes,
				AbilityData::IonStorm::Attribute::CastRange,
				0.01f
			) &&
			HasValidAttribute(
				projectile->attributes,
				AbilityData::IonStorm::Attribute::Duration,
				0.01f
			) &&
			HasValidAttribute(
				projectile->attributes,
				AbilityData::IonStorm::Attribute::Damage,
				0.f
			);

		const bool validField = field &&
			NearlyEqual(FindValue(
				field->attributes,
				AbilityData::IonStorm::Attribute::Duration,
				0.f
			), BaseDuration) &&
			NearlyEqual(FindValue(
				field->attributes,
				AbilityData::IonStorm::Attribute::TickInterval,
				0.f
			), BaseTickInterval) &&
			NearlyEqual(FindValue(
				field->attributes,
				AbilityData::IonStorm::Attribute::Damage,
				0.f
			), BaseDamage) &&
			HasValidAttribute(field->attributes, DamageAttributeIds::ElectricStacks, 1.f, true) &&
			FindValue(field->attributes, DamageAttributeIds::ElectricStacks, 0.f) <= 4.f &&
			NearlyEqual(FindValue(
				field->attributes,
				AbilityData::IonStorm::Attribute::InnerCoreRadius,
				0.f
			), BaseInnerCoreRadius) &&
			NearlyEqual(FindValue(
				field->attributes,
				AbilityData::IonStorm::Attribute::OuterMinRadius,
				0.f
			), BaseOuterMinRadius) &&
			NearlyEqual(FindValue(
				field->attributes,
				AbilityData::IonStorm::Attribute::OuterMaxRadius,
				0.f
			), BaseOuterMaxRadius) &&
			HasValidAttribute(
				field->attributes,
				AbilityData::IonStorm::Attribute::BoundaryPointCount,
				3.f,
				true
			);

		bool validProgression = definition.levelProgression.size() ==
			RequiredProgressionStepCount;
		if (validProgression)
		{
			for (const AbilityLevelStep& step : definition.levelProgression)
			{
				if (!HasModifierWithSign(step, CommonAttributeIds::Damage, true) ||
					!HasModifierWithSign(step, CommonAttributeIds::Cooldown, false) ||
					step.scalingRules.size() != 1 ||
					step.scalingRules.front().targetAttributeId != CommonAttributeIds::Damage ||
					step.scalingRules.front().sourceAttributeId != OwnerAttributeIds::EnergyPower ||
					step.scalingRules.front().operation != sas::AttributeModifierOperation::Add ||
					!std::isfinite(step.scalingRules.front().coefficient) ||
					step.scalingRules.front().coefficient <= 0.f)
				{
					validProgression = false;
					break;
				}
			}
		}

		if (!validIdentity || !validLifecycle || !validDamageTags ||
			!validProjectile || !validField || !HasExpectedScaling(definition) ||
			!validProgression || !HasProjectileSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Ion Storm requires a cursor projectile, electric field values, and EnergyPower damage scaling.";
			}
			return false;
		}

		return true;
	}
}
