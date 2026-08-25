#pragma once

#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/combatSentry/CombatSentryPresentationIds.h"

namespace AbilityData::CombatSentry
{
	// These fallback definitions deliberately carry identity only. Shipped JSON
	// owns balance numbers; keeping them there makes live content and tests use
	// the same data shape without duplicating tuning values in C++.
	inline const ly::AbilityActorDefinition ActorTurretBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Turret::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::CombatSentryTurret;
		definition.presentationProfileId = ly::CombatSentryPresentationIds::TurretBasic;
		return definition;
	}();

	inline const ly::AbilityActorDefinition ActorProjectileBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Projectile::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::CombatSentryProjectile;
		definition.presentationProfileId = ly::CombatSentryPresentationIds::ProjectileBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition CombatSentry_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::CombatSentry::AbilityId::Basic;
		// Only a fallback catalog slot; the player loadout remains runtime-owned.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = AbilityData::CombatSentry::DefaultCooldown;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::CombatSentry
		};
		definition.displayName = "Combat Sentry";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 110, 210, 255, 255 };
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.behaviorType = ly::AbilityBehaviorType::CombatSentry;
		return definition;
	}();
}
