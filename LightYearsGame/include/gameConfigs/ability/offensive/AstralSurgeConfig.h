#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationIds.h"


namespace AbilityData::AstralSurge
{
	inline const vega::AbilityActorDefinition ActorProjectileBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::AstralSurgeProjectile;
		definition.presentationProfileId = vega::AstralSurgePresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition AstralSurge_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::AstralSurge::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::AstralSurge
		};
		definition.displayName = "Astral Surge";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 175, 105, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::AstralSurge;
		return definition;
	}();
}
