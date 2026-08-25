#include "gameplay/ability/energySpear/EnergySpearAbility.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/DirectionalChargeTelegraphActor.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/ability/energySpear/EnergySpearTraversalActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/energySpear/EnergySpearPresentationIds.h"
#include "presentation/ability/energySpear/EnergySpearPresentationProfile.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}

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

		sf::Vector2f ResolveMouseDirection(const Actor& owner)
		{
			if (const World* world = owner.GetWorld(); world && world->GetApplication())
			{
				sf::Vector2f direction = world->GetMouseWorldPosition() -
					owner.GetActorLocation();
				if (GetVectorLength(direction) > 0.001f)
				{
					NormalizeVector(direction);
					return direction;
				}
			}

			// A cursor exactly on the ship has no usable direction. Use a fixed
			// world direction instead of silently falling back to ship rotation.
			return { 0.f, -1.f };
		}

		bool HasExpectedScalingRule(const GameAbilityDefinition& definition)
		{
			if (definition.scalingRules.size() != 1)
			{
				return false;
			}

			const sas::AttributeScalingRule& rule = definition.scalingRules.front();
			return rule.targetAttributeId == AbilityData::EnergySpear::Attribute::Damage &&
				rule.sourceAttributeId == OwnerAttributeIds::AttackPower &&
				rule.operation == sas::AttributeModifierOperation::Add &&
				std::abs(rule.coefficient - 1.f) <= 0.0001f;
		}

		bool HasExpectedLevelStep(const AbilityLevelStep& step)
		{
			if (step.attributeModifiers.size() != 2)
			{
				return false;
			}

			bool damageStep = false;
			bool cooldownStep = false;
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == CommonAttributeIds::Damage &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::abs(modifier.magnitude - 2.f) <= 0.0001f)
				{
					damageStep = true;
				}
				if (modifier.attributeId == CommonAttributeIds::Cooldown &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					std::abs(modifier.magnitude + 0.20f) <= 0.0001f)
				{
					cooldownStep = true;
				}
			}
			return damageStep && cooldownStep;
		}
	}

	bool EnergySpearAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validLifecycle =
			definition.abilityId == AbilityData::EnergySpear::AbilityId::Basic &&
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::WhileHeld &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::WhileInputHeld &&
			definition.maxCharges == 1 &&
			IsFinite(definition.cooldown) && definition.cooldown > 0.f &&
			IsFinite(definition.duration) && definition.duration > 0.f;
		if (!validLifecycle || !HasExpectedScalingRule(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Energy Spear requires held activation, one charge, positive timing and AttackPower damage scaling.";
			}
			return false;
		}

		const List<sas::AttributeId> requiredAttributes{
			AbilityData::EnergySpear::Attribute::Damage,
			AbilityData::EnergySpear::Attribute::MaximumDistance,
			AbilityData::EnergySpear::Attribute::CollisionRadius,
			AbilityData::EnergySpear::Attribute::MinimumDistance,
			AbilityData::EnergySpear::Attribute::MaximumDistanceChargeThreshold,
			AbilityData::EnergySpear::Attribute::ChargeDamageMultiplierAtFull,
			AbilityData::EnergySpear::Attribute::DistanceDamageMultiplierAtEndpoint,
			AbilityData::EnergySpear::Attribute::EnergyMaxReference,
			AbilityData::EnergySpear::Attribute::EnergyMaxDistanceScale,
			AbilityData::EnergySpear::Attribute::TravelSpeed
		};
		for (const sas::AttributeId& attributeId : requiredAttributes)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				attributeId
			);
			if (!attribute || !IsFinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Energy Spear must declare every runtime attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return sas::FindAttributeValue(definition.attributes, id, 0.f);
		};
		const float maximumDistance = value(AbilityData::EnergySpear::Attribute::MaximumDistance);
		const float minimumDistance = value(AbilityData::EnergySpear::Attribute::MinimumDistance);
		const float threshold = value(
			AbilityData::EnergySpear::Attribute::MaximumDistanceChargeThreshold
		);
		if (value(AbilityData::EnergySpear::Attribute::Damage) < 0.f ||
			maximumDistance < minimumDistance || minimumDistance <= 0.f ||
			value(AbilityData::EnergySpear::Attribute::CollisionRadius) <= 0.f ||
			threshold <= 0.f || threshold > 1.f ||
			value(AbilityData::EnergySpear::Attribute::ChargeDamageMultiplierAtFull) < 1.f ||
			value(AbilityData::EnergySpear::Attribute::DistanceDamageMultiplierAtEndpoint) < 1.f ||
			value(AbilityData::EnergySpear::Attribute::EnergyMaxReference) < 0.f ||
			value(AbilityData::EnergySpear::Attribute::EnergyMaxDistanceScale) < 0.f ||
			value(AbilityData::EnergySpear::Attribute::TravelSpeed) <= 0.f)
		{
			if (failureReason)
			{
				*failureReason = "Energy Spear contains invalid distance, charge or movement values.";
			}
			return false;
		}

		if (definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Energy ||
			definition.levelProgression.size() != 14 ||
			!std::all_of(
				definition.levelProgression.begin(),
				definition.levelProgression.end(),
				HasExpectedLevelStep
			))
		{
			if (failureReason)
			{
				*failureReason =
					"Energy Spear requires Energy damage and fourteen Damage/Cooldown progression steps.";
			}
			return false;
		}

		if (!PresentationProfileRegistry<EnergySpearPresentationProfile>::Find(
				EnergySpearPresentationIds::ChargeBasic
			) ||
			!PresentationProfileRegistry<EnergySpearPresentationProfile>::Find(
				EnergySpearPresentationIds::TraversalBasic
			))
		{
			if (failureReason)
			{
				*failureReason = "Energy Spear presentation profiles are not registered.";
			}
			return false;
		}

		return true;
	}

	bool EnergySpearAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mStarted)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		mStartLocation = context.owner.GetActorLocation();
		mDirection = ResolveMouseDirection(context.owner);
		mFocusElapsed = 0.f;
		mStarted = true;
		mReleased = false;
		mTraversalStarted = false;

		context.abilitySystem.AddOwnedTag(GameplayTags::State::ActionLock::AbilityActivation);
		context.abilitySystem.AddOwnedTag(GameplayTags::State::ActionLock::PrimaryWeaponFire);
		// Focusing is a charge/aim phase. The player must remain free to move
		// while choosing position; movement is locked only after release hands
		// control to the fixed-direction traversal actor.
		context.abilitySystem.AddOwnedTag(AbilityData::EnergySpear::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (const EnergySpearPresentationProfile* profile =
				PresentationProfileRegistry<EnergySpearPresentationProfile>::Find(
					EnergySpearPresentationIds::ChargeBasic
				))
			{
				const float minimumDistance = FindValue(
					values,
					AbilityData::EnergySpear::Attribute::MinimumDistance,
					200.f
				);
				const float maximumDistance = FindValue(
					values,
					AbilityData::EnergySpear::Attribute::MaximumDistance,
					600.f
				);
				const float threshold = FindValue(
					values,
					AbilityData::EnergySpear::Attribute::MaximumDistanceChargeThreshold,
					0.90f
				);
				mTelegraph = world->SpawnActor<DirectionalChargeTelegraphActor>(
					DirectionalChargeTelegraphActor::SpawnParams{
						mStartLocation,
						mDirection,
						minimumDistance,
						maximumDistance,
						context.definition.duration,
						threshold,
						profile->chargeTelegraph,
						&context.owner
					}
				);
			}
		}

		EmitEvent(context, AbilityData::EnergySpear::Event::Started);
		return true;
	}

	void EnergySpearAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (!mStarted || mReleased)
		{
			return;
		}

		// Aim remains live throughout the focus phase. The traversal receives
		// the final direction when the input is released; it is not tied to the
		// ship's rotation or to the direction from activation time.
		mDirection = ResolveMouseDirection(context.owner);
		if (const shared_ptr<DirectionalChargeTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetDirection(mDirection);
		}

		mFocusElapsed = std::min(
			std::max(0.f, context.definition.duration),
			mFocusElapsed + std::max(0.f, deltaTime)
		);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float progress = std::clamp(mFocusElapsed / focusDuration, 0.f, 1.f);
		if (const shared_ptr<DirectionalChargeTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetExternalProgress(progress);
		}

		const float threshold = sas::FindAttributeValue(
			ResolveValues(context),
			AbilityData::EnergySpear::Attribute::MaximumDistanceChargeThreshold,
			0.90f
		);
		if (progress >= threshold && progress - std::max(0.f, deltaTime) / focusDuration < threshold)
		{
			EmitEvent(context, AbilityData::EnergySpear::Event::MaxDistanceReached);
		}

		if (progress >= 1.f)
		{
			Release(context);
			context.instance.Cancel(sas::AbilityEndReason::Completed);
		}
	}

	void EnergySpearAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		if (!mStarted)
		{
			return;
		}

		if (!mReleased &&
			(reason == sas::AbilityEndReason::InputReleased ||
				reason == sas::AbilityEndReason::Completed))
		{
			Release(context);
		}

		if (!mTraversalStarted)
		{
			RemoveFocusLocks(context);
			if (const shared_ptr<DirectionalChargeTelegraphActor> telegraph = mTelegraph.lock())
			{
				if (!telegraph->IsInCompletionFeedback())
				{
					telegraph->Destroy();
				}
			}
			EmitEvent(context, AbilityData::EnergySpear::Event::Ended);
		}

		mTelegraph.reset();
		mFocusElapsed = 0.f;
		mStarted = false;
		mReleased = false;
		mTraversalStarted = false;
	}

	void EnergySpearAbility::Release(GameAbilityBehaviorContext& context)
	{
		if (mReleased)
		{
			return;
		}
		mReleased = true;
		// Movement is allowed during focus, so the spear must originate from the
		// ship's current position rather than the position at activation time.
		mStartLocation = context.owner.GetActorLocation();
		mDirection = ResolveMouseDirection(context.owner);

		const shared_ptr<DirectionalChargeTelegraphActor> telegraph = mTelegraph.lock();
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float chargeProgress = std::clamp(
			mFocusElapsed / focusDuration,
			0.f,
			1.f
		);
		const float minimumDistance = std::max(
			1.f,
			FindValue(values, AbilityData::EnergySpear::Attribute::MinimumDistance, 200.f)
		);
		const float baseMaximumDistance = std::max(
			minimumDistance,
			FindValue(values, AbilityData::EnergySpear::Attribute::MaximumDistance, 600.f)
		);
		const float maximumDistanceChargeThreshold = std::clamp(
			FindValue(
				values,
				AbilityData::EnergySpear::Attribute::MaximumDistanceChargeThreshold,
				0.90f
			),
			0.001f,
			1.f
		);
		const float energyMax = context.abilitySystem.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::EnergyMax
		);
		const float energyMaxReference = std::max(
			0.f,
			FindValue(values, AbilityData::EnergySpear::Attribute::EnergyMaxReference, 50.f)
		);
		const float energyMaxDistanceScale = std::max(
			0.f,
			FindValue(values, AbilityData::EnergySpear::Attribute::EnergyMaxDistanceScale, 2.f)
		);
		const float maximumDistance = baseMaximumDistance +
			std::max(0.f, energyMax - energyMaxReference) * energyMaxDistanceScale;
		const float distanceProgress = std::clamp(
			chargeProgress / maximumDistanceChargeThreshold,
			0.f,
			1.f
		);
		const float travelDistance = minimumDistance +
			(maximumDistance - minimumDistance) * distanceProgress;
		const float fullChargeDamageMultiplier = std::max(
			1.f,
			FindValue(
				values,
				AbilityData::EnergySpear::Attribute::ChargeDamageMultiplierAtFull,
				2.50f
			)
		);
		const float chargeDamageMultiplier = 1.f +
			(fullChargeDamageMultiplier - 1.f) * chargeProgress;
		const float damage = std::max(
			0.f,
			FindValue(values, AbilityData::EnergySpear::Attribute::Damage, 10.f)
		);
		const float travelSpeed = std::max(
			1.f,
			FindValue(values, AbilityData::EnergySpear::Attribute::TravelSpeed, 2400.f)
		);
		const float collisionRadius = std::max(
			0.1f,
			FindValue(values, AbilityData::EnergySpear::Attribute::CollisionRadius, 18.f)
		);
		const float endpointDamageMultiplier = std::max(
			1.f,
			FindValue(
				values,
				AbilityData::EnergySpear::Attribute::DistanceDamageMultiplierAtEndpoint,
				2.f
			)
		);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		const DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);

		World* world = context.owner.GetWorld();
		const EnergySpearPresentationProfile* profile =
			PresentationProfileRegistry<EnergySpearPresentationProfile>::Find(
				EnergySpearPresentationIds::TraversalBasic
			);
		if (!world || !profile)
		{
			return;
		}

		const weak_ptr<EnergySpearTraversalActor> traversal =
			world->SpawnActor<EnergySpearTraversalActor>(
				EnergySpearTraversalRequest{
					&context.owner,
					mStartLocation,
					mDirection,
					travelDistance,
					travelSpeed,
					collisionRadius,
					minimumDistance,
					endpointDamageMultiplier,
					chargeDamageMultiplier,
					damage,
					damageTags,
					payload,
					sas::ContentId{ context.definition.abilityId },
					context.definition.abilityTags,
					profile->traversal
				}
			);
		if (traversal.expired())
		{
			return;
		}

		mTraversalStarted = true;
		context.abilitySystem.RemoveOwnedTag(AbilityData::EnergySpear::State::Focusing);
		context.abilitySystem.AddOwnedTag(AbilityData::EnergySpear::State::Traversing);
		// Traversal moves the owner along a captured direction, so normal input
		// must stop affecting the ship until the traversal actor completes.
		context.abilitySystem.AddOwnedTag(GameplayTags::State::ActionLock::MovementInput);
		if (telegraph)
		{
			// The charge preview belongs to the focus phase. Once released, the
			// traversal actor owns the directional feedback and the preview must
			// not keep following the moving ship.
			telegraph->Destroy();
		}
		EmitEvent(context, AbilityData::EnergySpear::Event::Released);
	}

	void EnergySpearAbility::RemoveFocusLocks(GameAbilityBehaviorContext& context) const
	{
		context.abilitySystem.RemoveOwnedTag(
			GameplayTags::State::ActionLock::AbilityActivation
		);
		context.abilitySystem.RemoveOwnedTag(
			GameplayTags::State::ActionLock::PrimaryWeaponFire
		);
		context.abilitySystem.RemoveOwnedTag(AbilityData::EnergySpear::State::Focusing);
	}

	void EnergySpearAbility::EmitEvent(
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
