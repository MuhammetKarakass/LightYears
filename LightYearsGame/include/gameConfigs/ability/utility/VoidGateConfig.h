#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/voidGate/VoidGateContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/voidGate/VoidGatePresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition VoidGatePortalBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::VoidGate::Actor::Portal::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::VoidGatePortal;
		definition.presentationProfileId = ly::VoidGatePresentationIds::PortalBasic;
		return definition;
	}();

	inline const ly::GameAbilityDefinition VoidGate_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::VoidGate::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// Ability1 is the default Q slot. Runtime loadout binding may still move
		// Void Gate to another Ability1-4 slot later.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// Void Gate needs two positional inputs. Echo replay only has one stored
		// activation context, so this staged ability explicitly opts out of history.
		definition.recordInAbilityHistory = false;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::VoidGate
		};
		definition.displayName = "Void Gate";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 145, 80, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::VoidGate;
		return definition;
	}();
}
