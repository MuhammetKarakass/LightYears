#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/frostMaelstrom/FrostMaelstromPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const vega::AbilityActorDefinition FrostMaelstromFieldBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::FrostMaelstrom::Actor::Field::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::FrostMaelstromField;
		definition.presentationProfileId =
			vega::FrostMaelstromPresentationIds::FieldBasic;
		return definition;
	}();

	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition FrostMaelstrom_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FrostMaelstrom::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Utility,
			vega::GameplayTags::Ability::Family::FrostMaelstrom
		};
		definition.displayName = "Frost Maelstrom";
		definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
		definition.accentColor = sf::Color{ 115, 220, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::FrostMaelstrom;
		return definition;
	}();
}
