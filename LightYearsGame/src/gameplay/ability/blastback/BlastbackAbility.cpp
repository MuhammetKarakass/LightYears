#include "gameplay/ability/blastback/BlastbackAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameConfigs/ability/offensive/BlastbackConfig.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/blastback/BlastbackBurstActor.h"
#include "gameplay/ability/blastback/BlastbackContracts.h"
#include "gameplay/ability/blastback/BlastbackFocusTelegraphActor.h"
#include "gameplay/ability/runtime/FocusActionLocks.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/blastback/BlastbackPresentationIds.h"
#include "presentation/ability/blastback/BlastbackPresentationProfile.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

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
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(
				executionContext
			);
		}

		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}
	}

	bool BlastbackAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::Blastback::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || !IsFinite(definition.cooldown) ||
			definition.cooldown <= 0.f || !IsFinite(definition.duration) ||
			definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Blastback requires pressed activation, positive focus duration, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::Blastback::Attribute::Damage,
			AbilityData::Blastback::Attribute::Range,
			AbilityData::Blastback::Attribute::InnerRange,
			AbilityData::Blastback::Attribute::ConeHalfAngleDegrees,
			AbilityData::Blastback::Attribute::InnerDamageMultiplier,
			AbilityData::Blastback::Attribute::InnerIgniteStacks,
			AbilityData::Blastback::Attribute::OuterIgniteStacks,
			AbilityData::Blastback::Attribute::InnerStunDuration,
			AbilityData::Blastback::Attribute::OuterStunDuration,
			AbilityData::Blastback::Attribute::MaxHealthReference,
			AbilityData::Blastback::Attribute::MaxHealthStunScale,
			AbilityData::Blastback::Attribute::MinimumPushInitialSpeed,
			AbilityData::Blastback::Attribute::MaximumPushInitialSpeed,
			AbilityData::Blastback::Attribute::InnerPushMultiplier,
			AbilityData::Blastback::Attribute::RecoilInitialSpeed,
			AbilityData::Blastback::Attribute::RecoilDuration,
			AbilityData::Blastback::Attribute::BurnDamagePerSecond,
			AbilityData::Blastback::Attribute::BurnDuration,
			AbilityData::Blastback::Attribute::BurnMaxStacks
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
						"Blastback must declare every cone, recoil, and Thermal runtime attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return FindValue(definition.attributes, id, 0.f);
		};
		if (value(AbilityData::Blastback::Attribute::Damage) < 0.f ||
			value(AbilityData::Blastback::Attribute::Range) <= 0.f ||
			value(AbilityData::Blastback::Attribute::InnerRange) <= 0.f ||
			value(AbilityData::Blastback::Attribute::InnerRange) >=
				value(AbilityData::Blastback::Attribute::Range) ||
			value(AbilityData::Blastback::Attribute::ConeHalfAngleDegrees) <= 0.f ||
			value(AbilityData::Blastback::Attribute::ConeHalfAngleDegrees) >= 90.f ||
			value(AbilityData::Blastback::Attribute::InnerDamageMultiplier) < 1.f ||
			value(AbilityData::Blastback::Attribute::InnerIgniteStacks) < 1.f ||
			value(AbilityData::Blastback::Attribute::OuterIgniteStacks) < 1.f ||
			value(AbilityData::Blastback::Attribute::InnerStunDuration) < 0.f ||
			value(AbilityData::Blastback::Attribute::OuterStunDuration) < 0.f ||
			value(AbilityData::Blastback::Attribute::MinimumPushInitialSpeed) < 0.f ||
			value(AbilityData::Blastback::Attribute::MaximumPushInitialSpeed) <
				value(AbilityData::Blastback::Attribute::MinimumPushInitialSpeed) ||
			value(AbilityData::Blastback::Attribute::InnerPushMultiplier) < 1.f ||
			value(AbilityData::Blastback::Attribute::RecoilInitialSpeed) < 0.f ||
			value(AbilityData::Blastback::Attribute::RecoilDuration) <= 0.f ||
			definition.levelProgression.size() != 14 ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Thermal ||
			!PresentationProfileRegistry<BlastbackPresentationProfile>::Find(
				BlastbackPresentationIds::Basic
			))
		{
			if (failureReason)
			{
				*failureReason =
					"Blastback has invalid cone/recoil values or no registered typed presentation profile.";
			}
			return false;
		}
		return true;
	}

	bool BlastbackAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (!dynamic_cast<SpaceShip*>(&context.owner) || mDischarged)
		{
			return false;
		}

		mFocusElapsed = 0.f;
		mDischarged = false;
		mFocusLocksApplied = true;
		ability::ApplyFocusActionLocks(context.abilitySystem);
		context.abilitySystem.AddOwnedTag(AbilityData::Blastback::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (const BlastbackPresentationProfile* profile =
				PresentationProfileRegistry<BlastbackPresentationProfile>::Find(
					BlastbackPresentationIds::Basic
				))
			{
				mTelegraph = world->SpawnActor<BlastbackFocusTelegraphActor>(
					&context.owner,
					*profile
				);
			}
		}
		EmitEvent(context, AbilityData::Blastback::Event::Started);
		return true;
	}

	void BlastbackAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
	{
		if (mDischarged)
		{
			return;
		}
		const float focusDuration = std::max(0.001f, context.definition.duration);
		mFocusElapsed = std::min(
			focusDuration,
			mFocusElapsed + std::max(0.f, deltaTime)
		);
		if (const shared_ptr<BlastbackFocusTelegraphActor> telegraph = mTelegraph.lock())
		{
			// The telegraph can finish expanding before the focus action ends. The
			// remaining focus time intentionally holds the fully authored area on
			// screen, giving the player a short maximum-range warning before blast.
			const BlastbackPresentationProfile* profile =
				PresentationProfileRegistry<BlastbackPresentationProfile>::Find(
					BlastbackPresentationIds::Basic
				);
			const float expansionDuration = std::min(
				focusDuration,
				std::max(0.001f, profile ? profile->focusExpansionDuration : focusDuration)
			);
			telegraph->SetProgress(mFocusElapsed / expansionDuration);
		}
		if (mFocusElapsed >= focusDuration)
		{
			Discharge(context);
		}
	}

	void BlastbackAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (mFocusLocksApplied)
		{
			ability::RemoveFocusActionLocks(context.abilitySystem);
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::Blastback::State::Focusing);
		if (const shared_ptr<BlastbackFocusTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTelegraph.reset();
		mFocusElapsed = 0.f;
		mDischarged = false;
		mFocusLocksApplied = false;
		EmitEvent(context, AbilityData::Blastback::Event::Ended);
	}

	void BlastbackAbility::Discharge(GameAbilityBehaviorContext& context)
	{
		if (mDischarged)
		{
			return;
		}
		mDischarged = true;

		SpaceShip* owner = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		const BlastbackPresentationProfile* profile =
			PresentationProfileRegistry<BlastbackPresentationProfile>::Find(
				BlastbackPresentationIds::Basic
			);
		if (!owner || !world || !profile)
		{
			context.instance.Cancel(sas::AbilityEndReason::Interrupted);
			return;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float range = std::max(1.f, FindValue(
			values,
			AbilityData::Blastback::Attribute::Range,
			AbilityData::Blastback::DefaultRange
		));
		const float innerRange = std::clamp(
			FindValue(
				values,
				AbilityData::Blastback::Attribute::InnerRange,
				AbilityData::Blastback::DefaultInnerRange
			),
			0.f,
			range
		);
		const float halfAngleRadians = FindValue(
			values,
			AbilityData::Blastback::Attribute::ConeHalfAngleDegrees,
			AbilityData::Blastback::DefaultConeHalfAngleDegrees
		) * 3.14159265358979323846f / 180.f;
		const float baseDamage = std::max(0.f, FindValue(
			values,
			AbilityData::Blastback::Attribute::Damage,
			0.f
		));
		const float innerDamageMultiplier = std::max(1.f, FindValue(
			values,
			AbilityData::Blastback::Attribute::InnerDamageMultiplier,
			2.f
		));
		const float minimumPushSpeed = std::max(0.f, FindValue(
			values,
			AbilityData::Blastback::Attribute::MinimumPushInitialSpeed,
			240.f
		));
		const float maximumPushSpeed = std::max(minimumPushSpeed, FindValue(
			values,
			AbilityData::Blastback::Attribute::MaximumPushInitialSpeed,
			760.f
		));
		const float innerPushMultiplier = std::max(1.f, FindValue(
			values,
			AbilityData::Blastback::Attribute::InnerPushMultiplier,
			1.2f
		));
		const int innerIgniteStacks = std::clamp(static_cast<int>(std::lround(
			FindValue(values, AbilityData::Blastback::Attribute::InnerIgniteStacks, 4.f)
		)), 1, 4);
		const int outerIgniteStacks = std::clamp(static_cast<int>(std::lround(
			FindValue(values, AbilityData::Blastback::Attribute::OuterIgniteStacks, 2.f)
		)), 1, 4);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		const sf::Vector2f origin = owner->GetActorLocation();
		const sf::Vector2f forward = owner->GetActorForwardDirection();

		targeting::TargetingQuery query;
		query.source = owner;
		query.origin = origin;
		query.range = range;
		query.shape = targeting::TargetingShape::Cone;
		query.direction = forward;
		query.coneHalfAngleRadians = std::clamp(
			halfAngleRadians,
			0.001f,
			1.55334306f
		);
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
			const bool innerZone = distance <= innerRange;
			DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
			payload.igniteStacks = innerZone ? innerIgniteStacks : outerIgniteStacks;
			payload.burnDamagePerSecond = std::max(0.f, FindValue(
				values,
				AbilityData::Blastback::Attribute::BurnDamagePerSecond,
				1.f
			));
			payload.burnDuration = std::max(0.f, FindValue(
				values,
				AbilityData::Blastback::Attribute::BurnDuration,
				3.f
			));
			payload.burnMaxStacks = std::max(1, static_cast<int>(std::lround(
				FindValue(
					values,
					AbilityData::Blastback::Attribute::BurnMaxStacks,
					4.f
				)
			)));
			ApplyCombatDamage(
				*target,
				baseDamage * (innerZone ? innerDamageMultiplier : 1.f),
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

			const sf::Vector2f delta = target->GetActorLocation() - origin;
			const float deltaLength = GetVectorLength(delta);
			const sf::Vector2f pushDirection = deltaLength > 0.001f
				? delta / deltaLength
				: forward;
			const float distanceRatio = std::clamp(distance / range, 0.f, 1.f);
			float pushSpeed = maximumPushSpeed -
				(maximumPushSpeed - minimumPushSpeed) * distanceRatio;
			if (innerZone)
			{
				pushSpeed *= innerPushMultiplier;
			}
			movement::MovementInfluenceService::ApplyImpulse(
				*target,
				movement::ImpulseRequest{
					pushDirection * pushSpeed,
					target->GetMovementComponent()
						.GetAttributes().linearDamping.currentValue
				}
			);
			ApplyStun(
				context,
				*target,
				FindValue(
					values,
					innerZone
						? AbilityData::Blastback::Attribute::InnerStunDuration
						: AbilityData::Blastback::Attribute::OuterStunDuration,
					innerZone ? 1.f : 0.5f
				)
			);
		}

		if (const shared_ptr<BlastbackFocusTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTelegraph.reset();
		context.abilitySystem.RemoveOwnedTag(
			GameplayTags::State::ActionLock::PrimaryWeaponFire
		);
		context.abilitySystem.RemoveOwnedTag(AbilityData::Blastback::State::Focusing);
		mFocusLocksApplied = false;

		// Recoil is a one-shot physical impulse, not a forced offset. It therefore
		// retains momentum and decays through the ship's normal influence damping
		// after the burst actor has cleaned up its short input-lock window.
		movement::MovementInfluenceService::ApplyImpulse(
			*owner,
			movement::ImpulseRequest{
				forward * -std::max(0.f, FindValue(
					values,
					AbilityData::Blastback::Attribute::RecoilInitialSpeed,
					800.f
				)),
				owner->GetMovementComponent().GetAttributes().linearDamping.currentValue
			}
		);

		// The burst actor owns the short post-blast locks.  If spawning ever
		// fails, release those locks here so the player cannot remain input-locked.
		const weak_ptr<BlastbackBurstActor> recoilActor = world->SpawnActor<BlastbackBurstActor>(
			owner,
			origin,
			forward,
			*profile,
			FindValue(
				values,
				AbilityData::Blastback::Attribute::RecoilDuration,
				AbilityData::Blastback::DefaultRecoilDuration
			)
		);
		if (recoilActor.expired())
		{
			context.abilitySystem.RemoveOwnedTag(
				GameplayTags::State::ActionLock::AbilityActivation
			);
			context.abilitySystem.RemoveOwnedTag(
				GameplayTags::State::ActionLock::MovementInput
			);
		}
		EmitEvent(context, AbilityData::Blastback::Event::Blasted);
		// The generic ability ends now, so normal cooldown calculation begins on
		// the blast frame while BlastbackBurstActor finishes the recoil window.
		context.instance.Cancel(sas::AbilityEndReason::Completed);
	}

	void BlastbackAbility::ApplyStun(
		GameAbilityBehaviorContext& context,
		SpaceShip& target,
		float baseDuration
	) const
	{
		const ControlResponse response = target.ResolveControlResponse(
			GameplayTags::State::Effect::Control::Stunned
		);
		if (baseDuration <= 0.f || response.mode == ControlResponseMode::Immune ||
			response.mode == ControlResponseMode::InterruptOnly)
		{
			return;
		}
		const sas::GameplayEffectDefinition* stunDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::Blastback::Effect::StunId);
		if (!stunDefinition)
		{
			return;
		}
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float maxHealth = std::max(0.f, context.abilitySystem.GetAttributes()
			.GetCurrentValue(OwnerAttributeIds::MaxHealth));
		const float reference = std::max(0.f, FindValue(
			values,
			AbilityData::Blastback::Attribute::MaxHealthReference,
			100.f
		));
		const float bonus = std::max(0.f, maxHealth - reference) * std::max(
			0.f,
			FindValue(
				values,
				AbilityData::Blastback::Attribute::MaxHealthStunScale,
				0.0005f
			)
		);
		sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*stunDefinition);
		spec.duration = (baseDuration + bonus) * std::max(
			0.f,
			response.durationMultiplier
		);
		spec.maxStacks = 1;
		if (spec.duration > 0.f)
		{
			target.GetAbilitySystemComponent().ApplyGameplayEffect(
				spec,
				sas::GameplayEffectSourceContext{ &context.owner, &context.instance }
			);
		}
	}

	void BlastbackAbility::EmitEvent(
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
