#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolContracts.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition IroncladProtocol_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::IroncladProtocol::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 22.f;
		definition.duration = 10.f;
		definition.maxCharges = 1;
		definition.abilityTags = { AbilityData::IroncladProtocol::CategoryTag, AbilityData::IroncladProtocol::FamilyTag };
		definition.displayName = "Ironclad Protocol";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		definition.accentColor = sf::Color{ 180, 180, 200, 255 };
		definition.attributes = {
			{ AbilityData::IroncladProtocol::Attribute::MovementSpeedMultiplier, 0.20f, 0.f, 1.f },
			{ AbilityData::IroncladProtocol::Attribute::MinimumFormDuration, 1.f, 0.f, 10.f },
			{ AbilityData::IroncladProtocol::Attribute::MinigunBaseDamage, 8.f, 0.f },
			{ AbilityData::IroncladProtocol::Attribute::BaseDamageReduction, 0.40f, 0.f, 0.95f },
			{ AbilityData::IroncladProtocol::Attribute::MaxHealthReference, 100.f, 0.f },
			{ AbilityData::IroncladProtocol::Attribute::MaximumDamageReductionBonus, 0.20f, 0.f, 0.95f },
			{ AbilityData::IroncladProtocol::Attribute::DamageReductionFalloffHealth, 500.f, 0.01f }
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(14,
			ly::AbilityLevelStep{ {
				{ AbilityData::IroncladProtocol::Attribute::MinigunBaseDamage, sas::AttributeModifierOperation::Add, 2.f },
				{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -0.35f }
			}, {}, {}, {} });
		definition.levelUpgradeScrapCosts = { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 };
		definition.behaviorType = ly::AbilityBehaviorType::IroncladProtocol;
		return definition;
	}();
}
