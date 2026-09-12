#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/content/GameAbilityActions.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/content/AbilityBehaviorType.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "abilities/AbilityDefinition.h"
#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"

#include <SFML/Graphics/Color.hpp>
#include <optional>
#include <string>

namespace ly
{
	struct AbilityEffectSpecDefinition
	{
		sas::ContentId effectId;
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
		sf::Color accentColor = sf::Color::White;
		List<AbilityActionSpec> actions;
		List<AbilityTriggerSpec> triggers;
		// Ability-scoped numeric values are materialized from the ability JSON
		// "attributes" array. This list may contain shared Common.* values such
		// as Common.ProjectileCount and feature-local Ability.* values. They are
		// resolved per ability execution and are deliberately not registered in
		// the owner's global AttributeSystem.
		sas::GameplayAttributeList attributes;
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
		AbilityBehaviorType behaviorType = AbilityBehaviorType::Configured;
		// Normal player activations are eligible for the shared ability-use
		// history by default. System abilities such as Echo can opt out without
		// making the history know a family-specific ability ID.
		bool recordInAbilityHistory = true;
		// Invocation systems may apply a final multiplier after the normal
		// attribute and scaling pipeline. The default keeps existing abilities
		// numerically unchanged; Echo uses it for its power rule.
		float attributeOutputMultiplier = 1.f;
		// Explicit list of attributes affected by attributeOutputMultiplier.
		// Decoupled from attribute scaling rules: adding or removing a scaling rule
		// does not change whether an attribute is an invocation output.
		// Initialized by default to { Common.Damage } to preserve existing baseline behavior.
		List<sas::AttributeId> invocationOutputAttributes = { CommonAttributeIds::Damage };
		// Level progression scaling rules materialized during RebuildDefinitionForLevel.
		// Kept separate from base scalingRules so execution order is explicit and deterministic.
		List<sas::AttributeScalingRule> levelScalingRules;

		const AbilityEffectSpecDefinition* FindEffectSpec(const sas::ContentId& effectId) const
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

		bool HasUnlockedUpgrade(const std::string& upgradeId) const
		{
			for (const std::string& unlockedUpgradeId : unlockedUpgradeIds)
			{
				if (unlockedUpgradeId == upgradeId)
				{
					return true;
				}
			}
			return false;
		}
	};

}
