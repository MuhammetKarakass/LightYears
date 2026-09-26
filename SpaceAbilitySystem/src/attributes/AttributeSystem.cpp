#include "attributes/AttributeSystem.h"

#include <algorithm>
#include <exception>
#include <stdexcept>

namespace sas
{
	float ApplyAttributeScalings(
		float baseValue,
		const AttributeId& targetAttributeId,
		const ly::List<AttributeScalingRule>& scalingRules,
		const AttributeSystem& sourceAttributes
	)
	{
		float value = baseValue;
		for (const AttributeScalingRule& scaling : scalingRules)
		{
			if (scaling.targetAttributeId != targetAttributeId)
			{
				continue;
			}

			const float sourceValue =
				sourceAttributes.GetCurrentValue(scaling.sourceAttributeId);
			switch (scaling.operation)
			{
			case AttributeModifierOperation::Add:
				value += sourceValue * scaling.coefficient;
				break;
			case AttributeModifierOperation::Multiply:
				value *= 1.f + sourceValue * scaling.coefficient;
				break;
			case AttributeModifierOperation::Override:
				value = sourceValue * scaling.coefficient;
				break;
			}
		}
		return std::max(0.f, value);
	}

	void AttributeSystem::RegisterAttribute(
		const AttributeId& id,
		float baseValue,
		float minValue,
		float maxValue
	)
	{
		RegisterAttribute(GameplayAttribute{ id, baseValue, minValue, maxValue });
	}

	void AttributeSystem::RegisterAttribute(const GameplayAttribute& attribute)
	{
		const bool alreadyRegistered = HasAttribute(attribute.id);
		GameplayAttributeEntry& entry = mAttributes[attribute.id];
		entry.attribute = attribute;
		Recalculate(attribute.id);
		++mRevision;
		if (!alreadyRegistered)
		{
			onAttributeRegistered.Broadcast(attribute.id);
		}
	}

	bool AttributeSystem::HasAttribute(const AttributeId& id) const
	{
		return mAttributes.find(id) != mAttributes.end();
	}

	float AttributeSystem::GetBaseValue(const AttributeId& id) const
	{
		auto found = mAttributes.find(id);
		return found != mAttributes.end() ? found->second.attribute.baseValue : 0.f;
	}

	float AttributeSystem::GetCurrentValue(
		const AttributeId& id,
		float fallback
	) const
	{
		auto found = mAttributes.find(id);
		return found != mAttributes.end() ? found->second.attribute.currentValue : fallback;
	}

	float AttributeSystem::GetSequentialReductionMultiplier(
		const AttributeId& id,
		float minimumMultiplier
	) const
	{
		auto found = mAttributes.find(id);
		if (found == mAttributes.end())
		{
			return 1.f;
		}

		const GameplayAttributeEntry& entry = found->second;
		auto reductionToMultiplier = [](float reduction)
		{
			return 1.f - std::clamp(reduction, -0.95f, 0.95f);
		};

		bool hasOverride = false;
		int bestOverridePriority = 0;
		float overrideValue = 0.f;
		float multiplier = reductionToMultiplier(entry.attribute.baseValue);

		for (const auto& pair : entry.modifiers)
		{
			const AttributeModifier& modifier = pair.second;
			switch (modifier.operation)
			{
			case AttributeModifierOperation::Add:
				multiplier *= reductionToMultiplier(modifier.magnitude);
				break;
			case AttributeModifierOperation::Multiply:
				multiplier *= std::max(minimumMultiplier, modifier.magnitude);
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

		if (hasOverride)
		{
			multiplier = reductionToMultiplier(overrideValue);
		}

		return std::max(minimumMultiplier, multiplier);
	}

	void AttributeSystem::SetBaseValue(const AttributeId& id, float baseValue)
	{
		if (!id.IsValid())
		{
			return;
		}

		if (!HasAttribute(id))
		{
			RegisterAttribute(id, baseValue);
			return;
		}

		GameplayAttribute& attribute = mAttributes[id].attribute;
		attribute.baseValue = std::clamp(baseValue, attribute.minValue, attribute.maxValue);
		Recalculate(id);
	}

	void AttributeSystem::ApplyBaseModifier(const AttributeModifier& modifier)
	{
		if (!modifier.attributeId.IsValid())
		{
			return;
		}
		if (!HasAttribute(modifier.attributeId))
		{
			RegisterAttribute(modifier.attributeId, 0.f);
		}

		GameplayAttribute& attribute = mAttributes[modifier.attributeId].attribute;
		switch (modifier.operation)
		{
		case AttributeModifierOperation::Add:
			attribute.baseValue += modifier.magnitude;
			break;
		case AttributeModifierOperation::Multiply:
			attribute.baseValue *= modifier.magnitude;
			break;
		case AttributeModifierOperation::Override:
			attribute.baseValue = modifier.magnitude;
			break;
		}
		attribute.baseValue = std::clamp(
			attribute.baseValue,
			attribute.minValue,
			attribute.maxValue
		);
		Recalculate(modifier.attributeId);
	}

	AttributeModifierHandle AttributeSystem::AddModifier(
		const AttributeModifier& modifier
	)
	{
		AttributeModifierHandle committedHandle;
		try
		{
			return AddModifier(modifier, committedHandle);
		}
		catch (...)
		{
			const std::exception_ptr error = std::current_exception();
			if (committedHandle.IsValid())
			{
				try { RemoveModifier(committedHandle); }
				catch (...) {}
			}
			std::rethrow_exception(error);
		}
	}

	AttributeModifierHandle AttributeSystem::AddModifier(
		const AttributeModifier& modifier,
		AttributeModifierHandle& committedHandle
	)
	{
		committedHandle = {};
		if (!modifier.attributeId.IsValid())
		{
			return {};
		}

		const AttributeId attributeId = modifier.attributeId;
		const uint64_t clearGeneration = mClearGeneration;
		if (mNextHandleId == 0)
		{
			throw std::overflow_error("Attribute modifier handle IDs are exhausted.");
		}
		const AttributeModifierHandle handle{ mNextHandleId++ };
		mHandleToAttribute.emplace(handle.id, attributeId);
		committedHandle = handle;

		if (!HasAttribute(modifier.attributeId))
		{
			RegisterAttribute(attributeId, 0.f);
			if (mClearGeneration != clearGeneration ||
				mHandleToAttribute.find(handle.id) == mHandleToAttribute.end())
			{
				return {};
			}
		}

		auto foundEntry = mAttributes.find(attributeId);
		if (foundEntry == mAttributes.end() ||
			mHandleToAttribute.find(handle.id) == mHandleToAttribute.end())
		{
			return {};
		}
		foundEntry->second.modifiers.emplace(handle.id, modifier);
		Recalculate(attributeId);

		if (mClearGeneration != clearGeneration)
		{
			return {};
		}
		const auto foundHandle = mHandleToAttribute.find(handle.id);
		const auto currentEntry = mAttributes.find(attributeId);
		return foundHandle != mHandleToAttribute.end() &&
			currentEntry != mAttributes.end() &&
			currentEntry->second.modifiers.find(handle.id) !=
				currentEntry->second.modifiers.end()
			? handle
			: AttributeModifierHandle{};
	}

	void AttributeSystem::RemoveModifier(AttributeModifierHandle handle)
	{
		auto foundAttribute = mHandleToAttribute.find(handle.id);
		if (foundAttribute == mHandleToAttribute.end())
		{
			return;
		}

		const AttributeId attributeId = foundAttribute->second;
		mHandleToAttribute.erase(foundAttribute);
		auto foundEntry = mAttributes.find(attributeId);
		if (foundEntry != mAttributes.end() &&
			foundEntry->second.modifiers.erase(handle.id) != 0)
		{
			Recalculate(attributeId);
		}
	}

	void AttributeSystem::Clear()
	{
		++mClearGeneration;
		mAttributes.clear();
		mHandleToAttribute.clear();
		// Handle IDs are never recycled. Resetting the counter here would let a
		// handle held across a clear alias a modifier created after it, silently
		// removing the wrong modifier.
		++mRevision;
		onAttributesCleared.Broadcast();
	}

	void AttributeSystem::Recalculate(const AttributeId& id)
	{
		const AttributeId attributeId = id;
		auto found = mAttributes.find(attributeId);
		if (found == mAttributes.end())
		{
			return;
		}

		GameplayAttributeEntry& entry = found->second;
		float value = entry.attribute.baseValue;
		float additive = 0.f;
		float multiplicative = 1.f;
		bool hasOverride = false;
		int bestOverridePriority = 0;
		float overrideValue = 0.f;

		for (const auto& pair : entry.modifiers)
		{
			const AttributeModifier& modifier = pair.second;
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

		value = (value + additive) * multiplicative;
		if (hasOverride)
		{
			value = overrideValue;
		}
		value = std::clamp(value, entry.attribute.minValue, entry.attribute.maxValue);

		const float previousValue = entry.attribute.currentValue;
		entry.attribute.currentValue = value;
		if (previousValue != value)
		{
			++mRevision;
			onAttributeChanged.Broadcast(attributeId, previousValue, value);
		}
	}
}
