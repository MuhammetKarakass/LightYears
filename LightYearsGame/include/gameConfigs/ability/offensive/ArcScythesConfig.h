#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/arcScythes/ArcScythesContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/arcScythes/ArcScythesPresentationIds.h"

namespace AbilityData::ArcScythes
{
	inline const vega::AbilityActorDefinition ActorBeamBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Beam::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::ArcScythesBeam;
		definition.presentationProfileId = vega::ArcScythesPresentationIds::BeamBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition ArcScythes_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ArcScythes::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadouts own the player binding. This is only a valid fallback
		// slot for content loading and does not permanently assign a key.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::ArcScythes
		};
		definition.displayName = "Arc Scythes";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 230, 255, 255 };
		definition.actions = {
			vega::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				vega::SpawnActorAction{
					AbilityData::ArcScythes::Actor::Beam::BasicDefinitionId,
					sas::AbilitySpawnPolicy::AtOwner
				},
				0.f,
				1
			}
		};
		definition.behaviorType = vega::AbilityBehaviorType::ArcScythes;
		return definition;
	}();
}
