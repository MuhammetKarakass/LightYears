#pragma once

#include "gameplay/ability/blastback/BlastbackContracts.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition Blastback_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::Blastback::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadouts own the actual binding. Ability1 is only the catalog
		// default required by the current content contract.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// Duration is precisely the input-locked focus time. Recoil continues in
		// a separate actor so it cannot delay cooldown start.
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::Blastback
		};
		definition.displayName = "Blastback";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed.png";
		definition.accentColor = sf::Color{ 255, 105, 48, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::Blastback;
		return definition;
	}();
}
