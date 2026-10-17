#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition HullShock_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::HullShock::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Ability1 is mapped to Q. Hull Shock replaces the previous default
		// ability in that input slot; Shield Harvest keeps the defensive F slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::WhileHeld;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::WhileInputHeld;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::HullShock
		};
		definition.displayName = "Hull Shock";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 75, 244, 255, 235 };
		definition.behaviorType = vega::AbilityBehaviorType::HullShock;
		return definition;
	}();
}
