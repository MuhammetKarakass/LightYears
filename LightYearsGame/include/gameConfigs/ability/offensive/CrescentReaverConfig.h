#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/crescentReaver/CrescentReaverContracts.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/crescentReaver/CrescentReaverPresentationIds.h"

namespace AbilityData::CrescentReaver
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::CrescentReaverProjectile;
		definition.presentationProfileId = ly::CrescentReaverPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns executable behavior and the spawn policy. Runtime balance is
	// authoritative in abilities.json, where the actor values are also exposed
	// to the content validator and progression pipeline.
	inline const ly::GameAbilityDefinition CrescentReaver_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CrescentReaver::AbilityId::Basic;
		// Ability1 is the shared Q input slot. The runtime loadout may still
		// rebind this ability later without changing the content definition.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 0.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::CrescentReaver
		};
		definition.displayName = "Crescent Reaver";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 125, 210, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::CrescentReaver::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::MouseWorld
				},
				0.f,
				1
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Cooldown,
			ly::AttachmentSchema::Capability::Projectile
		};
		definition.behaviorType = ly::AbilityBehaviorType::CrescentReaver;
		return definition;
	}();
}
