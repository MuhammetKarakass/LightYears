#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/crystalBarricade/CrystalBarricadePresentationIds.h"

namespace AbilityData::CrystalBarricade
{
	inline const vega::AbilityActorDefinition ActorWallBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Wall::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::CrystalBarricadeWall;
		definition.presentationProfileId = vega::CrystalBarricadePresentationIds::WallBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition CrystalBarricade_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CrystalBarricade::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::CrystalBarricade
		};
		definition.displayName = "Crystal Barricade";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 110, 225, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::CrystalBarricade;
		return definition;
	}();
}
