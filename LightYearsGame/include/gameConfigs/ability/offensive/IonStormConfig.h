#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "presentation/ability/ionStorm/IonStormPresentationIds.h"

namespace AbilityData::IonStorm
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::IonStormProjectile;
		definition.presentationProfileId =
			ly::IonStormPresentationIds::ProjectileBasic;
		return definition;
	}();

	inline const ly::AbilityActorDefinition ActorFieldBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::IonStormField;
		definition.presentationProfileId =
			ly::IonStormPresentationIds::FieldBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition IonStorm_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::IonStorm::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadout assignment owns the real input slot. This fallback slot
		// only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::IonStorm
		};
		definition.displayName = "Ion Storm";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 90, 190, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::IonStorm::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::MouseWorld
				},
				0.f,
				1
			}
		};
		definition.behaviorType = ly::AbilityBehaviorType::IonStorm;
		return definition;
	}();
}
