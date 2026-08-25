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
			{ AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier, 0.80f, 0.f },
			{ AbilityData::ReturnProtocol::Attribute::MaxHealthReference, 100.f, 0.f },
			{ AbilityData::ReturnProtocol::Attribute::MaxHealthDamageScale, 0.001f, 0.f }
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{ {
				{ AbilityData::ReturnProtocol::Attribute::BaseReflectDamageMultiplier,
					sas::AttributeModifierOperation::Add, 0.03f },
				{ ly::CommonAttributeIds::Cooldown,
					sas::AttributeModifierOperation::Add, -0.25f }
			}, {}, {}, {} }
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::ReturnProtocol;
		return definition;
	}();
}
