#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationIds.h"

namespace AbilityData::LanceDrive
{
	// This local definition is the C++ owner of the actor type and its typed
	// presentation profile; shipped JSON supplies only balance values.
	inline const ly::AbilityActorDefinition ActorLanceBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Lance::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::LanceDrive;
		definition.presentationProfileId = ly::LanceDrivePresentationIds::LanceBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition LanceDrive_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::LanceDrive::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// This is only the transitional catalog default; runtime key assignment
		// remains owned by the loadout system and is intentionally not changed here.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::LanceDrive
		};
		definition.displayName = "Lance Drive";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 255, 174, 76, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::LanceDrive;
		return definition;
	}();
}
