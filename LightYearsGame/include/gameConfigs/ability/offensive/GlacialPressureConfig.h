#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/glacialPressure/GlacialPressureContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition GlacialPressure_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::GlacialPressure::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// The shipped default loadout explicitly binds Glacial Pressure to R
		// (Ability4). Acquired abilities may still be rebound at runtime.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// The behavior treats definition.duration as the one-second focus period
		// and extends the actual active lifetime by the controlled push duration.
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::GlacialPressure
		};
		definition.displayName = "Glacial Pressure";
		// Keep the icon reference inside the shipped asset set. The ability's
		// gameplay identity is independent from this presentation texture.
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 155, 235, 255, 235 };
		definition.behaviorType = ly::AbilityBehaviorType::GlacialPressure;
		return definition;
	}();
}
