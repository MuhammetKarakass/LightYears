#pragma once

#include "framework/Core.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "abilities/AbilityDefinition.h"
#include "attributes/AttributeSystem.h"

#include <SFML/Graphics/Color.hpp>
#include <optional>
#include <string>

namespace ly
{
	struct AbilityBehaviorSchema
	{
		// This is the generic fallback behavior used by content-only definitions.
		// Concrete ability families own their behavior tags in feature contracts.
		inline static const GameplayTag Configured{ "GameAbilityBehavior.Configured" };
	};

	struct AbilityEffectSpecDefinition
	{
		std::string effectId;
		bool useAbilityDuration = false;
		std::optional<float> duration;
		std::optional<int> maxStacks;
		List<sas::AttributeModifier> modifiers;
		sas::GameplayAttributeList attributes;
	};

	struct GameAbilityDefinition : sas::AbilityDefinition
	{
		std::string displayName;
		std::string iconPath;
		std::string inputLabel;
		sf::Color accentColor = sf::Color::White;
		List<AbilityActionSpec> actions;
		List<AbilityTriggerSpec> triggers;
		// Source-owned balance values used to parameterize policy-only effects.
		List<AbilityEffectSpecDefinition> effectSpecs;
		List<AbilityLevelStep> levelProgression;
		// Indexed by target level minus two: [0] purchases level two.
		// Empty means this ability cannot be purchased through the run economy.
		List<unsigned int> levelUpgradeScrapCosts;
		// Damage identity and attachment compatibility are independent from ability tags and level steps.
		List<GameplayTag> damageTags;
		List<GameplayTag> attachmentCapabilities;
		size_t attachmentSlotCapacity = 2;
		GameplayTag behaviorTag = AbilityBehaviorSchema::Configured;

		const AbilityEffectSpecDefinition* FindEffectSpec(const std::string& effectId) const
		{
			for (const AbilityEffectSpecDefinition& spec : effectSpecs)
			{
				if (spec.effectId == effectId)
				{
					return &spec;
				}
			}
			return nullptr;
		}

		int GetMaxLevel() const
		{
			return 1 + static_cast<int>(levelProgression.size());
		}

		bool HasScrapCostToReachLevel(int targetLevel) const
		{
			return targetLevel >= 2 &&
				targetLevel <= GetMaxLevel() &&
				levelUpgradeScrapCosts.size() == levelProgression.size() &&
				levelUpgradeScrapCosts[static_cast<size_t>(targetLevel - 2)] > 0;
		}

		unsigned int GetScrapCostToReachLevel(int targetLevel) const
		{
			return HasScrapCostToReachLevel(targetLevel)
				? levelUpgradeScrapCosts[static_cast<size_t>(targetLevel - 2)]
				: 0;
		}

		bool HasUnlockedUpgrade(const GameplayTag& upgradeId) const
		{
			for (const GameplayTag& unlockedUpgradeId : unlockedUpgradeIds)
			{
				if (unlockedUpgradeId.MatchesTag(upgradeId))
				{
					return true;
				}
			}
			return false;
		}
	};

}
