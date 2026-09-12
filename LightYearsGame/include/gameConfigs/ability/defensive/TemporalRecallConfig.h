#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/temporalRecall/TemporalRecallContracts.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition TemporalRecall_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::TemporalRecall::AbilityId::Basic;
		// The ability inventory/loadout owns the real player binding. This fallback
		// is only required while content is validated before an ability is equipped.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.cooldown = 18.f;
		definition.duration = 1.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Defense,
			ly::GameplayTags::Ability::Family::TemporalRecall
		};
		definition.displayName = "Temporal Recall";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 108, 198, 255, 235 };
		definition.attributes = {
			{ AbilityData::TemporalRecall::Attribute::RecallWindow, 3.f, 0.01f },
			{ AbilityData::TemporalRecall::Attribute::FocusDuration, 0.2f, 0.01f },
			{ AbilityData::TemporalRecall::Attribute::RewindDuration, 0.8f, 0.01f },
			{ AbilityData::TemporalRecall::Attribute::PositiveRecoveryRatio, 0.60f, 0.f },
			{ AbilityData::TemporalRecall::Attribute::MaxHealthReference, 100.f, 0.f },
			{ AbilityData::TemporalRecall::Attribute::MaxHealthRecoveryScale, 0.002f, 0.f },
			{ AbilityData::TemporalRecall::Attribute::EnergyPowerReference, 100.f, 0.f },
			{ AbilityData::TemporalRecall::Attribute::EnergyPowerRecoveryScale, 0.002f, 0.f },
			{ AbilityData::TemporalRecall::Attribute::OvercapHoldDuration, 4.f, 0.f },
			{ AbilityData::TemporalRecall::Attribute::OvercapDecayPerSecond, 100.f, 0.f }
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{ {
				{ ly::CommonAttributeIds::Cooldown,
					sas::AttributeModifierOperation::Add, -0.30f },
				{ AbilityData::TemporalRecall::Attribute::PositiveRecoveryRatio,
					sas::AttributeModifierOperation::Add, 0.02f }
			}, {}, {}, {} }
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::TemporalRecall;
		return definition;
	}();
}
