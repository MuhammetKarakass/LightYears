#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/stormMark/StormMarkContracts.h"
#include "gameplay/tags/GameplayTags.h"


namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition StormMark_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::StormMark::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Ability1 is only the standalone fallback slot. The runtime loadout
		// remains the owner of the player's actual binding.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::StormMark
		};
		definition.displayName = "Storm Mark";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 65, 190, 255, 255 };
		// Behavior owns focus, target snapshot and delayed strikes. No generic
		// action is declared because it would execute before the focus completes.
		definition.behaviorType = vega::AbilityBehaviorType::StormMark;
		return definition;
	}();
}
