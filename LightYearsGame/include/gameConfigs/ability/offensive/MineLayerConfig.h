#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/mineLayer/MineLayerContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/mineLayer/MineLayerPresentationIds.h"

namespace AbilityData::MineLayer
{
	inline const vega::AbilityActorDefinition ActorMineBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Mine::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::MineLayerMine;
		definition.presentationProfileId = vega::MineLayerPresentationIds::MineBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns the executable behavior and actor/presentation identity. Numeric
	// balance lives only in abilities.json.
	inline const vega::GameAbilityDefinition MineLayer_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::MineLayer::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Ability1 is the default Q binding; the runtime loadout may still rebind it.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::MineLayer
		};
		definition.displayName = "Mine Layer";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 80, 175, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::MineLayer;
		return definition;
	}();
}
