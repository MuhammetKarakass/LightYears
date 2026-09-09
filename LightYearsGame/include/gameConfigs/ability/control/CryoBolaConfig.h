#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/cryoBola/CryoBolaContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/cryoBola/CryoBolaPresentationIds.h"

namespace AbilityData::CryoBola
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::CryoBolaProjectile;
		definition.presentationProfileId = ly::CryoBolaPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ supplies only the behavior identity, spawn policy and typed actor
	// skeleton. The numerical balance data belongs in abilities.json.
	inline const ly::GameAbilityDefinition CryoBola_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CryoBola::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 0.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Control,
			ly::GameplayTags::Ability::Family::CryoBola
		};
		definition.displayName = "Cryo Bola";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		definition.accentColor = sf::Color{ 120, 225, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{
					AbilityData::CryoBola::Actor::Projectile::BasicDefinitionId,
					sas::AbilitySpawnPolicy::OwnerForward,
					sas::AbilityDirectionPolicy::OwnerForward
				},
				0.f,
				1
			}
		};
		definition.damageTags = { ly::DamageTypeSchema::Cryo };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Projectile,
			ly::AttachmentSchema::Capability::Area
		};
		definition.behaviorType = ly::AbilityBehaviorType::CryoBola;
		return definition;
	}();
}
