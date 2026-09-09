#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/aegisReaver/AegisReaverContracts.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/aegisReaver/AegisReaverPresentationIds.h"

namespace AbilityData::AegisReaver
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::AegisReaverProjectile;
		definition.presentationProfileId = ly::AegisReaverPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition AegisReaver_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::AegisReaver::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability3;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 14.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::AegisReaver
		};
		definition.displayName = "Aegis Reaver";
		// Reuse an already shipped sprite until Aegis receives its own UI icon.
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 100, 225, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::AegisReaver::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::MouseWorld
				},
				0.f,
				1
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Cooldown,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::AegisReaver;
		return definition;
	}();
}
