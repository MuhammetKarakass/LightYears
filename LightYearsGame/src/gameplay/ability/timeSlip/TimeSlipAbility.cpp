#include "gameplay/ability/timeSlip/TimeSlipAbility.h"

#include "framework/World.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/timeSlip/TimeSlipContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
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

		float ClampTimeMultiplier(float multiplier)
		{
			return std::clamp(
				std::isfinite(multiplier) ? multiplier : 1.f,
				0.001f,
				1.f
			);
		}
	}

	bool TimeSlipAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::TimeSlip::AbilityId::Basic ||
			definition.behaviorType != AbilityBehaviorType::TimeSlip ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.cooldownStartPolicy != sas::AbilityCooldownStartPolicy::OnActivation ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f ||
			definition.attributes.size() != 4 ||
			definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Time Slip requires one duration charge, four runtime attributes, and fourteen level steps.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::TimeSlip::Attribute::GameplayTimeMultiplier,
			AbilityData::TimeSlip::Attribute::PrimaryFireRateMultiplier,
			AbilityData::TimeSlip::Attribute::EnergyPowerReference,
			AbilityData::TimeSlip::Attribute::EnergyPowerDurationScale
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue) ||
				!std::isfinite(attribute->minValue) || attribute->baseValue < 0.f)
			{
				if (failureReason)
				{
					*failureReason = "Time Slip has an invalid runtime attribute.";
				}
				return false;
			}
		}

		const float enemyMultiplier = FindValue(
			definition.attributes,
			AbilityData::TimeSlip::Attribute::GameplayTimeMultiplier,
			0.f
		);
		const float primaryMultiplier = FindValue(
			definition.attributes,
			AbilityData::TimeSlip::Attribute::PrimaryFireRateMultiplier,
			0.f
		);
		if (enemyMultiplier <= 0.f || enemyMultiplier > 1.f ||
			primaryMultiplier <= 0.f || primaryMultiplier > 1.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Time Slip temporal and primary-fire multipliers must be in (0, 1].";
			}
			return false;
		}
		return true;
	}

	bool TimeSlipAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		if (!ship || !world || mModifiersApplied)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float gameplayTimeMultiplier = ClampTimeMultiplier(FindValue(
			values,
			AbilityData::TimeSlip::Attribute::GameplayTimeMultiplier,
			0.35f
		));
		const float primaryFireRateMultiplier = ClampTimeMultiplier(FindValue(
			values,
			AbilityData::TimeSlip::Attribute::PrimaryFireRateMultiplier,
			0.35f
		));

		mModifierSourceId = BuildModifierSourceId(context.owner);
		world->SetSimulationTimeModifier(
			SimulationTimeDomain::HostileGameplay,
			mModifierSourceId,
			gameplayTimeMultiplier
		);
		// ProjectileGameplay is deliberately separate from hostile actors: every
		// projectile, including player primary/ability projectiles, receives the
		// same authored temporal multiplier while the player remains real-time.
		world->SetSimulationTimeModifier(
			SimulationTimeDomain::ProjectileGameplay,
			mModifierSourceId,
			gameplayTimeMultiplier
		);
		ship->SetPrimaryWeaponFireRateModifier(
			mModifierSourceId,
			primaryFireRateMultiplier
		);
		mModifiersApplied = true;

		context.abilitySystem.AddOwnedTag(AbilityData::TimeSlip::State::Active);
		EmitEvent(context, AbilityData::TimeSlip::Event::Started);
		return true;
	}

	void TimeSlipAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mModifiersApplied)
		{
			return;
		}

		if (World* world = context.owner.GetWorld())
		{
			world->RemoveSimulationTimeModifier(
				SimulationTimeDomain::HostileGameplay,
				mModifierSourceId
			);
			world->RemoveSimulationTimeModifier(
				SimulationTimeDomain::ProjectileGameplay,
				mModifierSourceId
			);
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->RemovePrimaryWeaponFireRateModifier(mModifierSourceId);
		}

		context.abilitySystem.RemoveOwnedTag(AbilityData::TimeSlip::State::Active);
		EmitEvent(context, AbilityData::TimeSlip::Event::Ended);
		mModifierSourceId = 0;
		mModifiersApplied = false;
	}

	float TimeSlipAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		GameAbilityBehaviorContext& mutableContext =
			const_cast<GameAbilityBehaviorContext&>(context);
		const sas::GameplayAttributeList values = ResolveValues(mutableContext);
		const float energyReference = FindValue(
			values,
			AbilityData::TimeSlip::Attribute::EnergyPowerReference,
			50.f
		);
		const float energyScale = FindValue(
			values,
			AbilityData::TimeSlip::Attribute::EnergyPowerDurationScale,
			0.0015f
		);
		const float energyPower = context.abilitySystem.GetAttributes().GetCurrentValue(
			AbilityData::TimeSlip::Attribute::DurationScalingSource
		);
		const float durationBonus = std::max(0.f, energyPower - energyReference) *
			std::max(0.f, energyScale);
		const float resolvedDuration = defaultDuration + durationBonus;
		return std::isfinite(resolvedDuration)
			? std::max(0.f, resolvedDuration)
			: std::max(0.f, defaultDuration);
	}

	void TimeSlipAbility::EmitEvent(
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

	temporal::RateModifierSourceId TimeSlipAbility::BuildModifierSourceId(
		const Actor& owner
	)
	{
		// The address is stable for the ability's lifetime and keeps two owners in
		// the same world from removing each other's temporal modifier.
		return static_cast<temporal::RateModifierSourceId>(
			reinterpret_cast<std::uintptr_t>(&owner)
		) ^ 0x54494D45534C4950ull;
	}
}
