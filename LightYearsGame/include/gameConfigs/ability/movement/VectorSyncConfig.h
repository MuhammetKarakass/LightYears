#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/vectorSync/VectorSyncContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition VectorSync_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::VectorSync::AbilityId::Basic;
		// Loadouts own real bindings. This only keeps the catalog definition
		// valid before a player equips Vector Sync into a runtime slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Movement,
			vega::GameplayTags::Ability::Family::VectorSync
		};
		definition.displayName = "Vector Sync";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 90, 225, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::VectorSync;
		return definition;
	}();
}
