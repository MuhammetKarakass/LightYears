#include "gameplay/ability/infernoSpray/InfernoSprayAbility.h"
#include "gameplay/content/AbilityContentCatalog.h"

#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "framework/Actor.h"

#include <variant>

namespace ly
{
	namespace
	{
		float ResolveInfernoSetting(
			const std::string& abilityId,
			const std::string& settingName,
			float fallback
		)
		{
			return content::AbilityContentCatalog::FindNumericSetting(abilityId, settingName)
				.value_or(fallback);
		}

		void EmitLifecycleEvent(
			LightYearsAbilitySystemComponent& abilitySystem,
			Actor& owner,
			const GameplayTag& eventTag)
		{
			sas::AbilityEvent event;
			event.eventTag = eventTag;
			event.SetSource(&owner);
			event.SetTarget(&owner);
			abilitySystem.HandleGameplayEvent(event);
		}
	}

	bool InfernoSprayAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		if (!definition.FindEffectSpec(AbilityData::InfernoSpray::MovementPenaltyEffectId))
		{
			if (failureReason)
			{
				*failureReason = "Inferno Spray must own its movement penalty effect values.";
			}
			return false;
		}
		for (const AbilityActionSpec& action : definition.actions)
		{
			if (action.phase == sas::AbilityActionPhase::OnActivate &&
				std::holds_alternative<SpawnActorAction>(action.action))
			{
				return true;
			}
		}

		if (failureReason)
		{
			*failureReason = "Inferno Spray abilities require an OnActivate actor spawn action.";
		}
		return false;
	}

	bool InfernoSprayAbility::Activate(GameAbilityBehaviorContext& context)
	{
		mActiveTime = 0.f;
		mCancelAvailableFired = false;

		const sas::GameplayEffectDefinition* effectDef =
			EffectData::FindGameplayEffectDefinition(AbilityData::InfernoSpray::MovementPenaltyEffectId);
		if (effectDef)
		{
			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*effectDef);
			if (const AbilityEffectSpecDefinition* sourceSpec =
				context.definition.FindEffectSpec(AbilityData::InfernoSpray::MovementPenaltyEffectId))
			{
				spec.duration = sourceSpec->useAbilityDuration
					? context.definition.duration
					: sourceSpec->duration.value_or(spec.duration);
				spec.maxStacks = sourceSpec->maxStacks.value_or(spec.maxStacks);
				spec.modifiers = sourceSpec->modifiers;
				spec.attributes = sourceSpec->attributes;
			}
			mMovementPenaltyEffectHandle = context.abilitySystem.ApplyGameplayEffect(spec);
		}

		context.abilitySystem.AddOwnedTag(AbilityData::InfernoSpray::StateTag);
		EmitLifecycleEvent(
			context.abilitySystem,
			context.owner,
			AbilityData::InfernoSpray::StartEvent
		);

		mStarted = true;
		return true;
	}

	void InfernoSprayAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
	{
		if (!mStarted)
		{
			return;
		}

		mActiveTime += deltaTime;
		const float minCancelDuration = ResolveInfernoSetting(
			context.definition.abilityId,
			"minCancelDuration",
			0.f
		);

		if (mActiveTime >= minCancelDuration &&
			!mCancelAvailableFired)
		{
			mCancelAvailableFired = true;
			EmitLifecycleEvent(
				context.abilitySystem,
				context.owner,
				AbilityData::InfernoSpray::CancelAvailableEvent
			);
		}

		if (context.instance.IsPressedThisFrame())
		{
			if (mActiveTime >= minCancelDuration)
			{
				context.instance.Cancel(sas::AbilityEndReason::Cancelled);
			}
		}
	}

	void InfernoSprayAbility::End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason)
	{
		if (!mStarted)
		{
			return;
		}

		if (mMovementPenaltyEffectHandle.IsValid())
		{
			context.abilitySystem.RemoveGameplayEffect(mMovementPenaltyEffectHandle);
			mMovementPenaltyEffectHandle = {};
		}

		context.abilitySystem.RemoveOwnedTag(AbilityData::InfernoSpray::StateTag);

		if (reason == sas::AbilityEndReason::Cancelled)
		{
			EmitLifecycleEvent(
				context.abilitySystem,
				context.owner,
				AbilityData::InfernoSpray::CancelledEvent
			);
		}
		else
		{
			EmitLifecycleEvent(
				context.abilitySystem,
				context.owner,
				AbilityData::InfernoSpray::CompletedEvent
			);
		}

		EmitLifecycleEvent(
			context.abilitySystem,
			context.owner,
			AbilityData::InfernoSpray::EndEvent
		);

		mStarted = false;
	}
}
