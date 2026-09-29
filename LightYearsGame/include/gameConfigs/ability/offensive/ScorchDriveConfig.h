#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/scorchDrive/ScorchDriveContracts.h"
#include "presentation/ability/scorchDrive/ScorchDrivePresentationIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ScorchDrive
{
	inline const ly::AbilityActorDefinition ActorFireSegmentBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::FireSegment::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::ScorchDriveFireSegment;
		definition.presentationProfileId =
			ly::ScorchDrivePresentationIds::FireSegmentBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ScorchDrive_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ScorchDrive::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// The runtime loadout decides the actual input slot. This transitional
		// value only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ScorchDrive
		};
		definition.displayName = "Scorch Drive";
		definition.iconPath = "SpaceShooterRedux/PNG/Effects/fire01.png";
		definition.accentColor = sf::Color{ 255, 95, 25, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::ScorchDrive;
		return definition;
	}();
}
