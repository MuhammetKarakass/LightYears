#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/emberSwarm/EmberSwarmContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition EmberSwarm_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::EmberSwarm::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Default runtime ability slot mapping has one owner only: DefaultAbilityLoadout.cpp.
		// Fallback config specifies Ability1 so it is a valid loadout ability when granted.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::EmberSwarm
		};
		definition.displayName = "Ember Swarm";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 120, 30, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::EmberSwarm;
		return definition;
	}();
}
