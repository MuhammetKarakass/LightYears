#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "presentation/ability/ionStorm/IonStormPresentationIds.h"

namespace AbilityData::IonStorm
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::IonStormProjectile;
		definition.presentationProfileId =
			vega::IonStormPresentationIds::ProjectileBasic;
		return definition;
	}();

	inline const vega::AbilityActorDefinition ActorFieldBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::IonStormField;
		definition.presentationProfileId =
			vega::IonStormPresentationIds::FieldBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition IonStorm_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::IonStorm::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadout assignment owns the real input slot. This fallback slot
		// only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::IonStorm
		};
		definition.displayName = "Ion Storm";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 190, 255, 255 };
		definition.actions = {
			vega::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				vega::SpawnActorAction{
					AbilityData::IonStorm::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::MouseWorld
				},
				0.f,
				1
			}
		};
		definition.behaviorType = vega::AbilityBehaviorType::IonStorm;
		return definition;
	}();
}
