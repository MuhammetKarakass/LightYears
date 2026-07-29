#pragma once

#include "gameConfigs/combat/EffectStructs.h"

namespace ly
{
	// Immutable content rules stay in definition. A spec is the resolved payload
	// produced by a weapon, ability, enemy, pickup, or area for one application.
	struct GameplayEffectSpec
	{
		GameplayEffectDefinition definition;
		float duration = 0.f;
		int maxStacks = 1;
		List<AttributeModifier> modifiers;
		GameplayAttributeList attributes;
		List<GameplayTag> sourceAbilityUpgradeIds;

		bool HasSourceAbilityUpgrade(const GameplayTag& upgradeId) const
		{
			for (const GameplayTag& sourceUpgradeId : sourceAbilityUpgradeIds)
			{
				if (sourceUpgradeId.MatchesTag(upgradeId))
				{
					return true;
				}
			}
			return false;
		}
	};

	inline GameplayEffectSpec MakeGameplayEffectSpec(
		const GameplayEffectDefinition& definition
	)
	{
		return GameplayEffectSpec{
			definition,
			definition.duration,
			definition.maxStacks,
			definition.modifiers,
			definition.attributes,
			{}
		};
	}

	inline bool SetGameplayEffectModifierMagnitude(
		GameplayEffectSpec& spec,
		const GameplayTag& attributeId,
		float magnitude
	)
	{
		bool changed = false;
		for (AttributeModifier& modifier : spec.modifiers)
		{
			if (modifier.attributeId == attributeId)
			{
				modifier.magnitude = magnitude;
				changed = true;
			}
		}
		return changed;
	}

	inline bool SetGameplayEffectAttributeBaseValue(
		GameplayEffectSpec& spec,
		const GameplayTag& attributeId,
		float value
	)
	{
		GameplayAttribute* attribute = FindGameplayAttribute(spec.attributes, attributeId);
		if (!attribute)
		{
			return false;
		}
		attribute->baseValue = value;
		attribute->currentValue = value;
		return true;
	}
}
