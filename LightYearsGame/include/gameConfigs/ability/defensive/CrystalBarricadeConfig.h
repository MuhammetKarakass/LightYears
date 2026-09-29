#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/crystalBarricade/CrystalBarricadePresentationIds.h"

namespace AbilityData::CrystalBarricade
{
	inline const ly::AbilityActorDefinition ActorWallBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Wall::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::CrystalBarricadeWall;
		definition.presentationProfileId = ly::CrystalBarricadePresentationIds::WallBasic;
		definition.lifeTime = 6.f;
		definition.attributes = {
			{ ly::CommonAttributeIds::Duration, 6.f, 0.01f },
			{ ly::AreaAttributeIds::Length, 300.f, 1.f },
			{ ly::AreaAttributeIds::Width, 22.f, 1.f },
			{ Actor::Wall::SameSurfaceLockDuration, 0.12f, 0.f },
			{ Actor::Wall::BaseContactDamage, 75.f, 0.f },
			{ Actor::Wall::EnergyPowerContactScale, 0.60f, 0.f },
			{ Actor::Wall::ContactInterval, 1.5f, 0.f },
			{ Actor::Wall::BaseRicochetMultiplier, 1.25f, 0.f },
			{ Actor::Wall::EnergyPowerRicochetReference, 500.f, 0.f },
			{ Actor::Wall::EnergyPowerRicochetScale, 1.f, 0.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition CrystalBarricade_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CrystalBarricade::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 14.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::CrystalBarricade
		};
		definition.displayName = "Crystal Barricade";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 110, 225, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::CrystalBarricade;
		return definition;
	}();
}
