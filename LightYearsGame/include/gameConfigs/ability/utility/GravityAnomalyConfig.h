#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"

namespace AbilityData
{
	namespace GravityAnomaly
	{
		inline const vega::AbilityActorDefinition ActorProjectileBasic = []
		{
			vega::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
			definition.actorType = vega::AbilityActorType::GravityAnomalyProjectile;
			definition.presentationProfileId = vega::GravityAnomalyPresentationIds::ProjectileBasic;
			return definition;
		}();

		inline const vega::AbilityActorDefinition ActorFieldBasic = []
		{
			vega::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
			definition.actorType = vega::AbilityActorType::GravityAnomalyField;
			definition.presentationProfileId = vega::GravityAnomalyPresentationIds::FieldBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		// Numeric balance, progression and costs are authored in abilities.json.
		inline const vega::GameAbilityDefinition GravityAnomaly_Basic = []
		{
			vega::GameAbilityDefinition definition;
			definition.abilityId = GravityAnomaly::AbilityId::Basic;
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.abilityTags = {
				vega::GameplayTags::Ability::Utility,
				vega::GameplayTags::Ability::Family::GravityAnomaly
			};
			definition.displayName = "Gravity Anomaly";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
			definition.accentColor = sf::Color{ 135, 95, 255, 255 };
			definition.actions = {
				vega::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					vega::SpawnActorAction{
						GravityAnomaly::ActorProjectileBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::OwnerForward,
						sas::AbilityDirectionPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.behaviorType = vega::AbilityBehaviorType::GravityAnomaly;
			return definition;
		}();
	}

}
