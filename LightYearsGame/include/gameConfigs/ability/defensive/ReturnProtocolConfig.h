#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition ReturnProtocol_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ReturnProtocol::AbilityId::Basic;
		// Runtime loadouts decide the real key/slot. This is only the required
		// fallback slot for content validation before an ability is equipped.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::ReturnProtocol
		};
		definition.displayName = "Return Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 105, 225, 255, 235 };
		definition.behaviorType = ly::AbilityBehaviorType::ReturnProtocol;
		return definition;
	}();
}
