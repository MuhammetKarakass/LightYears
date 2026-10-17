#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/ability/rocket/RocketContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"

namespace AbilityData
{
	namespace Rocket
	{
		// Actor/presentation schema only. Numeric tuning lives in abilities.json.

		inline const vega::AbilityActorDefinition ActorProjectileBasic = []
		{
			vega::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
			definition.actorType = vega::AbilityActorType::RocketProjectile;
			definition.presentationProfileId = vega::RocketPresentationIds::ProjectileBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const vega::GameAbilityDefinition Rocket_Basic = []
		{
			vega::GameAbilityDefinition definition;
			definition.abilityId = Rocket::AbilityId::Basic;
			// Numeric balance, progression and costs are authored in abilities.json.
			// Ability3 is the default F binding for the projectile loadout.
			// Runtime loadout bindings may still move Rocket later.
			definition.slot = sas::AbilitySlot::Ability3;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.abilityTags = {
				vega::GameplayTags::Ability::Offense,
				vega::GameplayTags::Ability::Family::Rocket
			};
			definition.displayName = "Rocket";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
			definition.accentColor = sf::Color{ 255, 115, 75, 255 };
			definition.actions = {
				vega::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					vega::SpawnActorAction{
						Rocket::ActorProjectileBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::OwnerForward,
						sas::AbilityDirectionPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.damageTags = { vega::DamageTypeSchema::Kinetic };
			definition.behaviorType = vega::AbilityBehaviorType::Rocket;
			return definition;
		}();
	}

}
