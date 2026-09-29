#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolContracts.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition ReturnProtocol_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::ReturnProtocol::AbilityId::Basic;
		// Runtime loadouts decide the real key/slot. This is only the required
		// fallback slot for content validation before an ability is equipped.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 14.f;
		definition.duration = 1.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::ReturnProtocol
		};
		definition.displayName = "Return Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 105, 225, 255, 235 };
		definition.attributes = {
			{ AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier, 0.90f, 0.f },
			{ AbilityData::ReturnProtocol::Attribute::AttackPowerReference, 100.f, 0.f },
			{ AbilityData::ReturnProtocol::Attribute::AttackPowerScale, 0.08f, 0.f },
			{ AbilityData::ReturnProtocol::Attribute::EnergyPowerReference, 100.f, 0.f },
			{ AbilityData::ReturnProtocol::Attribute::EnergyPowerScale, 0.12f, 0.f }
		};
		ly::SetRepeatingAbilityLevelStep(definition, ly::AbilityLevelStep{
			{
				{ AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier,
					sas::AttributeModifierOperation::Add, 0.04f },
				{ AbilityData::ReturnProtocol::Attribute::AttackPowerScale,
					sas::AttributeModifierOperation::Add, 0.01f },
				{ AbilityData::ReturnProtocol::Attribute::EnergyPowerScale,
					sas::AttributeModifierOperation::Add, 0.01f }
			}, {}, {}, {}
		});
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ReturnProtocol;
		return definition;
	}();
}
