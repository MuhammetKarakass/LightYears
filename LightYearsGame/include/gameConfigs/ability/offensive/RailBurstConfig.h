#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/railBurst/RailBurstContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/railBurst/RailBurstPresentationIds.h"

namespace AbilityData::RailBurst
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::RailBurstProjectile;
		definition.presentationProfileId = vega::RailBurstPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns the executable behavior and spawn policy. Balance values remain
	// in abilities.json so shipped content and progression stay data-driven.
	inline const vega::GameAbilityDefinition RailBurst_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::RailBurst::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::RailBurst
		};
		definition.displayName = "Rail Burst";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 120, 235, 255, 255 };
		definition.actions = {
			vega::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				vega::SpawnActorAction{
					AbilityData::RailBurst::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::OwnerForward
				},
				0.f,
				1
			}
		};
		definition.behaviorType = vega::AbilityBehaviorType::RailBurst;
		return definition;
	}();
}
