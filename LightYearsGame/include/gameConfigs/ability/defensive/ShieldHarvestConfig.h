#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/shieldHarvest/ShieldHarvestContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition ShieldHarvest_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ShieldHarvest::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::ShieldHarvest
		};
		definition.displayName = "Shield Harvest";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 95, 220, 255, 220 };
		definition.behaviorType = vega::AbilityBehaviorType::ShieldHarvest;
		return definition;
	}();
}
