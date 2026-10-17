#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/phaseDrift/PhaseDriftContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition PhaseDrift_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::PhaseDrift::AbilityId::Basic;
		// Ability3 is mapped to F by the existing player input layer.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Movement,
			vega::GameplayTags::Ability::Family::PhaseDrift
		};
		definition.displayName = "Phase Drift";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 120, 220, 255, 180 };
		definition.behaviorType = vega::AbilityBehaviorType::PhaseDrift;
		return definition;
	}();
}
