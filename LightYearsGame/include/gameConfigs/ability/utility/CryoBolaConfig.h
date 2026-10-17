#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/cryoBola/CryoBolaContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/cryoBola/CryoBolaPresentationIds.h"

namespace AbilityData::CryoBola
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::CryoBolaProjectile;
		definition.presentationProfileId = vega::CryoBolaPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition CryoBola_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CryoBola::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Utility,
			vega::GameplayTags::Ability::Family::CryoBola
		};
		definition.displayName = "Cryo Bola";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 120, 225, 255, 255 };
		definition.actions = {
			vega::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				vega::SpawnActorAction{
					AbilityData::CryoBola::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::OwnerForward
				},
				0.f,
				1
			}
		};
		definition.behaviorType = vega::AbilityBehaviorType::CryoBola;
		return definition;
	}();
}
