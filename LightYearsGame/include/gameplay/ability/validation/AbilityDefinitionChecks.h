#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"

#include <cmath>

namespace vega::ability_checks
{
	// Shared helpers for per-ability definition validation. Abilities with a
	// different epsilon or different semantics keep their own local copy.
	inline bool NearlyEqual(float left, float right)
	{
		return std::isfinite(left) && std::isfinite(right) && std::abs(left - right) <= 0.0001f;
	}

	inline const sas::GameplayAttribute* FindAttribute(const GameAbilityDefinition& definition, const sas::AttributeId& attributeId)
	{
		return sas::FindAttribute(definition.attributes, attributeId);
	}

	inline bool HasValidAttribute(const GameAbilityDefinition& definition, const sas::AttributeId& attributeId, float minimumBaseValue, bool requireWholeNumber = false)
	{
		const sas::GameplayAttribute* attribute = FindAttribute(definition, attributeId);
		if (!attribute ||
			!std::isfinite(attribute->baseValue) ||
			!std::isfinite(attribute->currentValue) ||
			!std::isfinite(attribute->minValue) ||
			!std::isfinite(attribute->maxValue) ||
			attribute->minValue > attribute->maxValue ||
			attribute->baseValue < minimumBaseValue ||
			attribute->baseValue < attribute->minValue ||
			attribute->baseValue > attribute->maxValue)
		{
			return false;
		}

		return !requireWholeNumber || std::round(attribute->baseValue) == attribute->baseValue;
	}

	inline bool HasModifier(const AbilityLevelStep& step, const sas::AttributeId& attributeId, float magnitude)
	{
		for (const sas::AttributeModifier& modifier : step.attributeModifiers)
		{
			if (modifier.attributeId == attributeId &&
				modifier.operation == sas::AttributeModifierOperation::Add &&
				std::abs(modifier.magnitude - magnitude) <= 0.0001f)
			{
				return true;
			}
		}

		return false;
	}

	inline bool HasSignedModifier(const AbilityLevelStep& step, const sas::AttributeId& attributeId, bool positive)
	{
		for (const sas::AttributeModifier& modifier : step.attributeModifiers)
		{
			if (modifier.attributeId == attributeId &&
				modifier.operation == sas::AttributeModifierOperation::Add &&
				std::isfinite(modifier.magnitude) &&
				(positive ? modifier.magnitude > 0.f : modifier.magnitude < 0.f))
			{
				return true;
			}
		}

		return false;
	}

	inline bool HasExactAbilityTags(const GameAbilityDefinition& definition, const GameplayTag& categoryTag, const GameplayTag& familyTag)
	{
		if (definition.abilityTags.size() != 2)
		{
			return false;
		}

		bool hasCategory = false;
		bool hasFamily = false;
		for (const GameplayTag& tag : definition.abilityTags)
		{
			hasCategory = hasCategory || tag.MatchesTagExact(categoryTag);
			hasFamily = hasFamily || tag.MatchesTagExact(familyTag);
		}

		return hasCategory && hasFamily;
	}
}
