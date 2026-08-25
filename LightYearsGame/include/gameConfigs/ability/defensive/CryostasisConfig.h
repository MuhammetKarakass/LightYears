#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/cryostasis/CryostasisContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// Fallback/test definition. Shipped balance is loaded from abilities.json;
	// this preserves the C++ behavior identity when JSON is not loaded yet.
	inline const ly::GameAbilityDefinition Cryostasis_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::Cryostasis::AbilityId::Basic;
		// Content validation requires a legal loadout-slot fallback. This is not
		// an equip command: the runtime AbilityLoadout still decides which owned
		// ability occupies each player slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 22.f;
		definition.duration = 5.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::Cryostasis
		};
		definition.displayName = "Cryostasis";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 130, 220, 255, 235 };
		definition.attributes = {
			{ AbilityData::Cryostasis::Attribute::Radius, 300.f, 1.f },
			{ AbilityData::Cryostasis::Attribute::FieldTickDamage, 4.f, 0.f },
			{ AbilityData::Cryostasis::Attribute::FieldTickInterval, 0.25f, 0.01f },
			{ AbilityData::Cryostasis::Attribute::FieldTickDamageMaxHealthScale, 0.02f, 0.f },
			{ AbilityData::Cryostasis::Attribute::BaseIceHealth, 200.f, 1.f },
			{ AbilityData::Cryostasis::Attribute::IceHealthMaxHealthScale, 0.40f, 0.f },
			{ AbilityData::Cryostasis::Attribute::BaseHealthRegenPerSecond, 8.f, 0.f },
			{ AbilityData::Cryostasis::Attribute::HealthRegenMaxHealthScale, 0.04f, 0.f },
			{ AbilityData::Cryostasis::Attribute::BaseEnergyRegenPerSecond, 4.f, 0.f },
			{ AbilityData::Cryostasis::Attribute::EnergyRegenMaxHealthScale, 0.01f, 0.f },
			{ AbilityData::Cryostasis::Attribute::BaseBreakDamage, 70.f, 0.f },
			{ AbilityData::Cryostasis::Attribute::BreakDamageMaxIceHealthScale, 0.30f, 0.f },
			{ AbilityData::Cryostasis::Attribute::FieldCryoStacks, 1.f, 1.f, 1.f },
			{ AbilityData::Cryostasis::Attribute::BreakCryoStacks, 4.f, 4.f, 4.f },
			{ AbilityData::Cryostasis::Attribute::BreakCooldownMultiplier, 0.50f, 0.f, 1.f }
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{ {
				{ AbilityData::Cryostasis::Attribute::FieldTickDamage, sas::AttributeModifierOperation::Add, 1.f },
				{ AbilityData::Cryostasis::Attribute::BaseIceHealth, sas::AttributeModifierOperation::Add, 15.f },
				{ AbilityData::Cryostasis::Attribute::BaseHealthRegenPerSecond, sas::AttributeModifierOperation::Add, 0.75f },
				{ AbilityData::Cryostasis::Attribute::BaseEnergyRegenPerSecond, sas::AttributeModifierOperation::Add, 0.25f },
				{ AbilityData::Cryostasis::Attribute::BaseBreakDamage, sas::AttributeModifierOperation::Add, 5.f },
				{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -0.4f }
			}, {}, {}, {} }
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.damageTags = { ly::DamageTypeSchema::Cryo };
		definition.behaviorType = ly::AbilityBehaviorType::Cryostasis;
		return definition;
	}();
}
