#pragma once

#include "gameplay/ability/closedCircuit/ClosedCircuitContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "presentation/ability/closedCircuit/ClosedCircuitPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition ClosedCircuitDeliveryBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = AbilityData::ClosedCircuit::Actor::Delivery::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::ClosedCircuitDelivery;
		definition.presentationProfileId = ly::ClosedCircuitPresentationIds::FieldBasic;
		return definition;
	}();

	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition ClosedCircuit_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ClosedCircuit::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.abilityTags = { AbilityData::ClosedCircuit::CategoryTag, AbilityData::ClosedCircuit::FamilyTag };
		definition.displayName = "Closed Circuit";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 80, 220, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::ClosedCircuit;
		return definition;
	}();
}
