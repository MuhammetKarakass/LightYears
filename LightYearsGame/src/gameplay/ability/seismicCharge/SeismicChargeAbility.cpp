#include "gameplay/ability/seismicCharge/SeismicChargeAbility.h"

#include "attributes/AttributeSystem.h"
#include "framework/Actor.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/seismicCharge/SeismicChargeContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

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
	}

	bool SeismicChargeAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* bomb = AbilityData::FindAbilityActorDefinition(
			AbilityData::SeismicCharge::Actor::Bomb::BasicDefinitionId
		);
		const bool validIdentity =
			definition.abilityId == AbilityData::SeismicCharge::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::SeismicCharge &&
			definition.abilityTags.size() == 2 &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag) { return tag.MatchesTagExact(AbilityData::SeismicCharge::CategoryTag); }) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag) { return tag.MatchesTagExact(AbilityData::SeismicCharge::FamilyTag); });
		const bool validLifecycle = sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 && NearlyEqual(definition.duration, 0.f) &&
			NearlyEqual(definition.cooldown, 21.f);
		const bool validDamage = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Energy);
		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyPower &&
			definition.scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(definition.scalingRules.front().coefficient, 0.80f);
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(definition.levelProgression.begin(), definition.levelProgression.end(),
				[](const AbilityLevelStep& step)
				{
					return step.attributeModifiers.size() == 2 &&
						step.attributeModifiers[0].attributeId == CommonAttributeIds::Damage &&
						NearlyEqual(step.attributeModifiers[0].magnitude, 7.f) &&
						step.attributeModifiers[1].attributeId == CommonAttributeIds::Cooldown &&
						NearlyEqual(step.attributeModifiers[1].magnitude, -0.30f);
				});
		const bool validActor = bomb && bomb->actorType == AbilityActorType::SeismicChargeBomb &&
			bomb->lifeTime >= 5.5f && bomb->presentationProfileId.IsValid();

		if (!validIdentity || !validLifecycle || !validDamage || !validScaling ||
			!validProgression || !validActor || !definition.actions.empty())
		{
			if (failureReason)
			{
				*failureReason = "Seismic Charge requires its instant Energy bomb lifecycle, EnergyPower scaling, and 15-level damage/cooldown progression.";
			}
			return false;
		}
		return true;
	}

	bool SeismicChargeAbility::Activate(GameAbilityBehaviorContext& context)
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem, &context.definition, nullptr, &context.instance
		};
		sf::Vector2f forward = context.owner.GetActorForwardDirection();
		if (GetVectorLength(forward) <= Epsilon)
		{
			forward = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(forward);
		}

		// Spawn at the owner. The bomb actor captures this world-space snapshot
		// during configuration, then moves to the frozen position behind the ship.
		return AbilityActorSpawner::SpawnAtLocation(
			AbilityData::SeismicCharge::Actor::Bomb::BasicDefinitionId,
			executionContext,
			context.owner,
			context.owner.GetActorLocation(),
			forward
		).lock() != nullptr;
	}
}
