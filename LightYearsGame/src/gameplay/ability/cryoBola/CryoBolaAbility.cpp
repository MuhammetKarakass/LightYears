#include "gameplay/ability/cryoBola/CryoBolaAbility.h"

#include "attributes/AttributeSystem.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/cryoBola/CryoBolaContracts.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>
#include <cmath>
#include <variant>

namespace ly
{
	namespace
	{
		constexpr float BaseCooldown = 13.f;
		constexpr float BaseDamage = 40.f;
		constexpr float DamagePerLevel = 10.f;
		constexpr float BaseEnergyPowerDamageScale = 0.50f;
		constexpr float EnergyPowerDamageScalePerLevel = 0.06f;
		constexpr float DirectHitDamageMultiplier = 1.50f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= 0.0001f;
		}

		bool HasModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float magnitude
		)
		{
			return std::any_of(
				step.attributeModifiers.begin(),
				step.attributeModifiers.end(),
				[&](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == attributeId &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, magnitude);
				}
			);
		}

		bool HasSpawnAction(const GameAbilityDefinition& definition)
		{
			return std::any_of(
				definition.actions.begin(),
				definition.actions.end(),
				[](const AbilityActionSpec& action)
				{
					const auto* spawn = std::get_if<SpawnActorAction>(&action.action);
					return action.phase == sas::AbilityActionPhase::OnActivate && spawn &&
						spawn->actorDefinitionId ==
							AbilityData::CryoBola::Actor::Projectile::BasicDefinitionId &&
						spawn->spawnPolicy == sas::AbilitySpawnPolicy::OwnerForward &&
						spawn->directionPolicy == sas::AbilityDirectionPolicy::OwnerForward &&
						action.maxExecutions == 1;
				}
			);
		}
	}

	bool CryoBolaAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* projectile =
			AbilityData::FindAbilityActorDefinition(
				AbilityData::CryoBola::Actor::Projectile::BasicDefinitionId
			);
		const bool validIdentity =
			definition.abilityId == AbilityData::CryoBola::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::CryoBola &&
			definition.abilityTags.size() == 2 &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(
						AbilityData::CryoBola::CategoryTag
					);
				}) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(
						AbilityData::CryoBola::FamilyTag
					);
				});
		const bool validLifecycle = sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 && NearlyEqual(definition.duration, 0.f) &&
			NearlyEqual(definition.cooldown, BaseCooldown);
		const bool validDamage = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Cryo);

		bool validProjectile = projectile &&
			projectile->actorType == AbilityActorType::CryoBolaProjectile;
		for (const sas::AttributeId& required : {
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius,
			AbilityData::CryoBola::Actor::Projectile::ProjectileSpeed,
			AbilityData::CryoBola::Actor::Projectile::RuptureRadius,
			AbilityData::CryoBola::Actor::Projectile::EnergyPowerDamageScale,
			AbilityData::CryoBola::Actor::Projectile::DirectHitDamageMultiplier,
			DamageAttributeIds::CryoBuildupPerHit
		})
		{
			if (!validProjectile || !sas::FindAttribute(projectile->attributes, required))
			{
				validProjectile = false;
				break;
			}
		}

		List<AbilityLevelStep> levelSteps = definition.levelProgression;
		levelSteps.insert(levelSteps.end(), definition.repeatingLevelProgression.begin(),
			definition.repeatingLevelProgression.end());
		bool validProgression = !levelSteps.empty();
		for (const AbilityLevelStep& step : levelSteps)
		{
			if (!validProgression || step.attributeModifiers.size() != 2 ||
				!HasModifier(step, CommonAttributeIds::Damage, DamagePerLevel) ||
				!HasModifier(step,
					AbilityData::CryoBola::Actor::Projectile::EnergyPowerDamageScale,
					EnergyPowerDamageScalePerLevel))
			{
				validProgression = false;
				break;
			}
		}

		const bool validProjectileScaling = projectile &&
			NearlyEqual(sas::FindAttributeValue(
				projectile->attributes,
				CommonAttributeIds::Damage,
				0.f
			), BaseDamage) &&
			NearlyEqual(sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::CryoBola::Actor::Projectile::EnergyPowerDamageScale,
				0.f
			), BaseEnergyPowerDamageScale) &&
			NearlyEqual(sas::FindAttributeValue(
				projectile->attributes,
				AbilityData::CryoBola::Actor::Projectile::DirectHitDamageMultiplier,
				0.f
			), DirectHitDamageMultiplier);

		if (!validIdentity || !validLifecycle || !validDamage || !validProjectile ||
			!validProgression || !validProjectileScaling ||
			!definition.scalingRules.empty() || !HasSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Cryo Bola requires one instant owner-forward projectile with Cryo full-stack payload, its direct-hit EnergyPower formula, and its fixed 14-step progression.";
			}
			return false;
		}
		return true;
	}
}
