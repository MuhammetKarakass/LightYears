#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/crescentReaver/CrescentReaverContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/crescentReaver/CrescentReaverPresentationIds.h"

namespace AbilityData::CrescentReaver
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::CrescentReaverProjectile;
		definition.presentationProfileId = vega::CrescentReaverPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns executable behavior and the spawn policy. Runtime balance is
	// authoritative in abilities.json, where the actor values are also exposed
	// to the content validator and progression pipeline.
	inline const vega::GameAbilityDefinition CrescentReaver_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CrescentReaver::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Ability1 is the shared Q input slot. The runtime loadout may still
		// rebind this ability later without changing the content definition.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::CrescentReaver
		};
		definition.displayName = "Crescent Reaver";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 125, 210, 255, 255 };
		definition.actions = {
			vega::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				vega::SpawnActorAction{
					AbilityData::CrescentReaver::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::MouseWorld
				},
				0.f,
				1
			}
		};
		definition.behaviorType = vega::AbilityBehaviorType::CrescentReaver;
		return definition;
	}();
}
