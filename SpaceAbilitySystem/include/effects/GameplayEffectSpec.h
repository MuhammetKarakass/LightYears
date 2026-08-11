#pragma once

#include "effects/GameplayEffectDefinition.h"

namespace sas
{
	struct GameplayEffectSpec
	{
		GameplayEffectDefinition definition;
		float duration = 0.f;
		int maxStacks = 1;
		ly::List<AttributeModifier> modifiers;
		GameplayAttributeList attributes;
		ly::List<std::string> sourceAbilityUpgradeIds;

		bool HasSourceAbilityUpgrade(const std::string& upgradeId) const
		{
			for (const std::string& sourceUpgradeId : sourceAbilityUpgradeIds)
			{
				if (sourceUpgradeId == upgradeId)
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
		const AttributeId& attributeId,
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
		const AttributeId& attributeId,
		float value
	)
	{
		GameplayAttribute* attribute = FindAttribute(spec.attributes, attributeId);
		if (!attribute)
		{
			return false;
		}
		attribute->baseValue = value;
		attribute->currentValue = value;
		return true;
	}
}
