#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/frozenThrong/FrozenThrongContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "presentation/ability/frozenThrong/FrozenThrongPresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition FrozenThrongHuskBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::FrozenThrong::Actor::Husk::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::FrozenThrongHusk;
		definition.presentationProfileId =
			ly::FrozenThrongPresentationIds::HuskBasic;
		return definition;
	}();

	inline const ly::GameAbilityDefinition FrozenThrong_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FrozenThrong::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// This is only the builtin fallback slot. The runtime loadout owns the
		// player's actual binding, so Frozen Throng is not added to the default
		// loadout by declaring this value here.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::FrozenThrong
		};
		definition.displayName = "Frozen Throng";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 150, 225, 255, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::FrozenThrong;
		return definition;
	}();
}
