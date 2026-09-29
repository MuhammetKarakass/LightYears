#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/temporalConvergence/TemporalConvergencePresentationIds.h"

namespace AbilityData::TemporalConvergence
{
	inline const ly::AbilityActorDefinition ActorFieldBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Field::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::TemporalConvergenceField;
		definition.presentationProfileId =
			ly::TemporalConvergencePresentationIds::FieldBasic;
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition TemporalConvergence_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TemporalConvergence::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::TemporalConvergence
		};
		definition.displayName = "Temporal Convergence";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_shield.png";
		definition.accentColor = sf::Color{ 125, 195, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::TemporalConvergence;
		return definition;
	}();
}
