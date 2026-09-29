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
		definition.duration = 4.5f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Movement,
			ly::GameplayTags::Ability::Family::ZeroDrag
		};
		definition.displayName = "Zero Drag";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 150, 230, 255, 255 };

		definition.attributes = {
			{ AbilityData::ZeroDrag::Attribute::EnergyPowerReference, 100.f, 0.f },
			{ AbilityData::ZeroDrag::Attribute::EnergyPowerDurationScale, 0.50f, 0.f },
			{ AbilityData::ZeroDrag::Attribute::ThrustBonus, 0.80f, 0.f },
			{ AbilityData::ZeroDrag::Attribute::DampingMultiplier, 0.20f, 0.f },
			{ AbilityData::ZeroDrag::Attribute::NormalizationDuration, 1.1f, 0.01f }
		};

		ly::AbilityLevelStep step;
		step.attributeModifiers = {
			{ ly::CommonAttributeIds::Duration, sas::AttributeModifierOperation::Add, 0.10f },
			{ AbilityData::ZeroDrag::Attribute::EnergyPowerDurationScale, sas::AttributeModifierOperation::Add, 0.05f },
			{ AbilityData::ZeroDrag::Attribute::ThrustBonus, sas::AttributeModifierOperation::Add, 0.05f }
		};
		ly::SetRepeatingAbilityLevelStep(definition, std::move(step));
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ZeroDrag;
		return definition;
	}();
}
