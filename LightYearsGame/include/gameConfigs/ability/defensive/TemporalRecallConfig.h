#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/temporalRecall/TemporalRecallContracts.h"

namespace AbilityData::Definitions
{
	// Numeric balance, progression and costs are authored in abilities.json.
	inline const ly::GameAbilityDefinition TemporalRecall_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TemporalRecall::AbilityId::Basic;
		// The ability inventory/loadout owns the real player binding. This fallback
		// is only required while content is validated before an ability is equipped.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::TemporalRecall
		};
		definition.displayName = "Temporal Recall";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 108, 198, 255, 235 };
		definition.behaviorType = ly::AbilityBehaviorType::TemporalRecall;
		return definition;
	}();
}
