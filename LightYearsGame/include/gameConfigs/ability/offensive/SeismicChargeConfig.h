#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/seismicCharge/SeismicChargeContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/seismicCharge/SeismicChargePresentationIds.h"

namespace AbilityData::SeismicCharge
{
	inline const vega::AbilityActorDefinition ActorBombBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Bomb::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::SeismicChargeBomb;
		definition.presentationProfileId = vega::SeismicChargePresentationIds::BombBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const vega::GameAbilityDefinition SeismicCharge_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::SeismicCharge::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Offense,
			vega::GameplayTags::Ability::Family::SeismicCharge
		};
		definition.displayName = "Seismic Charge";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 85, 190, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::SeismicCharge;
		return definition;
	}();
}
