#pragma once

#include "framework/Core.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "abilities/AbilityDefinition.h"
#include "attributes/AttributeSystem.h"

#include <SFML/Graphics/Color.hpp>
#include <string>

namespace ly
{
	struct AbilityBehaviorSchema
	{
		inline static const GameplayTag Configured{ "GameAbilityBehavior.Configured" };
	};

	struct GameAbilityDefinition : sas::AbilityDefinition
	{
		std::string displayName;
		std::string iconPath;
		std::string inputLabel;
		sf::Color accentColor = sf::Color::White;
		List<AbilityActionSpec> actions;
		List<AbilityTriggerSpec> triggers;
		List<AbilityLevelStep> levelProgression;
		// Indexed by target level minus two: [0] purchases level two.
		// Empty means this ability cannot be purchased through the run economy.
		List<unsigned int> levelUpgradeScrapCosts;
		// Damage identity and attachment compatibility are independent from ability tags and level steps.
		List<GameplayTag> damageTags;
		List<GameplayTag> attachmentCapabilities;
		size_t attachmentSlotCapacity = 2;
		GameplayTag behaviorId = AbilityBehaviorSchema::Configured;

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
