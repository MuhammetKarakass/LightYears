#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition DirectionalBarrier_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::DirectionalBarrier::AbilityId::Basic;
		// Runtime loadout bindings own the player's actual slot. This fallback
		// slot exists only for legacy validation and content materialization.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::Toggle;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::DirectionalBarrier
		};
		definition.displayName = "Directional Barrier";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 90, 190, 255, 235 };
		definition.behaviorType = ly::AbilityBehaviorType::DirectionalBarrier;
		return definition;
	}();
}
