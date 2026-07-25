#pragma once

#include "gameConfigs/ability/AbilityStructs.h"
#include "gameConfigs/combat/EffectConfig.h"

namespace AbilityData
{
	namespace Shield
	{
		inline const ly::GameplayTag BehaviorId{ "AbilityBehavior.Shield" };
		inline const ly::GameplayTag FamilyTag{ "Ability.Defense.Shield" };
	}

	namespace Definitions
	{
		inline const ly::AbilityDefinition Shield_Basic = []
		{
			ly::AbilityDefinition definition;
			definition.abilityId = "Ability.Shield.Basic";
			definition.slot = ly::AbilitySlot::Ability1;
			definition.activationPolicy = ly::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = ly::AbilityLifetimePolicy::Duration;
			definition.cooldown = 8.f;
			definition.duration = 5.f;
			definition.maxCharges = 1;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Defense" },
				Shield::FamilyTag
			};
			definition.displayName = "Shield";
			definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
			definition.inputLabel = "Q";
			definition.accentColor = sf::Color{ 80, 200, 255, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					ly::AbilityActionPhase::OnActivate,
					ly::ApplyEffectAction{ "Effect.Barrier.Basic", ly::AbilityTargetPolicy::Self },
					0.f,
					1
				}
			};
			definition.triggers = {
				ly::AbilityTriggerSpec{
					ly::GameplayTag{ "Event.Owner.BarrierBroken" },
					0.f,
					{},
					{},
					{
						ly::AbilityActionSpec{
							ly::AbilityActionPhase::OnActivate,
							ly::ApplyEffectAction{
								"Effect.Test.BarrierBreak.ThrustBoost",
								ly::AbilityTargetPolicy::Self
							},
							0.f,
							1
						}
					}
				}
			};
			definition.scalingRules = {
				ly::AttributeScalingRule{
					BarrierEffectSchema::Capacity,
					ly::OwnerAttributeIds::MaxHealth,
					ly::AttributeModifierOperation::Add,
					0.2f
				},
				ly::AttributeScalingRule{
					BarrierEffectSchema::Capacity,
					ly::OwnerAttributeIds::Armor,
					ly::AttributeModifierOperation::Add,
					50.f
				}
			};
			definition.behaviorId = Shield::BehaviorId;
			return definition;
		}();
	}
}
