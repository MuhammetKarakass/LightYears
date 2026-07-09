#pragma once

#include "framework/Core.h"
#include <string>

namespace ly
{
	struct GameplayAttribute
	{
		float baseValue = 0.f;
		float currentValue = 0.f;

		GameplayAttribute() = default;

		GameplayAttribute(float value)
			: baseValue{ value },
			currentValue{ value }
		{
		}

		GameplayAttribute(float inBaseValue, float inCurrentValue)
			: baseValue{ inBaseValue },
			currentValue{ inCurrentValue }
		{
		}

		void ResetToBaseValue()
		{
			currentValue = baseValue;
		}
	};

	enum class AttributeModifierOperation
	{
		Add,
		Multiply,
		Override
	};

	struct AttributeModifier
	{
		std::string attributeName;
		AttributeModifierOperation operation = AttributeModifierOperation::Add;
		float magnitude = 0.f;
	};

	enum class GameplayEffectDurationPolicy
	{
		Instant,
		Duration
	};

	enum class GameplayEffectStackingPolicy
	{
		None,
		RefreshDuration,
		Stack
	};

	struct GameplayEffect
	{
		std::string effectName;
		List<AttributeModifier> modifiers;
		GameplayEffectDurationPolicy durationPolicy = GameplayEffectDurationPolicy::Instant;
		GameplayEffectStackingPolicy stackingPolicy = GameplayEffectStackingPolicy::None;
		float duration = 0.f;
		int maxStacks = 1;

		// TODO(GAS-Lite): Effect ticking, duration expiration, stack refresh, and stack merging are intentionally not implemented yet.
		// This struct is the data shape we will connect to a real effect application system later.
	};

	class AttributeSet
	{
	public:
		void SetAttribute(const std::string& attributeName, const GameplayAttribute& attribute)
		{
			mAttributes[attributeName] = attribute;
		}

		GameplayAttribute* FindAttribute(const std::string& attributeName)
		{
			auto iter = mAttributes.find(attributeName);
			return iter != mAttributes.end() ? &iter->second : nullptr;
		}

		const GameplayAttribute* FindAttribute(const std::string& attributeName) const
		{
			auto iter = mAttributes.find(attributeName);
			return iter != mAttributes.end() ? &iter->second : nullptr;
		}

		void ResetCurrentValuesToBase()
		{
			for (auto& pair : mAttributes)
			{
				pair.second.ResetToBaseValue();
			}
		}

		// TODO(GAS-Lite): Modifier application is deliberately minimal for now.
		// The first movement pass will read typed attributes directly; generic modifier routing comes next.
	private:
		Dictionary<std::string, GameplayAttribute> mAttributes;
	};
}
