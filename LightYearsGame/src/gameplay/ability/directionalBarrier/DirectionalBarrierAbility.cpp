#include "gameplay/ability/directionalBarrier/DirectionalBarrierAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierVisualActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationIds.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sas::GameplayAttributeList ResolveValues(
			GameAbilityBehaviorContext& context
		)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}
	}

	bool DirectionalBarrierAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::Toggle ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 1.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Directional Barrier requires Toggle + Duration, one charge, and a duration above the shared one-second toggle window.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::DirectionalBarrier::Attribute::MaxHealthReference,
			AbilityData::DirectionalBarrier::Attribute::MaxHealthDurationScale,
			AbilityData::DirectionalBarrier::Attribute::MovementSpeedMultiplier
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason =
						"Directional Barrier must declare all runtime attributes.";
				}
				return false;
			}
		}

		const float maxHealthReference = sas::FindAttributeValue(
			definition.attributes,
			AbilityData::DirectionalBarrier::Attribute::MaxHealthReference,
			-1.f
		);
		const float maxHealthScale = sas::FindAttributeValue(
			definition.attributes,
			AbilityData::DirectionalBarrier::Attribute::MaxHealthDurationScale,
			-1.f
		);
		const float movementMultiplier = sas::FindAttributeValue(
			definition.attributes,
			AbilityData::DirectionalBarrier::Attribute::MovementSpeedMultiplier,
			-1.f
		);
		if (!IsFiniteNonNegative(maxHealthReference) ||
			!IsFiniteNonNegative(maxHealthScale) ||
			movementMultiplier <= 0.f || movementMultiplier > 1.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Directional Barrier contains an invalid MaxHealth scaling or movement multiplier.";
			}
			return false;
		}

		const sas::GameplayEffectDefinition* effect =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::DirectionalBarrier::Effect::ActiveEffectId
			);
		if (!effect || effect->durationPolicy != sas::GameplayEffectDurationPolicy::Duration)
		{
			if (failureReason)
			{
				*failureReason = "Directional Barrier requires a duration gameplay effect.";
			}
			return false;
		}

		return true;
	}

	float DirectionalBarrierAbility::ResolveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		GameAbilityBehaviorContext mutableContext{
			const_cast<LightYearsAbilitySystemComponent&>(context.abilitySystem),
			const_cast<GameAbility&>(context.instance),
			const_cast<Actor&>(context.owner),
			context.definition
		};
		const sas::GameplayAttributeList values = ResolveValues(mutableContext);
		const float reference = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::DirectionalBarrier::Attribute::MaxHealthReference,
				100.f
			)
		);
		const float scale = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::DirectionalBarrier::Attribute::MaxHealthDurationScale,
				0.0025f
			)
		);
		const float maxHealth = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::MaxHealth
			)
		);
		return std::max(
			0.f,
			defaultDuration + std::max(0.f, maxHealth - reference) * scale
		);
	}

	float DirectionalBarrierAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		return ResolveDuration(context, defaultDuration);
	}

	bool DirectionalBarrierAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || mActive)
		{
			return false;
		}

		const sas::GameplayEffectDefinition* effectDefinition =
			EffectData::FindGameplayEffectDefinition(
				AbilityData::DirectionalBarrier::Effect::ActiveEffectId
			);
		if (!effectDefinition)
		{
			return false;
		}

		const float duration = context.instance.GetActiveDuration();
		if (duration <= 1.f)
		{
			return false;
		}

		sas::GameplayEffectSpec effectSpec =
			sas::MakeGameplayEffectSpec(*effectDefinition);
		effectSpec.duration = duration;
		effectSpec.maxStacks = 1;
		mActiveEffectHandle = context.abilitySystem.ApplyGameplayEffect(
			effectSpec,
			sas::GameplayEffectSourceContext{
				&context.owner,
				&context.instance
			}
		);
		if (!mActiveEffectHandle.IsValid())
		{
			return false;
		}

		GameAbilityBehaviorContext mutableContext = context;
		const sas::GameplayAttributeList values = ResolveValues(mutableContext);
		const float movementMultiplier = std::clamp(
			sas::FindAttributeValue(
				values,
				AbilityData::DirectionalBarrier::Attribute::MovementSpeedMultiplier,
				0.80f
			),
			0.f,
			1.f
		);
		ship->GetRuntimeModifiers().Set(
			context.definition.abilityId,
			ShipRuntimeModifier{ movementMultiplier }
		);

		if (World* world = context.owner.GetWorld())
		{
			if (const DirectionalBarrierPresentationProfile* profile =
				PresentationProfileRegistry<DirectionalBarrierPresentationProfile>::Find(
					DirectionalBarrierPresentationIds::Basic
				))
			{
				mVisualActor = world->SpawnActor<DirectionalBarrierVisualActor>(
					&context.owner,
					*profile
				);
			}
		}

		mActive = true;
		EmitEvent(context, AbilityData::DirectionalBarrier::Event::Started);
		return true;
	}

	void DirectionalBarrierAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}

		if (mActiveEffectHandle.IsValid())
		{
			context.abilitySystem.RemoveGameplayEffect(mActiveEffectHandle);
			mActiveEffectHandle = {};
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(context.definition.abilityId);
		}
		if (const shared_ptr<DirectionalBarrierVisualActor> visual = mVisualActor.lock())
		{
			visual->Destroy();
		}
		mVisualActor.reset();
		mActive = false;
		EmitEvent(context, AbilityData::DirectionalBarrier::Event::Ended);
	}

	void DirectionalBarrierAbility::OnOwnerAbilityActivated(
		GameAbilityBehaviorContext& context,
		const sas::AbilityLifecycleEvent& event
	)
	{
		if (!mActive || event.abilityId == context.definition.abilityId)
		{
			return;
		}

		const bool isMovementAbility = std::any_of(
			event.abilityTags.begin(),
			event.abilityTags.end(),
			[](const GameplayTag& tag)
			{
				return tag.MatchesTag(GameplayTags::Ability::Movement);
			}
		);
		if (isMovementAbility)
		{
			// System interruption intentionally bypasses the one-second manual
			// Toggle release rule: movement abilities must always be allowed to
			// terminate a conflicting directional defense immediately.
			context.instance.Cancel(sas::AbilityEndReason::Interrupted);
		}
	}

	void DirectionalBarrierAbility::EmitEvent(
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
