#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const vega::AbilityActorDefinition ReclaimerRepairKitBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::ReclaimerProtocol::Actor::RepairKit::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::ReclaimerRepairKit;
		definition.presentationProfileId =
			vega::ReclaimerProtocolPresentationIds::RepairKitBasic;
		return definition;
	}();

	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition ReclaimerProtocol_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ReclaimerProtocol::AbilityId::Basic;
		// This is only the builtin fallback slot. The runtime loadout owns the
		// player's actual binding, so Reclaimer Protocol is not added to the default
		// loadout by declaring this value here.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::ReclaimerProtocol
		};
		definition.displayName = "Reclaimer Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupGreen_shield.png";
		definition.accentColor = sf::Color{ 80, 220, 140, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::ReclaimerProtocol;
		return definition;
	}();
}
