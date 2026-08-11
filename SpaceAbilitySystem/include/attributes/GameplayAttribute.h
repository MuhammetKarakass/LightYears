#pragma once

#include "framework/Core.h"
#include "attributes/AttributeId.h"

#include <algorithm>
#include <limits>

namespace sas
{
	struct GameplayAttribute
	{
		AttributeId id;
		float baseValue = 0.f;
		float currentValue = 0.f;
		float minValue = 0.f;
		float maxValue = std::numeric_limits<float>::max();

		GameplayAttribute() = default;
		GameplayAttribute(float value) : baseValue{ value }, currentValue{ value } {}
		GameplayAttribute(
			const AttributeId& inId,
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

	inline GameplayAttribute* FindAttribute(
		GameplayAttributeList& attributes,
		const AttributeId& id
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

	inline const GameplayAttribute* FindAttribute(
		const GameplayAttributeList& attributes,
		const AttributeId& id
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

	inline float FindAttributeValue(
		const GameplayAttributeList& attributes,
		const AttributeId& id,
		float fallback = 0.f
	)
	{
		const GameplayAttribute* attribute = FindAttribute(attributes, id);
		return attribute ? attribute->currentValue : fallback;
	}

	inline bool HasAttribute(
		const GameplayAttributeList& attributes,
		const AttributeId& id
	)
	{
		return FindAttribute(attributes, id) != nullptr;
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
