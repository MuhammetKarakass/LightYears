#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/shieldGraft/ShieldGraftContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition ShieldGraft_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ShieldGraft::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::ShieldGraft
		};
		definition.displayName = "Shield Graft";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupGreen_shield.png";
		definition.accentColor = sf::Color{ 80, 220, 160, 255 };
		definition.behaviorType = ly::AbilityBehaviorType::ShieldGraft;
		return definition;
	}();
}
