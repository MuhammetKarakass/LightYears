#pragma once

#include "gameplay/ability/closedCircuit/ClosedCircuitContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "presentation/ability/closedCircuit/ClosedCircuitPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const vega::AbilityActorDefinition ClosedCircuitDeliveryBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = AbilityData::ClosedCircuit::Actor::Delivery::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::ClosedCircuitDelivery;
		definition.presentationProfileId = vega::ClosedCircuitPresentationIds::FieldBasic;
		return definition;
	}();

	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition ClosedCircuit_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ClosedCircuit::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.abilityTags = { AbilityData::ClosedCircuit::CategoryTag, AbilityData::ClosedCircuit::FamilyTag };
		definition.displayName = "Closed Circuit";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 80, 220, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::ClosedCircuit;
		return definition;
	}();
}
