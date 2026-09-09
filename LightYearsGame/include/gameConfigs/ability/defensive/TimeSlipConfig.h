#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/timeSlip/TimeSlipContracts.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition TimeSlip_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TimeSlip::AbilityId::Basic;
		// The loadout owns the runtime binding. This fallback slot only keeps the
		// definition valid before a player equips the ability dynamically.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.cooldown = 18.f;
		definition.duration = 3.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::TimeSlip
		};
		definition.displayName = "Time Slip";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 155, 210, 255, 245 };
		definition.attributes = {
			{ AbilityData::TimeSlip::Attribute::GameplayTimeMultiplier, 0.35f, 0.001f, 1.f },
			{ AbilityData::TimeSlip::Attribute::PrimaryFireRateMultiplier, 0.35f, 0.001f, 1.f },
			{ AbilityData::TimeSlip::Attribute::EnergyMaxReference, 50.f, 0.f },
			{ AbilityData::TimeSlip::Attribute::EnergyMaxDurationScale, 0.0015f, 0.f }
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{ {
				{ ly::CommonAttributeIds::Duration,
					sas::AttributeModifierOperation::Add, 0.10f },
				{ ly::CommonAttributeIds::Cooldown,
					sas::AttributeModifierOperation::Add, -0.20f }
			}, {}, {}, {} }
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::TimeSlip;
		return definition;
	}();
}
