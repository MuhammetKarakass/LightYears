#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/ability/rocket/RocketContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"

namespace AbilityData
{
	namespace Rocket
	{
		// Actor/presentation schema only. Numeric tuning lives in abilities.json.

		inline const ly::AbilityActorDefinition ActorProjectileBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
			definition.actorType = ly::AbilityActorType::RocketProjectile;
			definition.presentationProfileId = ly::RocketPresentationIds::ProjectileBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Rocket_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = Rocket::AbilityId::Basic;
			// Numeric balance, progression and costs are authored in abilities.json.
			// Ability3 is the default F binding for the projectile loadout.
			// Runtime loadout bindings may still move Rocket later.
			definition.slot = sas::AbilitySlot::Ability3;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.abilityTags = {
				ly::GameplayTags::Ability::Offense,
				ly::GameplayTags::Ability::Family::Rocket
			};
			definition.displayName = "Rocket";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
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
			definition.behaviorType = ly::AbilityBehaviorType::Rocket;
			return definition;
		}();
	}

}
