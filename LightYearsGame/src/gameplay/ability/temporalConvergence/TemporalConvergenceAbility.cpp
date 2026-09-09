#include "gameplay/ability/temporalConvergence/TemporalConvergenceAbility.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceContracts.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceFieldActor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		bool HasPositiveAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& id
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes, id
			);
			return attribute && std::isfinite(attribute->baseValue) &&
				attribute->baseValue > 0.f;
		}
	}

	bool TemporalConvergenceAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* field = AbilityData::FindAbilityActorDefinition(
			AbilityData::TemporalConvergence::Actor::Field::BasicDefinitionId
		);
		const bool validIdentity =
			definition.abilityId == AbilityData::TemporalConvergence::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::TemporalConvergence &&
			definition.abilityTags.size() == 2 &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(
						AbilityData::TemporalConvergence::CategoryTag
					);
				}) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(
						AbilityData::TemporalConvergence::FamilyTag
					);
				});
		const bool validLifecycle = sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 && NearlyEqual(definition.duration, 0.f) &&
			definition.cooldown > 0.f;
		const List<sas::AttributeId> requiredAttributes{
				AbilityData::TemporalConvergence::Attribute::CastRange,
				AbilityData::TemporalConvergence::Attribute::Radius,
				AbilityData::TemporalConvergence::Attribute::BaseShield,
				AbilityData::TemporalConvergence::Attribute::InitialDelay,
				AbilityData::TemporalConvergence::Attribute::TravelSpeed,
				AbilityData::TemporalConvergence::Attribute::FieldDuration,
				AbilityData::TemporalConvergence::Attribute::SlowFraction,
				AbilityData::TemporalConvergence::Attribute::StunDuration,
				AbilityData::TemporalConvergence::Attribute::OvershieldHoldDuration,
				AbilityData::TemporalConvergence::Attribute::OvershieldDecayPerSecond
		};
		const bool validAttributes = std::all_of(
			requiredAttributes.begin(), requiredAttributes.end(),
			[&definition](const sas::AttributeId& id)
			{
				return HasPositiveAttribute(definition, id);
			}
		);
		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId ==
				AbilityData::TemporalConvergence::Attribute::BaseShield &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyMax &&
			definition.scalingRules.front().operation ==
				sas::AttributeModifierOperation::Add &&
			NearlyEqual(definition.scalingRules.front().coefficient, 0.60f);
		const bool validProgression = definition.levelProgression.size() == 14;
		const bool validActor = field &&
			field->actorType == AbilityActorType::TemporalConvergenceField &&
			field->lifeTime >= 5.6f && field->presentationProfileId.IsValid();

		if (!validIdentity || !validLifecycle || !validAttributes || !validScaling ||
			!validProgression || !validActor || !definition.actions.empty() ||
			!definition.damageTags.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Temporal Convergence requires its instant snapshot field, EnergyMax shield scaling, and fourteen level steps.";
			}
			return false;
		}
		return true;
	}

	bool TemporalConvergenceAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return false;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem, &context.definition, nullptr, &context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float castRange = std::max(1.f, sas::FindAttributeValue(
			values,
			AbilityData::TemporalConvergence::Attribute::CastRange,
			1600.f
		));
		const sf::Vector2f origin = context.owner.GetActorLocation();
		sf::Vector2f target = world->GetMouseWorldPosition();
		sf::Vector2f offset = target - origin;
		const float distance = GetVectorLength(offset);
		if (distance > castRange)
		{
			target = origin + offset / distance * castRange;
			offset = target - origin;
		}

		sf::Vector2f direction = offset;
		if (GetVectorLength(direction) <= Epsilon)
		{
			direction = context.owner.GetActorForwardDirection();
		}
		if (GetVectorLength(direction) <= Epsilon)
		{
			direction = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(direction);
		}

		const shared_ptr<AbilityWorldActor> spawned =
			AbilityActorSpawner::SpawnAtLocation(
				AbilityData::TemporalConvergence::Actor::Field::BasicDefinitionId,
				executionContext,
				context.owner,
				origin,
				direction
			).lock();
		const auto field = std::dynamic_pointer_cast<TemporalConvergenceFieldActor>(
			spawned
		);
		if (!field)
		{
			return false;
		}

		// The actor receives an immutable target and resolved values once. It does
		// not read the cursor afterwards, so it stays independent of player input.
		field->ConfigureFromAbilityValues(values);
		field->SetSnapshotTarget(target);
		return true;
	}
}
