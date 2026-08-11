#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/overdriveCore/OverdriveCorePresentationIds.h"

namespace AbilityData::OverdriveCore
{
	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::OverdriveCoreProjectile;
		definition.presentationProfileId = ly::OverdriveCorePresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns only the executable behavior family and semantic identity. The
	// shipped numeric values are loaded from abilities.json.
	inline const ly::GameAbilityDefinition OverdriveCore_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::OverdriveCore::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 0.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::OverdriveCore
		};
		definition.displayName = "Overdrive Core";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
		// Ability4 is bound to R in PlayerMovementComponent. Keeping the
		// definition label in sync prevents the UI from showing a different key.
		definition.inputLabel = "R";
		definition.accentColor = sf::Color{ 255, 95, 45, 255 };
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::OverdriveCore;
		return definition;
	}();
}
