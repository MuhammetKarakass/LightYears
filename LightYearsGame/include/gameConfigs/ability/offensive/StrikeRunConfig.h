#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/strikeRun/StrikeRunContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "presentation/ability/strikeRun/StrikeRunPresentationIds.h"

namespace AbilityData::StrikeRun
{
	inline const ly::AbilityActorDefinition ActorBombardmentBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Bombardment::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::StrikeRunBombardment;
		// The bombardment actor owns its preview/telegraph/impact timeline. Its
		// lifetime is therefore controlled by the actor rather than a generic
		// projectile lifetime field.
		definition.presentationProfileId =
			ly::StrikeRunPresentationIds::BombardmentBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition StrikeRun_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::StrikeRun::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadout assignment owns the real input slot. Ability1 only keeps
		// the standalone C++ fallback definition structurally valid.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// The first press defers this duration. On confirmation the behavior ends
		// immediately so cooldown starts at the second press while the delivery
		// actor continues its own 1.4-second presentation timeline.
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::StrikeRun
		};
		definition.displayName = "Strike Run";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 120, 190, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::StrikeRun;
		return definition;
	}();
}
