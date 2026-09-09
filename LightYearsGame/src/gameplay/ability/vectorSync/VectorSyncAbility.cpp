#include "gameplay/ability/vectorSync/VectorSyncAbility.h"

#include "gameplay/MovementComponent.h"
#include "gameplay/ability/vectorSync/VectorSyncContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

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

		bool HasExpectedProgressionStep(const AbilityLevelStep& step)
		{
			if (step.attributeModifiers.size() != 2 ||
				!step.unlockedUpgradeIds.empty() || !step.addedActions.empty() ||
				!step.addedTriggers.empty())
			{
				return false;
			}
			return std::any_of(step.attributeModifiers.begin(), step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == CommonAttributeIds::Duration &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, 0.10f);
				}) && std::any_of(
				step.attributeModifiers.begin(), step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == CommonAttributeIds::Cooldown &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, -0.20f);
				}
			);
		}
	}

	bool VectorSyncAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::VectorSync::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::VectorSync &&
			definition.abilityTags.size() == 2 &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::VectorSync::CategoryTag);
				}) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::VectorSync::FamilyTag);
				});
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 && NearlyEqual(definition.cooldown, 10.f) &&
			NearlyEqual(definition.duration, 5.f);
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(definition.levelProgression.begin(), definition.levelProgression.end(),
				HasExpectedProgressionStep);

		if (!validIdentity || !validLifecycle || !validProgression ||
			!definition.attributes.empty() || !definition.actions.empty() ||
			!definition.effectSpecs.empty() || !definition.scalingRules.empty() ||
			!definition.damageTags.empty() || !definition.triggers.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Vector Sync requires its five-second movement-only lifecycle and fourteen duration/cooldown progression steps.";
			}
			return false;
		}
		return true;
	}

	bool VectorSyncAbility::Activate(GameAbilityBehaviorContext& context)
	{
		auto* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || mActive ||
			ship->GetMovementComponent().GetMovementMode() != ShipMovementMode::ThrustDrift)
		{
			return false;
		}

		// The movement component resolves the forward thrust at input time. This
		// does not copy values into an AttributeSystem, so live buffs/debuffs are
		// reflected immediately and do not require a per-frame write.
		ship->GetMovementComponent().SetDirectionalThrustSync(
			context.definition.abilityId,
			true
		);
		context.abilitySystem.AddOwnedTag(AbilityData::VectorSync::State::Active);
		mActive = true;
		return true;
	}

	void VectorSyncAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}

		if (auto* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetMovementComponent().SetDirectionalThrustSync(
				context.definition.abilityId,
				false
			);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::VectorSync::State::Active);
		mActive = false;
	}
}
