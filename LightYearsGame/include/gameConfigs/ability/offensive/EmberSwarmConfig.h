#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/emberSwarm/EmberSwarmContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition EmberSwarm_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::EmberSwarm::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Default runtime ability slot mapping has one owner only: DefaultAbilityLoadout.cpp.
		// Fallback config specifies Ability1 so it is a valid loadout ability when granted.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::EmberSwarm
		};
		definition.displayName = "Ember Swarm";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 120, 30, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::EmberSwarm;
		return definition;
	}();
}
