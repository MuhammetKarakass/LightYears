#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/dash/DashContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData
{
	namespace Dash
	{
		// Feature schema only. Numeric tuning lives in abilities.json.
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Dash_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = Dash::AbilityId::Basic;
			definition.slot = sas::AbilitySlot::Ability3;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTags::Ability::Movement,
				ly::GameplayTags::Ability::Family::Dash
			};
			definition.displayName = "Dash";
			definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/star_gold.png";
			definition.inputLabel = "F";
			definition.accentColor = sf::Color{ 120, 220, 255, 255 };
			definition.behaviorType = ly::AbilityBehaviorType::Dash;
			return definition;
		}();
	}
}
