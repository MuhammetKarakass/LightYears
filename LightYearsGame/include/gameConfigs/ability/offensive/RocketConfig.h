#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/ability/rocket/RocketContracts.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"

namespace AbilityData
{
	namespace Rocket
	{
		// Actor/behavior/presentation contract only. Numeric tuning lives in abilities.json.

		inline const ly::AbilityActorDefinition ActorProjectileBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = "Actor.Ability.Rocket.Projectile.Basic";
			definition.actorTypeTag = ActorSchema::TypeId;
			definition.texturePath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
			definition.presentationProfileId = ly::RocketPresentationIds::Basic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition* FindActorDefinition(
			const std::string& actorDefinitionId)
		{
			return actorDefinitionId == ActorProjectileBasic.actorDefinitionId
				? &ActorProjectileBasic
				: nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Rocket_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.Rocket.Basic";
			definition.slot = sas::AbilitySlot::Ability4;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Offense" },
				Rocket::FamilyTag
			};
			definition.displayName = "Rocket";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
			definition.inputLabel = "R";
			definition.accentColor = sf::Color{ 255, 115, 75, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						Rocket::ActorProjectileBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::OwnerForward,
						sas::AbilityDirectionPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.damageTags = { ly::DamageTypeSchema::Kinetic };
			definition.attachmentCapabilities = { ly::AttachmentSchema::Capability::Damage };
			definition.behaviorId = Rocket::BehaviorId;
			return definition;
		}();
	}

	namespace AbilityActors
	{
		inline const ly::AbilityActorDefinition& Actor_Rocket_Basic =
			Rocket::ActorProjectileBasic;
	}
}
