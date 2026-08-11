#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"

namespace AbilityData
{
	namespace GravityAnomaly
	{
		// Actor/presentation schema only. Numeric tuning lives in abilities.json.

		inline const ly::AbilityActorDefinition ActorProjectileBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
			definition.actorType = ly::AbilityActorType::GravityAnomalyProjectile;
			definition.presentationProfileId = ly::GravityAnomalyPresentationIds::ProjectileBasic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition ActorFieldBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
			definition.actorType = ly::AbilityActorType::GravityAnomalyField;
			definition.presentationProfileId = ly::GravityAnomalyPresentationIds::FieldBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition GravityAnomaly_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = GravityAnomaly::AbilityId::Basic;
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTags::Ability::Control,
				ly::GameplayTags::Ability::Family::GravityAnomaly
			};
			definition.displayName = "Gravity Anomaly";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
			definition.inputLabel = "Q";
			definition.accentColor = sf::Color{ 135, 95, 255, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						GravityAnomaly::ActorProjectileBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::OwnerForward,
						sas::AbilityDirectionPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.behaviorType = ly::AbilityBehaviorType::GravityAnomaly;
			return definition;
		}();
	}

}
