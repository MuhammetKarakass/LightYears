#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Fallback/test definition. Shipped balance is loaded from abilities.json;
	// this preserves the C++ behavior identity when JSON is not loaded yet.
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition Cryostasis_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::Cryostasis::AbilityId::Basic;
		// Content validation requires a legal loadout-slot fallback. This is not
		// an equip command: the runtime AbilityLoadout still decides which owned
		// ability occupies each player slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::Cryostasis
		};
		definition.displayName = "Cryostasis";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 130, 220, 255, 235 };
		definition.behaviorType = vega::AbilityBehaviorType::Cryostasis;
		return definition;
	}();
}
