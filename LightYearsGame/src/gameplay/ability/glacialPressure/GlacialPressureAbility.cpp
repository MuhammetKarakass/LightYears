#include "gameplay/ability/glacialPressure/GlacialPressureAbility.h"

#include "gameplay/ability/runtime/FocusActionLocks.h"

#include "effects/GameplayEffectSpec.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/ability/glacialPressure/GlacialPressurePushMath.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/glacialPressure/GlacialPressureTelegraphActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/glacialPressure/GlacialPressurePresentationIds.h"
#include "presentation/ability/glacialPressure/GlacialPressurePresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/MathUtility.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
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

		sas::GameplayAttributeList ResolveValues(
			const GameAbilityBehaviorContext& context
		)
		{
			GameAbilityBehaviorContext mutableContext{
				const_cast<LightYearsAbilitySystemComponent&>(context.abilitySystem),
				const_cast<GameAbility&>(context.instance),
				const_cast<Actor&>(context.owner),
				context.definition
			};
			return ResolveValues(mutableContext);
		}

		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}

		int ResolveSegmentIndex(float distance, float range, int segmentCount)
		{
			const int safeSegmentCount = std::clamp(segmentCount, 1, 5);
			const float segmentLength = std::max(
				0.001f,
				range / static_cast<float>(safeSegmentCount)
			);
			return std::clamp(
				static_cast<int>(distance / segmentLength),
				0,
				safeSegmentCount - 1
			);
		}

		DamagePayload BuildCryoPayload(
			const List<GameplayTag>& damageTags,
			const sas::GameplayAttributeList& values,
			int cryoStacks
		)
		{
			DamagePayload payload = DamageTypeSystem::BuildPayload(
				damageTags,
				values
			);
			// Cryo's shared status pipeline treats each buildup application as one
			// stack. Glacial Pressure snapshots the segment's stack count into the
			// same payload instead of creating a second slow implementation.
			payload.cryoBuildupPerHit = std::clamp(cryoStacks, 0, 4);
			payload.cryoBuildupRequired = 4;
			payload.cryoBuildupDuration = 2.5f;
			payload.cryoSlowPercent = 0.25f;
			payload.cryoSlowDuration = 1.5f;
			return payload;
		}

		float ResolveShipCollisionRadius(const SpaceShip& ship)
		{
			const float explicitRadius = ship.GetPhysicsCollisionRadius();
			if (std::isfinite(explicitRadius) && explicitRadius > 0.001f)
			{
				return explicitRadius;
			}

			// Ships usually use sprite bounds rather than an explicit physics
			// radius. This ability needs the hull size for a reliable ship impact.
			const auto bounds = ship.GetActorGlobalBounds();
			return std::max(
				1.f,
				std::min(bounds.size.x, bounds.size.y) * 0.5f
			);
		}

		float ResolveInitialImpulseSpeed(
			float desiredDistance,
			float observationDuration,
			float retentionPerSecond
		)
		{
			const float retention = std::clamp(
				retentionPerSecond,
				0.0001f,
				0.9999f
			);
			const float duration = std::max(0.01f, observationDuration);
			const float decayRate = -std::log(retention);
			const float distanceFraction = std::max(
				0.001f,
				1.f - std::pow(retention, duration)
			);
			// Integrate MovementComponent's exponential decay so the configured
			// distance is reached while the ship visibly slows down.
			return std::max(0.f, desiredDistance) * decayRate / distanceFraction;
		}
	}

	bool GlacialPressureAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::GlacialPressure::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!IsFinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!IsFinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Glacial Pressure requires pressed activation, duration lifetime, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::GlacialPressure::Attribute::Range,
			AbilityData::GlacialPressure::Attribute::InitialDamage,
			AbilityData::GlacialPressure::Attribute::CollisionDamage,
			AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale,
			AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale,
			AbilityData::GlacialPressure::Attribute::MaxHealthReference,
			AbilityData::GlacialPressure::Attribute::MaxHealthPushScale,
			AbilityData::GlacialPressure::Attribute::PushDistance,
			AbilityData::GlacialPressure::Attribute::PushDuration,
			AbilityData::GlacialPressure::Attribute::ConeHalfAngleDegrees,
			AbilityData::GlacialPressure::Attribute::SegmentCount,
			AbilityData::GlacialPressure::Attribute::SegmentOneExtraStun,
			AbilityData::GlacialPressure::Attribute::PushStunDuration,
			AbilityData::GlacialPressure::Attribute::CollisionStunDuration
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !IsFinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason =
						"Glacial Pressure must declare every runtime attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return FindValue(definition.attributes, id, 0.f);
		};
		const float range = value(AbilityData::GlacialPressure::Attribute::Range);
		const float pushDuration = value(AbilityData::GlacialPressure::Attribute::PushDuration);
		const float segmentCount = value(AbilityData::GlacialPressure::Attribute::SegmentCount);
		if (range <= 0.f ||
			value(AbilityData::GlacialPressure::Attribute::InitialDamage) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::CollisionDamage) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::MaxHealthReference) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::MaxHealthPushScale) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::PushDistance) <= 0.f ||
			pushDuration <= 0.f ||
			value(AbilityData::GlacialPressure::Attribute::ConeHalfAngleDegrees) <= 0.f ||
			value(AbilityData::GlacialPressure::Attribute::ConeHalfAngleDegrees) >= 90.f ||
			segmentCount != static_cast<float>(AbilityData::GlacialPressure::DefaultSegmentCount) ||
			value(AbilityData::GlacialPressure::Attribute::SegmentOneExtraStun) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::PushStunDuration) < 0.f ||
			value(AbilityData::GlacialPressure::Attribute::CollisionStunDuration) < 0.f ||
			definition.levelProgression.size() != 14 ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Cryo)
		{
			if (failureReason)
			{
				*failureReason =
					"Glacial Pressure contains invalid range, segment, push, or Cryo values.";
			}
			return false;
		}

		return true;
	}

	float GlacialPressureAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float pushDuration = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::PushDuration,
				AbilityData::GlacialPressure::DefaultImpulseWindowDuration
			)
		);
		// The cooldown starts after focus and the impulse collision window ends.
		return std::max(0.f, defaultDuration) + pushDuration;
	}

	bool GlacialPressureAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (!dynamic_cast<SpaceShip*>(&context.owner) || mDischarged)
		{
			return false;
		}

		mPushStates.clear();
		mResolvedCollisionPairs.clear();
		mFocusElapsed = 0.f;
		mPushDuration = 0.f;
		mDischarged = false;

		// Focus uses the project-wide committed-input rule: voluntary movement,
		// primary fire, and ability activation are blocked until discharge.
		ability::ApplyFocusActionLocks(context.abilitySystem);
		context.abilitySystem.AddOwnedTag(AbilityData::GlacialPressure::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (const GlacialPressurePresentationProfile* profile =
				PresentationProfileRegistry<GlacialPressurePresentationProfile>::Find(
					GlacialPressurePresentationIds::FocusBasic
				))
			{
				mTelegraph = world->SpawnActor<GlacialPressureTelegraphActor>(
					&context.owner,
					*profile
				);
			}
		}

		EmitEvent(context, AbilityData::GlacialPressure::Event::Started);
		return true;
	}

	void GlacialPressureAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (!mDischarged)
		{
			mFocusElapsed += safeDeltaTime;
			const float focusDuration = std::max(0.001f, context.definition.duration);
			const float progress = std::clamp(
				mFocusElapsed / focusDuration,
				0.f,
				1.f
			);
			if (const shared_ptr<GlacialPressureTelegraphActor> telegraph = mTelegraph.lock())
			{
				telegraph->SetExternalProgress(progress);
			}

			if (progress >= 1.f)
			{
				mFocusElapsed = focusDuration;
				Discharge(context);
			}
			return;
		}

		TickPushes(context, safeDeltaTime);
	}

	void GlacialPressureAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		ability::RemoveFocusActionLocks(context.abilitySystem);
		context.abilitySystem.RemoveOwnedTag(AbilityData::GlacialPressure::State::Focusing);
		context.abilitySystem.RemoveOwnedTag(AbilityData::GlacialPressure::State::Pushing);

		if (const shared_ptr<GlacialPressureTelegraphActor> telegraph = mTelegraph.lock())
		{
			if (!telegraph->IsInCompletionFeedback())
			{
				telegraph->Destroy();
			}
		}
		mTelegraph.reset();
		mPushStates.clear();
		mResolvedCollisionPairs.clear();
		mFocusElapsed = 0.f;
		mPushDuration = 0.f;
		mDischarged = false;
		EmitEvent(context, AbilityData::GlacialPressure::Event::Ended);
	}

	void GlacialPressureAbility::Discharge(GameAbilityBehaviorContext& context)
	{
		if (mDischarged)
		{
			return;
		}
		mDischarged = true;

		SpaceShip* owner = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		if (!owner || !world)
		{
			return;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float range = std::max(
			1.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::Range,
				AbilityData::GlacialPressure::DefaultConeLength
			)
		);
		const float coneHalfAngleRadians = DegreesToRadians(std::clamp(
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::ConeHalfAngleDegrees,
				AbilityData::GlacialPressure::DefaultConeHalfAngleDegrees
			),
			1.f,
			89.f
		));
		const int segmentCount = std::clamp(
			static_cast<int>(std::lround(FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::SegmentCount,
				5.f
			))),
			1,
			5
		);
		mPushDuration = std::max(
			0.01f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::PushDuration,
				AbilityData::GlacialPressure::DefaultImpulseWindowDuration
			)
		);

		const float energyPower = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyPower
			)
		);
		const float maxHealth = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::MaxHealth
			)
		);
		const float initialDamage = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::InitialDamage,
				12.f
			) + energyPower * std::max(
				0.f,
				FindValue(
					values,
					AbilityData::GlacialPressure::Attribute::EnergyPowerInitialScale,
					0.05f
				)
			)
		);
		const float collisionDamage = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::CollisionDamage,
				45.f
			) + energyPower * std::max(
				0.f,
				FindValue(
					values,
					AbilityData::GlacialPressure::Attribute::EnergyPowerCollisionScale,
					0.25f
				)
			)
		);
		const float maxHealthReference = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::MaxHealthReference,
				100.f
			)
		);
		const float pushMultiplier = 1.f + std::max(
			0.f,
			maxHealth - maxHealthReference
		) * std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::MaxHealthPushScale,
				0.0005f
			)
		);
		const float pushDistance = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::PushDistance,
				AbilityData::GlacialPressure::DefaultPushDistance
			)
		);
		const float extraStunDuration = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::SegmentOneExtraStun,
				AbilityData::GlacialPressure::DefaultCollisionStunDuration
			)
		);
		const float pushStunDuration = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::PushStunDuration,
				AbilityData::GlacialPressure::DefaultPushStunDuration
			)
		);
		const float collisionStunDuration = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::GlacialPressure::Attribute::CollisionStunDuration,
				AbilityData::GlacialPressure::DefaultCollisionStunDuration
			)
		);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);

		targeting::TargetingQuery query;
		query.source = owner;
		query.origin = owner->GetActorLocation();
		query.range = range;
		query.shape = targeting::TargetingShape::Cone;
		query.direction = owner->GetActorForwardDirection();
		query.coneHalfAngleRadians = coneHalfAngleRadians;
		query.requiredTargetLayers = targeting::ResolveOpposingLayer(*owner);
		query.requireCollisionCompatibility = true;
		query.filter = [](
			const Actor*,
			const Actor& candidate,
			const targeting::TargetingCandidate&)
		{
			const SpaceShip* ship = dynamic_cast<const SpaceShip*>(&candidate);
			return ship && ship->GetHealthComponent().GetHealth() > 0.f;
		};

		for (const targeting::TargetingCandidate& candidate :
			targeting::AutoTargeting::FindTargets(*world, query))
		{
			const shared_ptr<SpaceShip> target =
				std::dynamic_pointer_cast<SpaceShip>(candidate.actor);
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}

			const float distance = std::sqrt(std::max(0.f, candidate.distanceSquared));
			const int segmentIndex = ResolveSegmentIndex(
				distance,
				range,
				segmentCount
			);
			const AbilityData::GlacialPressure::SegmentProfile& segment =
				AbilityData::GlacialPressure::SegmentProfiles[
					static_cast<std::size_t>(segmentIndex)
				];
			const float finalInitialDamage = initialDamage * segment.damageMultiplier;
			DamagePayload payload = BuildCryoPayload(
				damageTags,
				values,
				segment.cryoStacks
			);
			ApplyCombatDamage(
				*target,
				finalInitialDamage,
				owner,
				damageTags,
				payload,
				sas::ContentId{ context.definition.abilityId },
				context.definition.abilityTags
			);
			if (target->GetHealthComponent().GetHealth() <= 0.f)
			{
				continue;
			}

			const sf::Vector2f targetDelta =
				target->GetActorLocation() - owner->GetActorLocation();
			const sf::Vector2f pushDirection = GetVectorLength(targetDelta) > 0.001f
				? targetDelta / GetVectorLength(targetDelta)
				: owner->GetActorForwardDirection();
			const float finalPushDistance = glacialPressure::ResolvePushDistance(
				pushDistance,
				segment.pushMultiplier,
				pushMultiplier
			);
			const float impulseSpeed = ResolveInitialImpulseSpeed(
				finalPushDistance,
				mPushDuration,
				target->GetMovementComponent()
					.GetAttributes().linearDamping.currentValue
			);
			// Apply the force once. The common movement boundary owns ensuing
			// collision-aware displacement and exponential slowdown, so this ability
			// never moves a target at a fixed speed by itself.
			movement::MovementInfluenceService::ApplyImpulse(
				*target,
				movement::ImpulseRequest{
					pushDirection * impulseSpeed,
					target->GetMovementComponent()
						.GetAttributes().linearDamping.currentValue
				}
			);

			ApplyStun(context, *target, pushStunDuration);
			mPushStates.push_back(PushState{
				target,
				target->GetActorLocation(),
				segment.cryoStacks,
				collisionDamage * segment.collisionDamageMultiplier,
				collisionStunDuration,
				mPushDuration,
				impulseSpeed,
				segment.appliesExtraStun ? extraStunDuration : 0.f,
				segment.appliesExtraStun ? extraStunDuration > 0.f : false
			});
		}

		ability::RemoveFocusActionLocks(context.abilitySystem);
		context.abilitySystem.RemoveOwnedTag(AbilityData::GlacialPressure::State::Focusing);
		if (!mPushStates.empty())
		{
			context.abilitySystem.AddOwnedTag(AbilityData::GlacialPressure::State::Pushing);
		}

		if (const shared_ptr<GlacialPressureTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Complete();
		}
		EmitEvent(context, AbilityData::GlacialPressure::Event::Blasted);
	}

	void GlacialPressureAbility::TickPushes(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		World* world = context.owner.GetWorld();
		SpaceShip* owner = dynamic_cast<SpaceShip*>(&context.owner);
		if (!world || !owner)
		{
			return;
		}

		for (PushState& state : mPushStates)
		{
			const shared_ptr<SpaceShip> target = state.target.lock();
			if (!target || target->GetIsPendingDestroy() ||
				target->GetHealthComponent().GetHealth() <= 0.f)
			{
				continue;
			}
			if (target->IsInPortalTransit())
			{
				state.previousLocation = target->GetActorLocation();
				state.wasInPortalTransit = true;
				continue;
			}
			if (state.wasInPortalTransit)
			{
				// A portal exit is a discontinuity, never a high-speed collision
				// segment through every ship between the two endpoints.
				state.previousLocation = target->GetActorLocation();
				state.wasInPortalTransit = false;
				continue;
			}

			const sf::Vector2f currentLocation = target->GetActorLocation();
			const float impactSpeed = GetVectorLength(
				currentLocation - state.previousLocation
			) / std::max(0.001f, deltaTime);
			if (state.collisionTimeRemaining > 0.f)
			{
			for (const weak_ptr<SpaceShip>& candidateWeak :
				world->GetActorsByType<SpaceShip>())
				{
					const shared_ptr<SpaceShip> candidate = candidateWeak.lock();
				if (!candidate || candidate.get() == target.get() ||
					candidate.get() == owner || candidate->GetIsPendingDestroy() ||
					candidate->IsInPortalTransit() ||
					!targeting::IsOpposingTarget(*owner, *candidate) ||
					candidate->GetHealthComponent().GetHealth() <= 0.f)
					{
						continue;
					}

					const float targetRadius = ResolveShipCollisionRadius(*target);
					const float candidateRadius = ResolveShipCollisionRadius(*candidate);
					const float collisionRadius = targetRadius + candidateRadius;
					if (targeting::swept::DistanceSquaredToSegment(
						candidate->GetActorLocation(),
						state.previousLocation,
						currentLocation
					) > collisionRadius * collisionRadius)
					{
						continue;
					}

					const Actor* first = std::min<const Actor*>(target.get(), candidate.get());
					const Actor* second = std::max<const Actor*>(target.get(), candidate.get());
					const CollisionPair pair{ first, second };
					if (!mResolvedCollisionPairs.insert(pair).second)
					{
						continue;
					}

					ResolveCollision(
						context,
						state,
						*target,
						*candidate,
						impactSpeed
					);
				}
				state.collisionTimeRemaining = std::max(
					0.f,
					state.collisionTimeRemaining - std::max(0.f, deltaTime)
				);
			}
			if (state.collisionTimeRemaining <= 0.f && state.extraStunPending)
			{
				ApplyStun(context, *target, state.extraStunDuration);
				state.extraStunPending = false;
			}

			state.previousLocation = currentLocation;
		}

		mPushStates.erase(
			std::remove_if(
				mPushStates.begin(),
				mPushStates.end(),
				[](const PushState& state)
				{
					const shared_ptr<SpaceShip> target = state.target.lock();
					return !target || target->GetIsPendingDestroy() ||
						target->GetHealthComponent().GetHealth() <= 0.f ||
						(state.collisionTimeRemaining <= 0.f && !state.extraStunPending);
				}
			),
			mPushStates.end()
		);

		if (mPushStates.empty())
		{
			context.abilitySystem.RemoveOwnedTag(AbilityData::GlacialPressure::State::Pushing);
			context.instance.Cancel(sas::AbilityEndReason::Completed);
		}
	}

	void GlacialPressureAbility::ResolveCollision(
		GameAbilityBehaviorContext& context,
		PushState& movingState,
		SpaceShip& movingTarget,
		SpaceShip& collidedTarget,
		float impactSpeed
	)
	{
		PushState* collidedState = FindPushState(&collidedTarget);
		const int cryoStacks = std::max(
			movingState.cryoStacks,
			collidedState ? collidedState->cryoStacks : 0
		);
		const float baseCollisionDamage = std::max(
			movingState.collisionDamage,
			collidedState ? collidedState->collisionDamage : 0.f
		);
		const float initialImpulseSpeed = std::max(
			0.001f,
			movingState.initialImpulseSpeed
		);
		const float speedRatio = std::clamp(
			std::max(0.f, impactSpeed) / initialImpulseSpeed,
			0.f,
			1.f
		);
		const float collisionDamage = baseCollisionDamage * (
			AbilityData::GlacialPressure::MinimumCollisionDamageMultiplier +
			(AbilityData::GlacialPressure::MaximumCollisionDamageMultiplier -
				AbilityData::GlacialPressure::MinimumCollisionDamageMultiplier) *
				speedRatio * speedRatio
		);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		const sas::GameplayAttributeList values = ResolveValues(context);
		const DamagePayload payload = BuildCryoPayload(
			damageTags,
			values,
			cryoStacks
		);
		SpaceShip* owner = dynamic_cast<SpaceShip*>(&context.owner);
		if (!owner)
		{
			return;
		}

		ApplyCombatDamage(
			movingTarget,
			collisionDamage,
			owner,
			damageTags,
			payload,
			sas::ContentId{ context.definition.abilityId },
			context.definition.abilityTags
		);
		ApplyCombatDamage(
			collidedTarget,
			collisionDamage,
			owner,
			damageTags,
			payload,
			sas::ContentId{ context.definition.abilityId },
			context.definition.abilityTags
		);
		ApplyStun(context, movingTarget, movingState.collisionStunDuration);
		ApplyStun(
			context,
			collidedTarget,
			collidedState
				? collidedState->collisionStunDuration
				: movingState.collisionStunDuration
		);
		EmitEvent(context, AbilityData::GlacialPressure::Event::Collision);
	}

	void GlacialPressureAbility::ApplyStun(
		GameAbilityBehaviorContext& context,
		SpaceShip& target,
		float duration
	) const
	{
		if (duration <= 0.f)
		{
			return;
		}
		const ControlResponse response = target.ResolveControlResponse(
			GameplayTags::State::Effect::Control::Stunned
		);
		if (response.mode == ControlResponseMode::Immune ||
			response.mode == ControlResponseMode::InterruptOnly)
		{
			return;
		}
		const sas::GameplayEffectDefinition* definition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::GlacialPressure::Effect::StunId
			);
		if (!definition)
		{
			return;
		}
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
		spec.duration = duration * std::max(0.f, response.durationMultiplier);
		spec.maxStacks = 1;
		if (spec.duration > 0.f)
		{
			target.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec,
				sas::GameplayEffectSourceContext{
					&context.owner,
					&context.instance
				}
			);
		}
	}

	GlacialPressureAbility::PushState* GlacialPressureAbility::FindPushState(
		SpaceShip* target
	)
	{
		for (PushState& state : mPushStates)
		{
			if (state.target.lock().get() == target)
			{
				return &state;
			}
		}
		return nullptr;
	}

	void GlacialPressureAbility::EmitEvent(
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
