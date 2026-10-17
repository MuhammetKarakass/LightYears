#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/shield/ShieldContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/EffectConfig.h"

namespace AbilityData
{
	namespace Shield
	{
		// Behavior/action contract only. Numeric tuning lives in abilities.json.
	}

	namespace Definitions
	{
		// Numeric balance, progression and costs are authored in abilities.json.
		inline const vega::GameAbilityDefinition Shield_Basic = []
		{
			vega::GameAbilityDefinition definition;
			definition.abilityId = Shield::AbilityId::Basic;
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.abilityTags = {
				vega::GameplayTags::Ability::Defense,
				vega::GameplayTags::Ability::Family::Shield
			};
			definition.displayName = "Shield";
			definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
			definition.accentColor = sf::Color{ 80, 200, 255, 255 };
			definition.actions = {
				vega::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					vega::ApplyEffectAction{
						BarrierEffectSchema::BasicEffectId,
						sas::AbilityTargetPolicy::Self
					},
					0.f,
					1
				}
			};
			definition.behaviorType = vega::AbilityBehaviorType::Shield;
			return definition;
		}();
	}
}
