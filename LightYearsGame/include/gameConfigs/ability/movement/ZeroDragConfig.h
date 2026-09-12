#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/zeroDrag/ZeroDragContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ZeroDrag_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ZeroDrag::AbilityId::Basic;
		// The catalog slot is only a valid placeholder. Runtime loadouts own the
		// player's actual Q/E/F/R binding and may equip this ability anywhere.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 12.f;
		definition.duration = 5.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Movement,
			ly::GameplayTags::Ability::Family::ZeroDrag
		};
		definition.displayName = "Zero Drag";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 150, 230, 255, 255 };

		definition.attributes = {
			{ AbilityData::ZeroDrag::Attribute::EnergyPowerReference, 50.f, 0.f },
			{ AbilityData::ZeroDrag::Attribute::EnergyPowerDurationPerPoint, 0.002f, 0.f },
			// Normal damping remains active. This extra drive force lets the player
			// actually reach the uncapped high-speed state instead of settling near
			// the normal ship's thrust-versus-damping equilibrium.
			{ AbilityData::ZeroDrag::Attribute::ThrustBonus, 0.60f, 0.f },
			{ AbilityData::ZeroDrag::Attribute::NormalizationDuration, 1.1f, 0.01f }
		};

		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Duration,
						sas::AttributeModifierOperation::Add,
						0.10f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.20f
					}
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ZeroDrag;
		return definition;
	}();
}
