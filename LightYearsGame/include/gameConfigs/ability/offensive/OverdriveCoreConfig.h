#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/overdriveCore/OverdriveCorePresentationIds.h"

namespace AbilityData::OverdriveCore
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::OverdriveCoreProjectile;
		definition.presentationProfileId = vega::OverdriveCorePresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns only the executable behavior family and semantic identity. The
	// shipped numeric values are loaded from abilities.json.
	inline const vega::GameAbilityDefinition OverdriveCore_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::OverdriveCore::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::OverdriveCore
		};
		definition.displayName = "Overdrive Core";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
		definition.accentColor = sf::Color{ 255, 95, 45, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::OverdriveCore;
		return definition;
	}();
}
