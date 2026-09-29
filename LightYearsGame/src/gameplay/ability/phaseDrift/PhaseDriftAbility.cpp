#include "gameplay/ability/phaseDrift/PhaseDriftAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/phaseDrift/PhaseDriftContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/tags/GameplayTags.h"
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

	}

	bool PhaseDriftAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		std::size_t progressionIndex = 0;
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(definition.levelProgression.begin(), definition.levelProgression.end(),
				[&definition, &progressionIndex](const AbilityLevelStep& step)
				{
					const std::size_t index = progressionIndex++;
					if (step.attributeModifiers.size() != 4 || !step.scalingRules.empty()) return false;
					const auto has = [&step](const sas::AttributeId& id, float magnitude)
					{
						return std::any_of(step.attributeModifiers.begin(), step.attributeModifiers.end(),
							[&id, magnitude](const sas::AttributeModifier& modifier)
							{
								return modifier.attributeId == id &&
									modifier.operation == sas::AttributeModifierOperation::Add &&
									std::abs(modifier.magnitude - magnitude) <= 0.0001f;
							});
					};
					return has(CommonAttributeIds::Duration, 0.05f) &&
						has(AbilityData::PhaseDrift::Attribute::EPDurationScale, 0.05f) &&
						has(AbilityData::PhaseDrift::Attribute::MovementSpeedBonus, 0.02f) &&
						has(CommonAttributeIds::Cooldown, -GetGlobalAbilityCooldownStepReduction(definition.cooldown, index));
				});
		if (definition.abilityId != AbilityData::PhaseDrift::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.cooldown <= 0.f ||
			definition.duration != 3.5f || definition.cooldown != 16.f ||
			definition.attributes.size() != 2 || !validProgression)
		{
			if (failureReason)
			{
				*failureReason =
					"Phase Drift requires a loadout slot, pressed activation, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::PhaseDrift::Attribute::EPDurationScale,
			AbilityData::PhaseDrift::Attribute::MovementSpeedBonus
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

		if (!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::EPDurationScale)->baseValue) ||
			!IsFiniteNonNegative(FindAttribute(definition, AbilityData::PhaseDrift::Attribute::MovementSpeedBonus)->baseValue))
		{
			if (failureReason)
			{
				*failureReason = "Phase Drift contains an invalid duration, mobility or regeneration value.";
			}
			return false;
		}

		return true;
	}

	float PhaseDriftAbility::ResolveDuration(
		const GameAbilityBehaviorContext& context
	) const
	{
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float energyPower = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)
		);
		return std::max(
			0.f,
			context.definition.duration + energyPower / 100.f * FindValue(
				values, AbilityData::PhaseDrift::Attribute::EPDurationScale, 0.40f)
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

		const float movementMultiplier = 1.f + std::max(
			0.f,
			FindValue(values, AbilityData::PhaseDrift::Attribute::MovementSpeedBonus, 0.f)
		);

		ship->GetShieldComponent().ClearRechargeDelay();
		ship->GetEnergyComponent().ClearRechargeDelay();
		ship->GetRuntimeModifiers().Set(
			AbilityData::PhaseDrift::AbilityId::Basic,
			ShipRuntimeModifier{ movementMultiplier }
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
