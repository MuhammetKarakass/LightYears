#include "gameplay/ability/temporalRecall/TemporalRecallAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/runtime/FocusActionLocks.h"
#include "gameplay/ability/temporalRecall/TemporalRecallContracts.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr movement::MovementInfluenceSourceId TemporalRecallMovementSource =
			0x54454D504F52414Cull;
		constexpr int TemporalRecallForcedMovementPriority = 1000;

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		float ResolveRecoveryRatio(
			float baseRatio,
			float sourceValue,
			float reference,
			float perPointScale
		)
		{
			return std::max(
				0.f,
				baseRatio + std::max(0.f, sourceValue - reference) *
					std::max(0.f, perPointScale)
			);
		}
	}

	bool TemporalRecallAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::TemporalRecall::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.duration <= 0.f ||
			definition.attributes.size() != 10 ||
			definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Temporal Recall requires one duration charge, ten runtime attributes, and fourteen level steps.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::TemporalRecall::Attribute::RecallWindow,
			AbilityData::TemporalRecall::Attribute::FocusDuration,
			AbilityData::TemporalRecall::Attribute::RewindDuration,
			AbilityData::TemporalRecall::Attribute::PositiveRecoveryRatio,
			AbilityData::TemporalRecall::Attribute::MaxHealthReference,
			AbilityData::TemporalRecall::Attribute::MaxHealthRecoveryScale,
			AbilityData::TemporalRecall::Attribute::EnergyPowerReference,
			AbilityData::TemporalRecall::Attribute::EnergyPowerRecoveryScale,
			AbilityData::TemporalRecall::Attribute::OvercapHoldDuration,
			AbilityData::TemporalRecall::Attribute::OvercapDecayPerSecond
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue) ||
				attribute->baseValue < 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Temporal Recall has an invalid runtime attribute.";
				}
				return false;
			}
		}

		const float focus = sas::FindAttributeValue(
			definition.attributes,
			AbilityData::TemporalRecall::Attribute::FocusDuration,
			0.f
		);
		const float rewind = sas::FindAttributeValue(
			definition.attributes,
			AbilityData::TemporalRecall::Attribute::RewindDuration,
			0.f
		);
		const float recallWindow = sas::FindAttributeValue(
			definition.attributes,
			AbilityData::TemporalRecall::Attribute::RecallWindow,
			0.f
		);
		if (recallWindow <= 0.f || focus <= 0.f || rewind <= 0.f ||
			std::abs((focus + rewind) - definition.duration) > 0.001f)
		{
			if (failureReason)
			{
				*failureReason =
					"Temporal Recall duration must equal its positive focus and rewind phases.";
			}
			return false;
		}
		return true;
	}

	bool TemporalRecallAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || mPhase != Phase::Idle)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float recallWindow = std::max(
			0.001f,
			FindValue(values, AbilityData::TemporalRecall::Attribute::RecallWindow, 3.f)
		);
		if (!ship->GetTemporalStateHistory().TryGetSnapshotSecondsAgo(
			recallWindow,
			mSnapshot
		))
		{
			// A full history is required. A partial rewind would silently violate the
			// advertised three-second state contract.
			return false;
		}

		mFocusDuration = std::max(
			0.001f,
			FindValue(values, AbilityData::TemporalRecall::Attribute::FocusDuration, 0.2f)
		);
		mRewindDuration = std::max(
			0.001f,
			FindValue(values, AbilityData::TemporalRecall::Attribute::RewindDuration, 0.8f)
		);
		const sf::Vector2f origin = context.owner.GetActorLocation();
		const sf::Vector2f desiredOffset = mSnapshot.location - origin;
		// Resolve the destination once, before the rewind starts. This keeps the
		// sampled history immutable while preventing a direct move through static
		// geometry when the old location is no longer valid.
		mResolvedDestination = origin + movement::ConstrainMovementAgainstStaticGeometry(
			context.owner,
			desiredOffset
		);
		mElapsed = 0.f;
		mPhase = Phase::Focusing;

		ability::ApplyFocusActionLocks(context.abilitySystem);
		context.abilitySystem.AddOwnedTag(AbilityData::TemporalRecall::State::Focusing);
		movement::MovementInfluenceService::SetForcedMovement(
			context.owner,
			movement::ForcedMovementRequest{
				TemporalRecallMovementSource,
				{},
				TemporalRecallForcedMovementPriority
			}
		);
		EmitEvent(context, AbilityData::TemporalRecall::Event::Started);
		return true;
	}

	void TemporalRecallAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (mPhase != Phase::Focusing && mPhase != Phase::Rewinding)
		{
			return;
		}

		mElapsed += std::max(0.f, deltaTime);
		if (mPhase == Phase::Focusing && mElapsed >= mFocusDuration)
		{
			mPhase = Phase::Rewinding;
			context.abilitySystem.RemoveOwnedTag(AbilityData::TemporalRecall::State::Focusing);
			context.abilitySystem.AddOwnedTag(AbilityData::TemporalRecall::State::Rewinding);
			EmitEvent(context, AbilityData::TemporalRecall::Event::RewindStarted);
		}

		if (mPhase != Phase::Rewinding)
		{
			return;
		}

		const float completionTime = mFocusDuration + mRewindDuration;
		const float remaining = completionTime - mElapsed;
		if (remaining <= 0.f)
		{
			Complete(context);
			return;
		}

		const sf::Vector2f offset = mResolvedDestination - context.owner.GetActorLocation();
		movement::MovementInfluenceService::SetForcedMovement(
			context.owner,
			movement::ForcedMovementRequest{
				TemporalRecallMovementSource,
				offset / remaining,
				TemporalRecallForcedMovementPriority
			}
		);
	}

	void TemporalRecallAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		if (reason == sas::AbilityEndReason::DurationExpired &&
			mPhase != Phase::Completed)
		{
			Complete(context);
		}

		movement::MovementInfluenceService::RemoveSource(
			context.owner,
			TemporalRecallMovementSource
		);
		ability::RemoveFocusActionLocks(context.abilitySystem);
		context.abilitySystem.RemoveOwnedTag(AbilityData::TemporalRecall::State::Focusing);
		context.abilitySystem.RemoveOwnedTag(AbilityData::TemporalRecall::State::Rewinding);
		mPhase = Phase::Idle;
		mElapsed = 0.f;
		EmitEvent(context, AbilityData::TemporalRecall::Event::Ended);
	}

	void TemporalRecallAbility::Complete(GameAbilityBehaviorContext& context)
	{
		if (mPhase == Phase::Completed)
		{
			return;
		}

		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship)
		{
			return;
		}

		const sf::Vector2f correction = movement::ConstrainMovementAgainstStaticGeometry(
			context.owner,
			mResolvedDestination - context.owner.GetActorLocation()
		);
		context.owner.AddActorLocationOffset(correction);
		context.owner.SetActorRotation(mSnapshot.rotationDegrees);
		movement::MovementInfluenceService::RemoveSource(
			context.owner,
			TemporalRecallMovementSource
		);
		context.owner.SetVelocity(mSnapshot.velocity);

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float baseRecoveryRatio = FindValue(
			values,
			AbilityData::TemporalRecall::Attribute::PositiveRecoveryRatio,
			0.6f
		);
		const sas::AttributeSystem& ownerAttributes =
			context.abilitySystem.GetAttributes();
		const float healthRatio = ResolveRecoveryRatio(
			baseRecoveryRatio,
			ownerAttributes.GetCurrentValue(
				AbilityData::TemporalRecall::Attribute::HealthScalingSource
			),
			FindValue(values, AbilityData::TemporalRecall::Attribute::MaxHealthReference, 100.f),
			FindValue(values, AbilityData::TemporalRecall::Attribute::MaxHealthRecoveryScale, 0.f)
		);
		const float shieldRatio = ResolveRecoveryRatio(
			baseRecoveryRatio,
			ownerAttributes.GetCurrentValue(
				AbilityData::TemporalRecall::Attribute::ShieldScalingSource
			),
			FindValue(values, AbilityData::TemporalRecall::Attribute::EnergyPowerReference, 100.f),
			FindValue(values, AbilityData::TemporalRecall::Attribute::EnergyPowerRecoveryScale, 0.f)
		);
		const float holdDuration = std::max(
			0.f,
			FindValue(values, AbilityData::TemporalRecall::Attribute::OvercapHoldDuration, 4.f)
		);
		const float decayPerSecond = std::max(
			0.f,
			FindValue(values, AbilityData::TemporalRecall::Attribute::OvercapDecayPerSecond, 100.f)
		);

		HealthComponent& health = ship->GetHealthComponent();
		if (mSnapshot.health > health.GetHealth())
		{
			health.GrantTemporaryOverhealth(
				context.definition.abilityId,
				(mSnapshot.health - health.GetHealth()) * healthRatio,
				holdDuration,
				decayPerSecond
			);
		}
		else if (mSnapshot.health < health.GetHealth())
		{
			health.ChangeHealth(mSnapshot.health - health.GetHealth());
		}

		ShieldComponent& shield = ship->GetShieldComponent();
		if (mSnapshot.shield > shield.GetShield())
		{
			shield.GrantTemporaryOvershield(
				context.definition.abilityId,
				(mSnapshot.shield - shield.GetShield()) * shieldRatio,
				holdDuration,
				decayPerSecond
			);
		}
		else if (mSnapshot.shield < shield.GetShield())
		{
			shield.ChangeShield(mSnapshot.shield - shield.GetShield());
		}

		mPhase = Phase::Completed;
		EmitEvent(context, AbilityData::TemporalRecall::Event::Completed);
	}

	void TemporalRecallAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}
}
