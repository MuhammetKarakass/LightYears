#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/timeSlip/TimeSlipContracts.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition TimeSlip_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TimeSlip::AbilityId::Basic;
		// The loadout owns the runtime binding. This fallback slot only keeps the
		// definition valid before a player equips the ability dynamically.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::TimeSlip
		};
		definition.displayName = "Time Slip";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 155, 210, 255, 245 };
		definition.behaviorType = vega::AbilityBehaviorType::TimeSlip;
		return definition;
	}();
}
