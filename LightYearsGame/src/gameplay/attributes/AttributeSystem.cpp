#include "gameplay/attributes/AttributeSystem.h"
#include <algorithm>

namespace ly
{
	void AttributeSystem::RegisterAttribute(const GameplayTag& id, float baseValue, float minValue, float maxValue)
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

	bool AttributeSystem::HasAttribute(const GameplayTag& id) const
	{
		return mAttributes.find(id) != mAttributes.end();
	}

	float AttributeSystem::GetBaseValue(const GameplayTag& id) const
	{
		auto found = mAttributes.find(id);
		return found != mAttributes.end() ? found->second.attribute.baseValue : 0.f;
	}

	float AttributeSystem::GetCurrentValue(const GameplayTag& id, float fallback) const
	{
		auto found = mAttributes.find(id);
		return found != mAttributes.end() ? found->second.attribute.currentValue : fallback;
	}

	float AttributeSystem::GetSequentialReductionMultiplier(const GameplayTag& id, float minimumMultiplier) const
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
		attribute.baseValue = std::clamp(attribute.baseValue, attribute.minValue, attribute.maxValue);
		Recalculate(modifier.attributeId);
	}

	AttributeModifierHandle AttributeSystem::AddModifier(const AttributeModifier& modifier)
	{
		if (!modifier.attributeId.IsValid())
		{
			return {};
		}
		if (!HasAttribute(modifier.attributeId))
		{
			RegisterAttribute(modifier.attributeId, 0.f);
		}

		const unsigned int handleId = mNextHandleId++;
		mAttributes[modifier.attributeId].modifiers[handleId] = modifier;
		mHandleToAttribute[handleId] = modifier.attributeId;
		Recalculate(modifier.attributeId);
		return AttributeModifierHandle{ handleId };
	}

	void AttributeSystem::RemoveModifier(AttributeModifierHandle handle)
	{
		auto foundAttribute = mHandleToAttribute.find(handle.id);
		if (foundAttribute == mHandleToAttribute.end())
		{
			return;
		}

		const GameplayTag attributeId = foundAttribute->second;
		auto foundEntry = mAttributes.find(attributeId);
		if (foundEntry != mAttributes.end())
		{
			foundEntry->second.modifiers.erase(handle.id);
			Recalculate(attributeId);
		}
		mHandleToAttribute.erase(foundAttribute);
	}

	void AttributeSystem::Clear()
	{
		mAttributes.clear();
		mHandleToAttribute.clear();
		mNextHandleId = 1;
		++mRevision;
		onAttributesCleared.Broadcast();
	}

	void AttributeSystem::Recalculate(const GameplayTag& id)
	{
		auto found = mAttributes.find(id);
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
			onAttributeChanged.Broadcast(id, previousValue, value);
		}
	}
}


