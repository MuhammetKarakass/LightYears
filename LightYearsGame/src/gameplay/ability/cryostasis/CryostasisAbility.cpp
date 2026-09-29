#include "gameplay/ability/cryostasis/CryostasisAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/ability/cryostasis/CryostasisVisualActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/cryostasis/CryostasisPresentationIds.h"
#include "presentation/ability/cryostasis/CryostasisPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		// Duration and cooldown live in GameAbilityDefinition. Every item below is
		// a value this family resolves at runtime, so the contract has one source
		// of truth and cannot duplicate definition-level data.
		constexpr std::size_t RequiredAttributeCount = 7;

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

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

	}

	bool CryostasisAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::Cryostasis::AbilityId::Basic ||
			definition.behaviorType != AbilityBehaviorType::Cryostasis ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.duration <= 0.f ||
			definition.cooldown <= 0.f || definition.attributes.size() != RequiredAttributeCount ||
			(definition.levelProgression.empty() && definition.repeatingLevelProgression.empty()))
		{
			if (failureReason)
			{
				*failureReason = "Cryostasis requires its duration lifecycle, seven runtime attributes, and fourteen progression steps.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::Cryostasis::Attribute::BaseIceHealth,
			AbilityData::Cryostasis::Attribute::IceHealthMaxHealthScale,
			AbilityData::Cryostasis::Attribute::BaseHealthRegenPerSecond,
			AbilityData::Cryostasis::Attribute::HealthRegenMaxHealthScale,
			AbilityData::Cryostasis::Attribute::BaseAfterburnerRecoveryPerSecond,
			AbilityData::Cryostasis::Attribute::AfterburnerRecoveryEnergyPowerScale,
			AbilityData::Cryostasis::Attribute::EnergyPowerReference
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue) ||
				attribute->baseValue < attribute->minValue)
			{
				if (failureReason)
				{
					*failureReason = "Cryostasis is missing a valid declared runtime attribute.";
				}
				return false;
			}
		}
		return EffectData::FindGameplayEffectDefinition(
			AbilityData::Cryostasis::Effect::IceShellId
		) != nullptr;
	}

	bool CryostasisAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		const sas::GameplayEffectDefinition* shellDefinition =
			EffectData::FindGameplayEffectDefinition(AbilityData::Cryostasis::Effect::IceShellId);
		if (!ship || !shellDefinition)
		{
			return false;
		}

		mResolvedValues = ResolveValues(context);
		const float maximumHealth = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::MaxHealth)
		);
		mMaximumIceHealth = std::max(
			1.f,
			FindValue(mResolvedValues, AbilityData::Cryostasis::Attribute::BaseIceHealth, 150.f) +
				maximumHealth * std::max(0.f, FindValue(
					mResolvedValues,
					AbilityData::Cryostasis::Attribute::IceHealthMaxHealthScale,
					0.50f
				))
		);
		sas::GameplayEffectSpec shellSpec = sas::MakeGameplayEffectSpec(*shellDefinition);
		shellSpec.duration = context.definition.duration;
		shellSpec.maxStacks = 1;
		shellSpec.attributes = {
			sas::GameplayAttribute{
				AbilityData::Cryostasis::Effect::IceHealth,
				mMaximumIceHealth,
				0.f,
				mMaximumIceHealth
			}
		};
		mIceShellHandle = context.abilitySystem.ApplyGameplayEffect(
			shellSpec,
			sas::GameplayEffectSourceContext{ &context.owner, &context.instance }
		);
		if (!mIceShellHandle.IsValid())
		{
			return false;
		}

		mIceBroken = false;
		mActive = true;
		// Cryostasis does not add a second shield-regeneration formula. It removes
		// the normal post-damage wait so the ship's existing shield regen begins
		// on the next simulation tick immediately after activation.
		ship->GetShieldComponent().ClearRechargeDelay();
		context.abilitySystem.AddOwnedTag(AbilityData::Cryostasis::State::Active);
		context.abilitySystem.AddOwnedTag(GameplayTagSchema::BlockAbilityActivation);
		context.abilitySystem.AddOwnedTag(GameplayTagSchema::BlockPrimaryWeaponFire);
		context.abilitySystem.AddOwnedTag(GameplayTagSchema::BlockMovementInput);
		context.abilitySystem.AddOwnedTag(GameplayTagSchema::BlockExternalMovement);
		if (World* world = context.owner.GetWorld())
		{
			if (const CryostasisPresentationProfile* profile =
				PresentationProfileRegistry<CryostasisPresentationProfile>::Find(
					CryostasisPresentationIds::Basic
				))
			{
				mVisualActor = world->SpawnActor<CryostasisVisualActor>(&context.owner, *profile);
			}
		}
		EmitEvent(context, AbilityData::Cryostasis::Event::Started);
		return true;
	}

	bool CryostasisAbility::OnInputPressed(GameAbilityBehaviorContext& context)
	{
		if (!mActive)
		{
			return false;
		}
		context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		return true;
	}

	void CryostasisAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
	{
		if (!mActive)
		{
			return;
		}

		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship)
		{
			return;
		}
		const float safeDeltaTime = std::max(0.f, deltaTime);
		const float maximumHealth = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::MaxHealth)
		);
		const float healthRegen = std::max(0.f,
			FindValue(mResolvedValues, AbilityData::Cryostasis::Attribute::BaseHealthRegenPerSecond, 5.f) +
				(maximumHealth / 100.f) * std::max(0.f, FindValue(mResolvedValues,
					AbilityData::Cryostasis::Attribute::HealthRegenMaxHealthScale, 5.f)));
		const float energyPower = std::max(0.f, context.abilitySystem.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::EnergyPower));
		const float afterburnerRecovery = std::max(0.f,
			FindValue(mResolvedValues, AbilityData::Cryostasis::Attribute::BaseAfterburnerRecoveryPerSecond, 4.f) +
				energyPower * std::max(0.f, FindValue(mResolvedValues,
					AbilityData::Cryostasis::Attribute::AfterburnerRecoveryEnergyPowerScale, 0.05f)));
		ship->GetHealthComponent().Regenerate(healthRegen * safeDeltaTime);
		ship->GetEnergyComponent().ClearRechargeDelay();
		ship->GetEnergyComponent().Tick(safeDeltaTime, afterburnerRecovery, true);
		if (const shared_ptr<CryostasisVisualActor> visual = mVisualActor.lock())
		{
			float iceHealthRatio = 0.f;
			if (const sas::ActiveGameplayEffect* shell =
				context.abilitySystem.FindGameplayEffect(mIceShellHandle))
			{
				if (const sas::GameplayAttribute* iceHealth = sas::FindAttribute(
					shell->runtimeAttributes, AbilityData::Cryostasis::Effect::IceHealth
				))
				{
					iceHealthRatio = iceHealth->maxValue > 0.f
						? iceHealth->currentValue / iceHealth->maxValue
						: 0.f;
				}
			}
			visual->SetIceHealthRatio(iceHealthRatio);
		}
	}

	void CryostasisAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		if (!mActive)
		{
			return;
		}
		if (reason == sas::AbilityEndReason::Cancelled && !mIceBroken)
		{
			EmitEvent(context, AbilityData::Cryostasis::Event::ManuallyEnded);
		}
		if (const shared_ptr<CryostasisVisualActor> visual = mVisualActor.lock())
		{
			if (mIceBroken)
			{
				visual->BeginBreak();
			}
			else
			{
				visual->Destroy();
			}
		}
		mVisualActor.reset();
		ClearRuntimeState(context);
		EmitEvent(context, AbilityData::Cryostasis::Event::Ended);
	}

	void CryostasisAbility::OnGameplayEvent(
		GameAbilityBehaviorContext& context,
		const sas::AbilityEvent& event
	)
	{
		if (!mActive || !event.eventTag.MatchesTagExact(AbilityData::Cryostasis::Event::IceBroken))
		{
			return;
		}
		mIceBroken = true;
		context.instance.Cancel(sas::AbilityEndReason::Interrupted);
	}

	void CryostasisAbility::ClearRuntimeState(GameAbilityBehaviorContext& context)
	{
		if (mIceShellHandle.IsValid())
		{
			context.abilitySystem.RemoveGameplayEffect(mIceShellHandle);
			mIceShellHandle = {};
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::Cryostasis::State::Active);
		context.abilitySystem.RemoveOwnedTag(GameplayTagSchema::BlockAbilityActivation);
		context.abilitySystem.RemoveOwnedTag(GameplayTagSchema::BlockPrimaryWeaponFire);
		context.abilitySystem.RemoveOwnedTag(GameplayTagSchema::BlockMovementInput);
		context.abilitySystem.RemoveOwnedTag(GameplayTagSchema::BlockExternalMovement);
		mActive = false;
	}

	void CryostasisAbility::EmitEvent(
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
