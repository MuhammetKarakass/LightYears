#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/inertialWake/InertialWakeContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/inertialWake/InertialWakePresentationIds.h"

namespace AbilityData::InertialWake
{
	// C++ declares only shape/type/presentation ownership. Numeric balance is
	// loaded from abilities.json so design tuning never forks between code/data.
	inline const ly::AbilityActorDefinition ActorWakeBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Wake::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::InertialWake;
		definition.presentationProfileId = ly::InertialWakePresentationIds::WakeBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition InertialWake_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::InertialWake::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Runtime loadouts, not this catalog fallback, own the player's key slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::InertialWake
		};
		definition.displayName = "Inertial Wake";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 105, 225, 255, 255 };
		definition.actions = {
			ly::AbilityActionSpec{
				sas::AbilityActionPhase::OnActivate,
				ly::SpawnActorAction{ AbilityData::InertialWake::ActorWakeBasic.actorDefinitionId,
					sas::AbilitySpawnPolicy::AtOwner },
				0.f,
				1
			}
		};
		definition.behaviorType = ly::AbilityBehaviorType::InertialWake;
		return definition;
	}();
}
