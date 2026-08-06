#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/dash/DashContracts.h"

namespace AbilityData
{
	namespace Dash
	{
		// Behavior/schema contract only. Numeric tuning lives in abilities.json.
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Dash_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.Dash.Basic";
			definition.slot = sas::AbilitySlot::Ability3;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Movement" },
				Dash::FamilyTag
			};
			definition.displayName = "Dash";
			definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/star_gold.png";
			definition.inputLabel = "F";
			definition.accentColor = sf::Color{ 120, 220, 255, 255 };
			definition.behaviorId = Dash::BehaviorId;
			return definition;
		}();
	}
}
