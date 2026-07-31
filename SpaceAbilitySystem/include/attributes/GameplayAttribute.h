#pragma once

#include "framework/Core.h"

#include <algorithm>
#include <limits>

namespace sas
{
	struct GameplayAttribute
	{
		ly::GameplayTag id;
		float baseValue = 0.f;
		float currentValue = 0.f;
		float minValue = 0.f;
		float maxValue = std::numeric_limits<float>::max();

		GameplayAttribute() = default;
		GameplayAttribute(float value) : baseValue{ value }, currentValue{ value } {}
		GameplayAttribute(
			const ly::GameplayTag& inId,
			float value,
			float inMinValue = 0.f,
			float inMaxValue = std::numeric_limits<float>::max()
		)
			: id{ inId },
			baseValue{ value },
			currentValue{ value },
			minValue{ inMinValue },
			maxValue{ inMaxValue }
		{
		}
		GameplayAttribute(float inBaseValue, float inCurrentValue)
			: baseValue{ inBaseValue }, currentValue{ inCurrentValue }
		{
		}

		void ResetToBaseValue() { currentValue = baseValue; }
	};

	using GameplayAttributeList = ly::List<GameplayAttribute>;

	inline GameplayAttribute* FindGameplayAttribute(
		GameplayAttributeList& attributes,
		const ly::GameplayTag& id
	)
	{
		for (GameplayAttribute& attribute : attributes)
		{
			if (attribute.id == id)
			{
				return &attribute;
			}
		}
		return nullptr;
	}

	inline const GameplayAttribute* FindGameplayAttribute(
		const GameplayAttributeList& attributes,
		const ly::GameplayTag& id
	)
	{
		for (const GameplayAttribute& attribute : attributes)
		{
			if (attribute.id == id)
			{
				return &attribute;
			}
		}
		return nullptr;
	}

	inline float FindGameplayAttributeValue(
		const GameplayAttributeList& attributes,
		const ly::GameplayTag& id,
		float fallback = 0.f
	)
	{
		const GameplayAttribute* attribute = FindGameplayAttribute(attributes, id);
		return attribute ? attribute->currentValue : fallback;
	}

	inline bool HasGameplayAttribute(
		const GameplayAttributeList& attributes,
		const ly::GameplayTag& id
	)
	{
		return FindGameplayAttribute(attributes, id) != nullptr;
	}

	inline GameplayAttributeList BuildBaseGameplayAttributes(
		const GameplayAttributeList& attributes
	)
	{
		GameplayAttributeList values;
		values.reserve(attributes.size());
		for (const GameplayAttribute& attribute : attributes)
		{
			values.emplace_back(
				attribute.id,
				std::max(attribute.minValue, attribute.baseValue),
				attribute.minValue,
				attribute.maxValue
			);
		}
		return values;
	}
}
