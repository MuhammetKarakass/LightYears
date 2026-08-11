#pragma once

#include "framework/Core.h"
#include "framework/Delegate.h"
#include "attributes/GameplayAttribute.h"

#include <algorithm>
#include <limits>

namespace sas
{
	enum class AttributeModifierOperation
	{
		Add,
		Multiply,
		Override
	};

	struct AttributeModifier
	{
		AttributeId attributeId;
		AttributeModifierOperation operation = AttributeModifierOperation::Add;
		float magnitude = 0.f;
		int priority = 0;

		AttributeModifier() = default;
		AttributeModifier(const AttributeId& inAttributeId, float inMagnitude)
			: attributeId{ inAttributeId },
			operation{ AttributeModifierOperation::Add },
			magnitude{ inMagnitude },
			priority{ 0 }
		{
		}
		AttributeModifier(
			const AttributeId& inAttributeId,
			AttributeModifierOperation inOperation,
			float inMagnitude,
			int inPriority = 0
		)
			: attributeId{ inAttributeId },
			operation{ inOperation },
			magnitude{ inMagnitude },
			priority{ inPriority }
		{
		}
	};

	struct AttributeScalingRule
	{
		AttributeId targetAttributeId;
		AttributeId sourceAttributeId;
		AttributeModifierOperation operation = AttributeModifierOperation::Multiply;
		float coefficient = 1.f;
	};

	struct AttributeModifierHandle
	{
		unsigned int id = 0;
		bool IsValid() const { return id != 0; }
		bool operator==(const AttributeModifierHandle& other) const { return id == other.id; }
	};

	inline float CalculateModifiedAttributeValue(
		const GameplayAttribute& attribute,
		const ly::List<AttributeModifier>& modifiers
	)
	{
		float additive = 0.f;
		float multiplicative = 1.f;
		bool hasOverride = false;
		int bestOverridePriority = 0;
		float overrideValue = 0.f;

		for (const AttributeModifier& modifier : modifiers)
		{
			if (modifier.attributeId != attribute.id)
			{
				continue;
			}

			switch (modifier.operation)
			{
			case AttributeModifierOperation::Add:
				additive += modifier.magnitude;
				break;
			case AttributeModifierOperation::Multiply:
				multiplicative *= modifier.magnitude;
				break;
			case AttributeModifierOperation::Override:
				if (!hasOverride || modifier.priority >= bestOverridePriority)
				{
					hasOverride = true;
					bestOverridePriority = modifier.priority;
					overrideValue = modifier.magnitude;
				}
				break;
			}
		}

		float value = (attribute.baseValue + additive) * multiplicative;
		if (hasOverride)
		{
			value = overrideValue;
		}
		return std::clamp(value, attribute.minValue, attribute.maxValue);
	}

	struct GameplayAttributeEntry
	{
		GameplayAttribute attribute;
		ly::Map<unsigned int, AttributeModifier> modifiers;
	};

	class AttributeSystem
	{
	public:
	void RegisterAttribute(
			const AttributeId& id,
			float baseValue,
			float minValue = 0.f,
			float maxValue = std::numeric_limits<float>::max()
		);
		void RegisterAttribute(const GameplayAttribute& attribute);
		bool HasAttribute(const AttributeId& id) const;
		uint64_t GetRevision() const { return mRevision; }
		float GetBaseValue(const AttributeId& id) const;
		float GetCurrentValue(const AttributeId& id, float fallback = 0.f) const;
		float GetSequentialReductionMultiplier(
			const AttributeId& id,
			float minimumMultiplier = 0.05f
		) const;
		void SetBaseValue(const AttributeId& id, float baseValue);
		void ApplyBaseModifier(const AttributeModifier& modifier);
		AttributeModifierHandle AddModifier(const AttributeModifier& modifier);
		void RemoveModifier(AttributeModifierHandle handle);
		void Clear();

		ly::Delegate<AttributeId, float, float> onAttributeChanged;
		ly::Delegate<AttributeId> onAttributeRegistered;
		ly::Delegate<> onAttributesCleared;

	private:
		void Recalculate(const AttributeId& id);

		ly::Dictionary<
			AttributeId,
			GameplayAttributeEntry,
			AttributeIdHash
		> mAttributes;
		ly::Map<unsigned int, AttributeId> mHandleToAttribute;
		unsigned int mNextHandleId = 1;
		uint64_t mRevision = 1;
	};

	float ApplyAttributeScalings(
		float baseValue,
		const AttributeId& targetAttributeId,
		const ly::List<AttributeScalingRule>& scalingRules,
		const AttributeSystem& sourceAttributes
	);
}
