#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/aegisReaver/AegisReaverContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/aegisReaver/AegisReaverPresentationIds.h"

namespace AbilityData::AegisReaver
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::AegisReaverProjectile;
		definition.presentationProfileId = vega::AegisReaverPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition AegisReaver_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::AegisReaver::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::AegisReaver
		};
		definition.displayName = "Aegis Reaver";
		// Reuse an already shipped sprite until Aegis receives its own UI icon.
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 100, 225, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::AegisReaver;
		return definition;
	}();
}
