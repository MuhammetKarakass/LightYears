#include "gameplay/ability/cryostasis/CryostasisAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
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
		constexpr std::size_t RequiredAttributeCount = 15;

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

		DamagePayload BuildCryoPayload(
			const List<GameplayTag>& damageTags,
			const sas::GameplayAttributeList& values,
			int stacks
		)
		{
			DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
			// Cryostasis applies the established Cryo status contract; it only
			// selects whether this hit contributes one field stack or four break
			// stacks. Slow mechanics remain in DamageTypeSystem.
			payload.cryoBuildupPerHit = std::clamp(stacks, 0, 4);
			payload.cryoBuildupRequired = 4;
			payload.cryoBuildupDuration = 2.5f;
			payload.cryoSlowPercent = 0.25f;
			payload.cryoSlowDuration = 1.5f;
			return payload;
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
			definition.damageTags.size() != 1 ||
			!definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Cryo) ||
			definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Cryostasis requires its duration lifecycle, Cryo identity, fifteen runtime attributes, and fourteen progression steps.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::Cryostasis::Attribute::Radius,
			AbilityData::Cryostasis::Attribute::FieldTickDamage,
			AbilityData::Cryostasis::Attribute::FieldTickInterval,
			AbilityData::Cryostasis::Attribute::FieldTickDamageMaxHealthScale,
			AbilityData::Cryostasis::Attribute::BaseIceHealth,
			AbilityData::Cryostasis::Attribute::IceHealthMaxHealthScale,
			AbilityData::Cryostasis::Attribute::BaseHealthRegenPerSecond,
			AbilityData::Cryostasis::Attribute::HealthRegenMaxHealthScale,
			AbilityData::Cryostasis::Attribute::BaseEnergyRegenPerSecond,
			AbilityData::Cryostasis::Attribute::EnergyRegenMaxHealthScale,
			AbilityData::Cryostasis::Attribute::BaseBreakDamage,
			AbilityData::Cryostasis::Attribute::BreakDamageMaxIceHealthScale,
			AbilityData::Cryostasis::Attribute::FieldCryoStacks,
			AbilityData::Cryostasis::Attribute::BreakCryoStacks,
			AbilityData::Cryostasis::Attribute::BreakCooldownMultiplier
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
			FindValue(mResolvedValues, AbilityData::Cryostasis::Attribute::BaseIceHealth, 200.f) +
				maximumHealth * std::max(0.f, FindValue(
					mResolvedValues,
					AbilityData::Cryostasis::Attribute::IceHealthMaxHealthScale,
					0.40f
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

		mDamageTags = context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		mFieldTickAccumulator = 0.f;
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
			FindValue(mResolvedValues, AbilityData::Cryostasis::Attribute::BaseHealthRegenPerSecond, 8.f) +
				maximumHealth * std::max(0.f, FindValue(mResolvedValues,
					AbilityData::Cryostasis::Attribute::HealthRegenMaxHealthScale, 0.04f)));
		const float energyRegen = std::max(0.f,
			FindValue(mResolvedValues, AbilityData::Cryostasis::Attribute::BaseEnergyRegenPerSecond, 4.f) +
				maximumHealth * std::max(0.f, FindValue(mResolvedValues,
					AbilityData::Cryostasis::Attribute::EnergyRegenMaxHealthScale, 0.01f)));
		ship->GetHealthComponent().Regenerate(healthRegen * safeDeltaTime);
		ship->GetEnergyComponent().ClearRechargeDelay();
		ship->GetEnergyComponent().Tick(safeDeltaTime, energyRegen, true);
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

		const float interval = std::max(0.001f, FindValue(
			mResolvedValues,
			AbilityData::Cryostasis::Attribute::FieldTickInterval,
			0.25f
		));
		const int tickCount = time::ConsumePeriodicTicks(
			mFieldTickAccumulator,
			safeDeltaTime,
			interval
		);
		for (int tickIndex = 0; tickIndex < tickCount; ++tickIndex)
		{
			ApplyFieldTick(context);
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
		if (mIceBroken)
		{
			TriggerBreakExplosion(context);
		}
		else if (reason == sas::AbilityEndReason::Cancelled)
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

	float CryostasisAbility::ResolveCooldownDurationOnEnd(
		GameAbilityBehaviorContext&,
		sas::AbilityEndReason,
		float resolvedCooldown
	)
	{
		return mIceBroken
			? resolvedCooldown * std::clamp(FindValue(
				mResolvedValues,
				AbilityData::Cryostasis::Attribute::BreakCooldownMultiplier,
				0.5f
			), 0.f, 1.f)
			: resolvedCooldown;
	}

	void CryostasisAbility::ApplyFieldTick(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return;
		}
		const float radius = std::max(0.f, FindValue(
			mResolvedValues,
			AbilityData::Cryostasis::Attribute::Radius,
			300.f
		));
		const float maximumHealth = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::MaxHealth)
		);
		const float damage = std::max(0.f, FindValue(
			mResolvedValues,
			AbilityData::Cryostasis::Attribute::FieldTickDamage,
			4.f
		) + maximumHealth * std::max(0.f, FindValue(
			mResolvedValues,
			AbilityData::Cryostasis::Attribute::FieldTickDamageMaxHealthScale,
			0.02f
		)));
		const int stacks = static_cast<int>(std::lround(FindValue(
			mResolvedValues,
			AbilityData::Cryostasis::Attribute::FieldCryoStacks,
			1.f
		)));
		const DamagePayload payload = BuildCryoPayload(mDamageTags, mResolvedValues, stacks);
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			context.owner,
			radius
		))
		{
			if (target && !target->GetIsPendingDestroy())
			{
				ApplyCombatDamage(
					*target, damage, &context.owner, mDamageTags, payload,
					sas::ContentId{ context.definition.abilityId },
					context.definition.abilityTags, DamageDeliveryType::Area
				);
			}
		}
	}

	void CryostasisAbility::TriggerBreakExplosion(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return;
		}
		const float radius = std::max(0.f, FindValue(
			mResolvedValues, AbilityData::Cryostasis::Attribute::Radius, 300.f
		));
		const float damage = std::max(0.f, FindValue(
			mResolvedValues, AbilityData::Cryostasis::Attribute::BaseBreakDamage, 70.f
		) + mMaximumIceHealth * std::max(0.f, FindValue(
			mResolvedValues,
			AbilityData::Cryostasis::Attribute::BreakDamageMaxIceHealthScale,
			0.30f
		)));
		const int stacks = static_cast<int>(std::lround(FindValue(
			mResolvedValues, AbilityData::Cryostasis::Attribute::BreakCryoStacks, 4.f
		)));
		const DamagePayload payload = BuildCryoPayload(mDamageTags, mResolvedValues, stacks);
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world, context.owner, radius
		))
		{
			if (target && !target->GetIsPendingDestroy())
			{
				ApplyCombatDamage(
					*target, damage, &context.owner, mDamageTags, payload,
					sas::ContentId{ context.definition.abilityId },
					context.definition.abilityTags, DamageDeliveryType::Area
				);
			}
		}
		// IceBroken is emitted by the shell effect when it consumes the breaking
		// hit. Re-emitting it here would make this explosion look like a second
		// break to every gameplay-event listener.
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
		mFieldTickAccumulator = 0.f;
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
