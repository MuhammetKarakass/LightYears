#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationIds.h"

namespace AbilityData::WingSentinels
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::WingSentinelProjectile;
		definition.presentationProfileId = ly::WingSentinelsPresentationIds::Basic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition WingSentinels_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::WingSentinels::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::WingSentinels
		};
		definition.displayName = "Wing Sentinels";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 100, 205, 255, 255 };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::WingSentinels;
		return definition;
	}();
}
