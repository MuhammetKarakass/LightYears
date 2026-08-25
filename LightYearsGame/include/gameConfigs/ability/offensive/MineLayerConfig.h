#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/mineLayer/MineLayerContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/mineLayer/MineLayerPresentationIds.h"

namespace AbilityData::MineLayer
{
	inline const ly::AbilityActorDefinition ActorMineBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Mine::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::MineLayerMine;
		definition.presentationProfileId = ly::MineLayerPresentationIds::MineBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns the executable behavior and actor/presentation identity. Numeric
	// balance is authoritative in abilities.json and is copied here only as a
	// safe fallback for tests that do not load shipped JSON.
	inline const ly::GameAbilityDefinition MineLayer_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::MineLayer::AbilityId::Basic;
		// Ability1 is the default Q binding; the runtime loadout may still rebind it.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = 9.f;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::MineLayer
		};
		definition.displayName = "Mine Layer";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 80, 175, 255, 255 };
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage,
			ly::AttachmentSchema::Capability::Cooldown,
			ly::AttachmentSchema::Capability::Area
		};
		definition.behaviorType = ly::AbilityBehaviorType::MineLayer;
		return definition;
	}();
}
