#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/shield/ShieldContracts.h"
#include "gameConfigs/combat/EffectConfig.h"

namespace AbilityData
{
	namespace Shield
	{
		// Behavior/action contract only. Numeric tuning lives in abilities.json.
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Shield_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = Shield::AbilityId::Basic;
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				Shield::CategoryTag,
				Shield::FamilyTag
			};
			definition.displayName = "Shield";
			definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
			definition.inputLabel = "Q";
			definition.accentColor = sf::Color{ 80, 200, 255, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::ApplyEffectAction{
						BarrierEffectSchema::BasicEffectId,
						sas::AbilityTargetPolicy::Self
					},
					0.f,
					1
				}
			};
			definition.triggers = {
				ly::AbilityTriggerSpec{
					BarrierEffectSchema::BrokenEventTag,
					0.f,
					{},
					{},
					{
						ly::AbilityActionSpec{
							sas::AbilityActionPhase::OnActivate,
							ly::ApplyEffectAction{
								BarrierEffectSchema::BreakThrustBoostEffectId,
								sas::AbilityTargetPolicy::Self
							},
							0.f,
							1
						}
					}
				}
			};
			definition.behaviorTag = Shield::BehaviorTag;
			return definition;
		}();
	}
}
