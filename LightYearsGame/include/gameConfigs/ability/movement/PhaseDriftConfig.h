#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/phaseDrift/PhaseDriftContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition PhaseDrift_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::PhaseDrift::AbilityId::Basic;
		// Ability3 is mapped to F by the existing player input layer.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 6.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Movement,
			ly::GameplayTags::Ability::Family::PhaseDrift
		};
		definition.displayName = "Phase Drift";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 120, 220, 255, 180 };
		definition.behaviorType = ly::AbilityBehaviorType::PhaseDrift;
		return definition;
	}();
}
