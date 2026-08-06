#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyContracts.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"

namespace AbilityData
{
	namespace GravityAnomaly
	{
		// Actor/behavior/presentation contract only. Numeric tuning lives in abilities.json.

		inline const ly::AbilityActorDefinition ActorProjectileBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = "Actor.Ability.GravityAnomaly.Projectile.Basic";
			definition.actorTypeTag = ActorSchema::ProjectileTypeId;
			definition.texturePath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
			definition.presentationProfileId = ly::GravityAnomalyPresentationIds::ProjectileBasic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition ActorFieldBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = "Actor.Ability.GravityAnomaly.Field.Basic";
			definition.actorTypeTag = ActorSchema::FieldTypeId;
			definition.presentationProfileId = ly::GravityAnomalyPresentationIds::FieldBasic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition* FindActorDefinition(
			const std::string& actorDefinitionId
		)
		{
			if (actorDefinitionId == ActorProjectileBasic.actorDefinitionId)
			{
				return &ActorProjectileBasic;
			}
			return actorDefinitionId == ActorFieldBasic.actorDefinitionId
				? &ActorFieldBasic
				: nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition GravityAnomaly_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.GravityAnomaly.Basic";
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Control" },
				GravityAnomaly::FamilyTag
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
			definition.behaviorId = GravityAnomaly::BehaviorId;
			return definition;
		}();
	}

	namespace AbilityActors
	{
		inline const ly::AbilityActorDefinition& Actor_GravityAnomaly_Projectile_Basic =
			GravityAnomaly::ActorProjectileBasic;
		inline const ly::AbilityActorDefinition& Actor_GravityAnomaly_Field_Basic =
			GravityAnomaly::ActorFieldBasic;
	}
}
