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
		ly::List<ly::GameplayTag> sourceAbilityUpgradeIds;

		bool HasSourceAbilityUpgrade(const ly::GameplayTag& upgradeId) const
		{
			for (const ly::GameplayTag& sourceUpgradeId : sourceAbilityUpgradeIds)
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
		const ly::GameplayTag& attributeId,
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
		const ly::GameplayTag& attributeId,
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
