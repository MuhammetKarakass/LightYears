#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/zeroDrag/ZeroDragContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition ZeroDrag_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ZeroDrag::AbilityId::Basic;
		// The catalog slot is only a valid placeholder. Runtime loadouts own the
		// player's actual Q/E/F/R binding and may equip this ability anywhere.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Movement,
			vega::GameplayTags::Ability::Family::ZeroDrag
		};
		definition.displayName = "Zero Drag";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 150, 230, 255, 255 };

		definition.behaviorType = vega::AbilityBehaviorType::ZeroDrag;
		return definition;
	}();
}
