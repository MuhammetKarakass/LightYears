#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition DirectionalBarrier_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::DirectionalBarrier::AbilityId::Basic;
		// Runtime loadout bindings own the player's actual slot. This fallback
		// slot exists only for legacy validation and content materialization.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::Toggle;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 11.f;
		definition.duration = 5.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::DirectionalBarrier
		};
		definition.displayName = "Directional Barrier";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 90, 190, 255, 235 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::DirectionalBarrier::Attribute::MaxHealthReference,
				100.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::DirectionalBarrier::Attribute::MaxHealthDurationScale,
				0.0025f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::DirectionalBarrier::Attribute::MovementSpeedMultiplier,
				0.80f,
				0.f,
				1.f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Duration,
						sas::AttributeModifierOperation::Add,
						0.05f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.20f
					}
				},
				{},
				{},
				{}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::DirectionalBarrier;
		return definition;
	}();
}
