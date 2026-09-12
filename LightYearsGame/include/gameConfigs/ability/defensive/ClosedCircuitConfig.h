#pragma once

#include "gameplay/ability/closedCircuit/ClosedCircuitContracts.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
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

	inline const ly::GameAbilityDefinition ClosedCircuit_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ClosedCircuit::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.cooldown = 14.f; definition.duration = 0.20f; definition.maxCharges = 1;
		definition.abilityTags = { AbilityData::ClosedCircuit::CategoryTag, AbilityData::ClosedCircuit::FamilyTag };
		definition.displayName = "Closed Circuit";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 80, 220, 255, 255 };
		definition.attributes = {
			{ AbilityData::ClosedCircuit::Attribute::BaseBarrierHealth, 180.f, 0.f },
			{ AbilityData::ClosedCircuit::Attribute::EnergyPowerBarrierHealthScale, 0.60f, 0.f },
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(14, ly::AbilityLevelStep{ {
			{ AbilityData::ClosedCircuit::Attribute::BaseBarrierHealth, sas::AttributeModifierOperation::Add, 15.f },
			{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -0.25f }
		}, {}, {}, {} });
		definition.levelUpgradeScrapCosts = { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 };
		definition.behaviorType = ly::AbilityBehaviorType::ClosedCircuit;
		return definition;
	}();
}
