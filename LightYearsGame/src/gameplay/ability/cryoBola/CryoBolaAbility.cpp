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
		constexpr float DamagePerLevel = 4.f;
		constexpr float SlowPerLevel = 0.01f;
		constexpr float CooldownPerLevel = -0.20f;

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
			DamageAttributeIds::CryoBuildupPerHit,
			DamageAttributeIds::CryoBuildupRequired,
			DamageAttributeIds::CryoBuildupDuration,
			DamageAttributeIds::CryoSlowPercent,
			DamageAttributeIds::CryoSlowDuration
		})
		{
			if (!validProjectile || !sas::FindAttribute(projectile->attributes, required))
			{
				validProjectile = false;
				break;
			}
		}

		bool validProgression = definition.levelProgression.size() == 14;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (!validProgression || step.attributeModifiers.size() != 3 ||
				!HasModifier(step, CommonAttributeIds::Damage, DamagePerLevel) ||
				!HasModifier(step, DamageAttributeIds::CryoSlowPercent, SlowPerLevel) ||
				!HasModifier(step, CommonAttributeIds::Cooldown, CooldownPerLevel))
			{
				validProgression = false;
				break;
			}
		}

		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyMax &&
			definition.scalingRules.front().operation ==
				sas::AttributeModifierOperation::Add &&
			NearlyEqual(definition.scalingRules.front().coefficient, 0.20f);

		if (!validIdentity || !validLifecycle || !validDamage || !validProjectile ||
			!validProgression || !validScaling || !HasSpawnAction(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Cryo Bola requires one instant owner-forward projectile with Cryo full-stack payload, EnergyMax damage scaling, and its fixed 15-level progression.";
			}
			return false;
		}
		return true;
	}
}
