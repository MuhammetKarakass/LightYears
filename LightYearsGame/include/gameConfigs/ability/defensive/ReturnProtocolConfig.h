#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition ReturnProtocol_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ReturnProtocol::AbilityId::Basic;
		// Runtime loadouts decide the real key/slot. This is only the required
		// fallback slot for content validation before an ability is equipped.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::ReturnProtocol
		};
		definition.displayName = "Return Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 105, 225, 255, 235 };
		definition.behaviorType = vega::AbilityBehaviorType::ReturnProtocol;
		return definition;
	}();
}
