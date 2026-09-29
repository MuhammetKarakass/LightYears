#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "presentation/ability/solarBombardment/SolarBombardmentPresentationIds.h"


namespace AbilityData::SolarBombardment
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::SolarBombardmentProjectile;
		definition.presentationProfileId =
			ly::SolarBombardmentPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition SolarBombardment_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::SolarBombardment::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadout assignment owns the real input slot. This fallback slot
		// only keeps the standalone C++ definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::SolarBombardment
		};
		definition.displayName = "Solar Bombardment";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
		definition.accentColor = sf::Color{ 255, 125, 35, 255 };
		// Activation is implemented by the family behavior because it must pass
		// the cursor-clamped target location into the projectile actor.
		definition.behaviorType = ly::AbilityBehaviorType::SolarBombardment;
		return definition;
	}();
}
