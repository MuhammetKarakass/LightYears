#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/foldspaceArena/FoldspaceArenaPresentationIds.h"

namespace AbilityData::FoldspaceArena
{
	inline const ly::AbilityActorDefinition ActorArenaBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Arena::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::FoldspaceArena;
		// Runtime duration is resolved from EnergyPower and extended through travel.
		// This is only the fallback before ability values are applied.
		definition.presentationProfileId = ly::FoldspaceArenaPresentationIds::ArenaBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition FoldspaceArena_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FoldspaceArena::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// The behavior extends this lifecycle through the resolved arena duration
		// and projectile travel time; this is the safe fallback duration.
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::FoldspaceArena
		};
		definition.displayName = "Foldspace Arena";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 145, 215, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::FoldspaceArena;
		return definition;
	}();
}
