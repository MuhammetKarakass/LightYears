#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/railBurst/RailBurstContracts.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/railBurst/RailBurstPresentationIds.h"

namespace AbilityData::RailBurst
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::RailBurstProjectile;
		definition.presentationProfileId = ly::RailBurstPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns the executable behavior and spawn policy. Balance values remain
	// in abilities.json so shipped content and progression stay data-driven.
	inline const ly::GameAbilityDefinition RailBurst_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::RailBurst::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 0.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::RailBurst
		};
		definition.displayName = "Rail Burst";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 120, 235, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::RailBurst::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::OwnerForward
				},
				0.f,
				1
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::RailBurst;
		return definition;
	}();
}
