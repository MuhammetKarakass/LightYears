#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolContracts.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition IroncladProtocol_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::IroncladProtocol::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = { AbilityData::IroncladProtocol::CategoryTag, AbilityData::IroncladProtocol::FamilyTag };
		definition.displayName = "Ironclad Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 180, 180, 200, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::IroncladProtocol;
		return definition;
	}();
}
