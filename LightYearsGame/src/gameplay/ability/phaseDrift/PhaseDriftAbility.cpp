#include "gameplay/ability/phaseDrift/PhaseDriftAbility.h"

#include "effects/GameplayEffectSpec.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/phaseDrift/PhaseDriftContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/phaseDrift/PhaseDriftPresentationIds.h"
#include "presentation/ability/phaseDrift/PhaseDriftPresentationProfile.h"
#include "gameplay/ability/phaseDrift/PhaseDriftVisualActor.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}

		sas::GameplayAttributeList ResolveValues(
			const GameAbilityBehaviorContext& context
		)
		{
			AbilityExecutionContext executionContext{
				const_cast<LightYearsAbilitySystemComponent*>(&context.abilitySystem),
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		float ResolveEnergyFactor(
			const LightYearsAbilitySystemComponent& abilitySystem,
			const sas::GameplayAttributeList& values
		)
		{
			const float energyScale = std::max(
				0.001f,
				FindValue(values, AbilityData::PhaseDrift::Attribute::EnergyScale, 1.f)
			);
			const float energyMax = std::max(
				0.f,
				abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyMax)
			);
			return std::clamp(1.f - std::exp(-energyMax / energyScale), 0.f, 1.f);
		}

		bool ApplyPolicyEffect(
			GameAbilityBehaviorContext& context,
			const char* effectId,
			float duration,
			List<sas::GameplayEffectHandle>& handles
		)
		{
			const sas::GameplayEffectDefinition* definition =
				EffectData::FindGameplayEffectDefinition(effectId);
			if (!definition)
			{
				return false;
			}

			sas::GameplayEffectSpec spec = sas::MakeGameplayEffectSpec(*definition);
			spec.duration = duration;
			spec.maxStacks = 1;
			const sas::GameplayEffectHandle handle =
				context.abilitySystem.ApplyGameplayEffect(
					spec,
					sas::GameplayEffectSourceContext{
						&context.owner,
						&context.instance
					}
				);
			if (!handle.IsValid())
			{
				return false;
			}
			handles.push_back(handle);
			return true;
		}
	}

	bool PhaseDriftAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::PhaseDrift::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Phase Drift requires a loadout slot, pressed activation, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::PhaseDrift::Attribute::MaximumMobilityDurationBonus,
			AbilityData::PhaseDrift::Attribute::MobilityScale,
			AbilityData::PhaseDrift::Attribute::MovementSpeedBonus,
			AbilityData::PhaseDrift::Attribute::ShieldRegenBonus,
			AbilityData::PhaseDrift::Attribute::AfterburnerRegenBonus,
			AbilityData::PhaseDrift::Attribute::EnergyScale,
			AbilityData::PhaseDrift::Attribute::MaximumEnergyShieldBonus,
			AbilityData::PhaseDrift::Attribute::MaximumEnergyAfterburnerBonus
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, required);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Phase Drift must declare all runtime attributes.";
				}
				return false;
			}
		}

		if (FindAttribute(definition, AbilityData::PhaseDrift::Attribute::MobilityScale)->baseValue <= 0.f ||
			FindAttribute(definition, AbilityData::PhaseDrift::Attribute::EnergyScale)->baseValue <= 0.f ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::MaximumMobilityDurationBonus)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::MovementSpeedBonus)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::ShieldRegenBonus)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::AfterburnerRegenBonus)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::MaximumEnergyShieldBonus)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::MaximumEnergyAfterburnerBonus)->baseValue))
		{
			if (failureReason)
			{
				*failureReason = "Phase Drift contains an invalid duration, mobility or regeneration value.";
			}
			return false;
		}

		for (const char* effectId : {
			AbilityData::PhaseDrift::Effect::MovementBoostId,
			AbilityData::PhaseDrift::Effect::ShieldRecoveryId,
			AbilityData::PhaseDrift::Effect::AfterburnerRecoveryId
		})
		{
			const sas::GameplayEffectDefinition* effect =
				EffectData::FindGameplayEffectDefinition(effectId);
			if (!effect || effect->durationPolicy != sas::GameplayEffectDurationPolicy::Duration)
			{
				if (failureReason)
				{
					*failureReason = "Phase Drift requires three reusable duration effects.";
				}
				return false;
			}
		}
		return true;
	}

	float PhaseDriftAbility::ResolveDuration(
		const GameAbilityBehaviorContext& context
	) const
	{
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float mobilityScale = std::max(
			0.001f,
			FindValue(values, AbilityData::PhaseDrift::Attribute::MobilityScale, 20.f)
		);
		const float mobilityRating = std::max(
			0.f,
			(
				context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::MoveSpeedHorizontal) +
				context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::MoveSpeedVertical)
			) * 0.5f
		);
		const float mobilityFactor = std::clamp(
			1.f - std::exp(-mobilityRating / mobilityScale),
			0.f,
			1.f
		);
		return std::max(
			0.f,
			context.definition.duration +
				FindValue(
					values,
					AbilityData::PhaseDrift::Attribute::MaximumMobilityDurationBonus,
					0.f
				) * mobilityFactor
		);
	}

	float PhaseDriftAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		(void)defaultDuration;
		return ResolveDuration(context);
	}

	bool PhaseDriftAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || mActive)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		mResolvedDuration = ResolveDuration(context);
		if (mResolvedDuration <= 0.f)
		{
			return false;
		}

		// Cleanse before applying Phase's own effects so the new beneficial
		// recovery effects cannot remove themselves.
		for (const sas::GameplayEffectDisposition disposition : {
			sas::GameplayEffectDisposition::Beneficial,
			sas::GameplayEffectDisposition::Harmful
		})
		{
			context.abilitySystem.RemoveGameplayEffectsIf(
				[disposition](const sas::ActiveGameplayEffect& activeEffect)
				{
					const sas::GameplayEffectDefinition& definition = activeEffect.spec.definition;
					return definition.disposition == disposition && definition.cleanseable;
				}
			);
		}

		const float energyFactor = ResolveEnergyFactor(context.abilitySystem, values);
		const float shieldMultiplier = 1.f +
			std::max(0.f, FindValue(values, AbilityData::PhaseDrift::Attribute::ShieldRegenBonus, 0.f)) +
			std::max(0.f, FindValue(values, AbilityData::PhaseDrift::Attribute::MaximumEnergyShieldBonus, 0.f)) * energyFactor;
		const float afterburnerMultiplier = 1.f +
			std::max(0.f, FindValue(values, AbilityData::PhaseDrift::Attribute::AfterburnerRegenBonus, 0.f)) +
			std::max(0.f, FindValue(values, AbilityData::PhaseDrift::Attribute::MaximumEnergyAfterburnerBonus, 0.f)) * energyFactor;
		const float movementMultiplier = 1.f + std::max(
			0.f,
			FindValue(values, AbilityData::PhaseDrift::Attribute::MovementSpeedBonus, 0.f)
		);

		ship->GetShieldComponent().ClearRechargeDelay();
		ship->GetEnergyComponent().ClearRechargeDelay();
		ship->GetRuntimeModifiers().Set(
			AbilityData::PhaseDrift::AbilityId::Basic,
			ShipRuntimeModifier{
				movementMultiplier,
				shieldMultiplier,
				afterburnerMultiplier
			}
		);
		ship->GetCombatRuntime().SetDamageProtection(
			AbilityData::PhaseDrift::AbilityId::Basic,
			true,
			true
		);

		mOriginalCollisionLayer = ship->GetCollisionLayer();
		mOriginalCollisionMask = ship->GetCollisionMask();
		mCollisionSnapshotValid = true;
		// Keep Powerup in the mask so pickup collection remains available while
		// enemy ships and enemy projectiles no longer generate collision events.
		ship->SetCollisionMask(CollisionLayer::Powerup);
		context.abilitySystem.AddOwnedTag(AbilityData::PhaseDrift::State::Active);

		mAppliedEffectHandles.clear();
		ApplyPolicyEffect(context, AbilityData::PhaseDrift::Effect::MovementBoostId, mResolvedDuration, mAppliedEffectHandles);
		ApplyPolicyEffect(context, AbilityData::PhaseDrift::Effect::ShieldRecoveryId, mResolvedDuration, mAppliedEffectHandles);
		ApplyPolicyEffect(context, AbilityData::PhaseDrift::Effect::AfterburnerRecoveryId, mResolvedDuration, mAppliedEffectHandles);

		if (World* world = context.owner.GetWorld())
		{
			if (const PhaseDriftPresentationProfile* profile =
				PresentationProfileRegistry<PhaseDriftPresentationProfile>::Find(
					PhaseDriftPresentationIds::Basic
				))
			{
				mVisualActor = world->SpawnActor<PhaseDriftVisualActor>(
					&context.owner,
					*profile
				);
			}
		}

		mActive = true;
		EmitEvent(context, AbilityData::PhaseDrift::Event::Started);
		return true;
	}

	void PhaseDriftAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		if (!mActive)
		{
			return;
		}

		RemoveAppliedEffects(context);
		if (auto* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetCombatRuntime().RemoveDamageProtection(
				AbilityData::PhaseDrift::AbilityId::Basic
			);
			ship->GetRuntimeModifiers().Remove(AbilityData::PhaseDrift::AbilityId::Basic);
			if (mCollisionSnapshotValid)
			{
				// Restore only the values Phase still owns. A concurrent system may
				// legitimately replace collision policy while Phase is active; in that
				// case its newer values must survive Phase cleanup.
				if (ship->GetCollisionLayer() == mOriginalCollisionLayer &&
					ship->GetCollisionMask() == CollisionLayer::Powerup)
				{
					ship->SetCollisionMask(mOriginalCollisionMask);
				}
			}
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::PhaseDrift::State::Active);
		if (auto visual = mVisualActor.lock())
		{
			visual->Destroy();
		}
		mVisualActor.reset();
		mCollisionSnapshotValid = false;
		mActive = false;

		if (reason == sas::AbilityEndReason::Completed)
		{
			EmitEvent(context, AbilityData::PhaseDrift::Event::Completed);
		}
		EmitEvent(context, AbilityData::PhaseDrift::Event::Ended);
	}

	void PhaseDriftAbility::RemoveAppliedEffects(GameAbilityBehaviorContext& context)
	{
		for (const sas::GameplayEffectHandle handle : mAppliedEffectHandles)
		{
			if (handle.IsValid())
			{
				context.abilitySystem.RemoveGameplayEffect(handle);
			}
		}
		mAppliedEffectHandles.clear();
	}

	void PhaseDriftAbility::OnOwnerAbilityActivated(
		GameAbilityBehaviorContext& context,
		const sas::AbilityLifecycleEvent& event
	)
	{
		(void)event;
		if (!mActive)
		{
			return;
		}
		EmitEvent(context, AbilityData::PhaseDrift::Event::BrokenByAction);
		context.instance.Cancel(sas::AbilityEndReason::Interrupted);
	}

	void PhaseDriftAbility::EmitEvent(
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
