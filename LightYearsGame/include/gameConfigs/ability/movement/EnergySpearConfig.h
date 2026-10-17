#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition EnergySpear_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::EnergySpear::AbilityId::Basic;
		// Q is the shipped default binding. Runtime loadout code may still equip
		// Energy Spear into any available Ability1-Ability4 slot later.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::WhileHeld;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::WhileInputHeld;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Movement,
			vega::GameplayTags::Ability::Family::EnergySpear
		};
		definition.displayName = "Energy Spear";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 190, 250, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::EnergySpear;
		return definition;
	}();
}
