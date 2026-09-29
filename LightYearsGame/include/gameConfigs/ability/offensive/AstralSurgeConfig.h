#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/astralSurge/AstralSurgePresentationIds.h"


namespace AbilityData::AstralSurge
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::AstralSurgeProjectile;
		definition.presentationProfileId = ly::AstralSurgePresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition AstralSurge_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::AstralSurge::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::AstralSurge
		};
		definition.displayName = "Astral Surge";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 175, 105, 255, 255 };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::AstralSurge;
		return definition;
	}();
}
