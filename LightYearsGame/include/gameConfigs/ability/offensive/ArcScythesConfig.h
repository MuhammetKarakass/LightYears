#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/arcScythes/ArcScythesContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/arcScythes/ArcScythesPresentationIds.h"

namespace AbilityData::ArcScythes
{
	inline const ly::AbilityActorDefinition ActorBeamBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Beam::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::ArcScythesBeam;
		definition.presentationProfileId = ly::ArcScythesPresentationIds::BeamBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ArcScythes_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ArcScythes::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadouts own the player binding. This is only a valid fallback
		// slot for content loading and does not permanently assign a key.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::ArcScythes
		};
		definition.displayName = "Arc Scythes";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 230, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::ArcScythes::Actor::Beam::BasicDefinitionId,
					sas::AbilitySpawnPolicy::AtOwner
				},
				0.f,
				1
			}
		};
		definition.behaviorType = ly::AbilityBehaviorType::ArcScythes;
		return definition;
	}();
}
