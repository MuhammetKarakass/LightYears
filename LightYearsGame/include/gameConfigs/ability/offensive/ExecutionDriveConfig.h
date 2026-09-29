#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance is loaded from abilities.json. This fallback only provides
	// the executable family identity and safe values for tests without content.
	inline const ly::GameAbilityDefinition ExecutionDrive_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ExecutionDrive::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Legacy fallback slot; runtime AbilityLoadoutManager may rebind this
		// ability to any player ability slot after acquisition.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ExecutionDrive
		};
		definition.displayName = "Execution Drive";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 80, 35, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::ExecutionDrive;
		return definition;
	}();
}
