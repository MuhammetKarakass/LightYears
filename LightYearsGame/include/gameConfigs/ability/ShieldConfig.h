#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/combat/EffectConfig.h"

namespace AbilityData
{
	namespace Shield
	{
		inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.Shield" };
		inline const ly::GameplayTag FamilyTag{ "Ability.Defense.Shield" };
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Shield_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.Shield.Basic";
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
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
					sas::AbilityActionPhase::OnActivate,
					ly::ApplyEffectAction{ "Effect.Barrier.Basic", sas::AbilityTargetPolicy::Self },
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
							sas::AbilityActionPhase::OnActivate,
							ly::ApplyEffectAction{
								"Effect.Test.BarrierBreak.ThrustBoost",
								sas::AbilityTargetPolicy::Self
							},
							0.f,
							1
						}
					}
				}
			};
			definition.scalingRules = {
				sas::AttributeScalingRule{
					BarrierEffectSchema::Capacity,
					ly::OwnerAttributeIds::MaxHealth,
					sas::AttributeModifierOperation::Add,
					0.2f
				},
				sas::AttributeScalingRule{
					BarrierEffectSchema::Capacity,
					ly::OwnerAttributeIds::Armor,
					sas::AttributeModifierOperation::Add,
					50.f
				}
			};
			definition.behaviorId = Shield::BehaviorId;
			return definition;
		}();
	}
}
