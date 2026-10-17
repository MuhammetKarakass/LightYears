#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationIds.h"

namespace AbilityData::WingSentinels
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::WingSentinelProjectile;
		definition.presentationProfileId = vega::WingSentinelsPresentationIds::Basic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition WingSentinels_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::WingSentinels::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::WingSentinels
		};
		definition.displayName = "Wing Sentinels";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 100, 205, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::WingSentinels;
		return definition;
	}();
}
