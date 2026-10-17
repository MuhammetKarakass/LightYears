#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/temporalConvergence/TemporalConvergencePresentationIds.h"

namespace AbilityData::TemporalConvergence
{
	inline const vega::AbilityActorDefinition ActorFieldBasic = []
	{
		vega::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
		definition.actorType = vega::AbilityActorType::TemporalConvergenceField;
		definition.presentationProfileId =
			vega::TemporalConvergencePresentationIds::FieldBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const vega::GameAbilityDefinition TemporalConvergence_Basic = []
	{
		vega::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TemporalConvergence::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			vega::GameplayTags::Ability::Defense,
			vega::GameplayTags::Ability::Family::TemporalConvergence
		};
		definition.displayName = "Temporal Convergence";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 125, 195, 255, 255 };
		definition.behaviorType = vega::AbilityBehaviorType::TemporalConvergence;
		return definition;
	}();
}
