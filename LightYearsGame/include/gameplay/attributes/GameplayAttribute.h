#pragma once

#include "framework/Core.h"
#include <limits>

namespace ly
{
	struct OwnerAttributeIds
	{
		inline static const GameplayTag MaxHealth{ "Attribute.Owner.MaxHealth" };
		inline static const GameplayTag AttackPower{ "Attribute.Owner.AttackPower" };
		inline static const GameplayTag AbilityHaste{ "Attribute.Owner.AbilityHaste" };
		inline static const GameplayTag MoveSpeedHorizontal{ "Attribute.Owner.MoveSpeedHorizontal" };
		inline static const GameplayTag MoveSpeedVertical{ "Attribute.Owner.MoveSpeedVertical" };
		inline static const GameplayTag Armor{ "Attribute.Owner.Armor" };
		inline static const GameplayTag Luck{ "Attribute.Owner.Luck" };
	};

	// Reusable local attributes declared by ability, weapon, actor, and effect definitions.
	struct CommonAttributeIds
	{
		inline static const GameplayTag Cooldown{ "Attribute.Common.Cooldown" };
		inline static const GameplayTag Damage{ "Attribute.Common.Damage" };
		inline static const GameplayTag Radius{ "Attribute.Common.Radius" };
		inline static const GameplayTag Duration{ "Attribute.Common.Duration" };
		inline static const GameplayTag Interval{ "Attribute.Common.Interval" };
		inline static const GameplayTag FireRate{ "Attribute.Common.FireRate" };
		inline static const GameplayTag Range{ "Attribute.Common.Range" };
		inline static const GameplayTag CollisionRadius{ "Attribute.Collision.Radius" };
		inline static const GameplayTag AreaRadius{ "Attribute.Area.Radius" };
	};

	struct GameplayAttribute
	{
		GameplayTag id;
		float baseValue = 0.f;
		float currentValue = 0.f;
		float minValue = 0.f;
		float maxValue = std::numeric_limits<float>::max();

		GameplayAttribute() = default;
		GameplayAttribute(float value) : baseValue{ value }, currentValue{ value } {}
		GameplayAttribute(const GameplayTag& inId, float value, float inMinValue = 0.f, float inMaxValue = std::numeric_limits<float>::max())
			: id{ inId }, baseValue{ value }, currentValue{ value }, minValue{ inMinValue }, maxValue{ inMaxValue } {}
		GameplayAttribute(float inBaseValue, float inCurrentValue)
			: baseValue{ inBaseValue }, currentValue{ inCurrentValue } {}

		void ResetToBaseValue() { currentValue = baseValue; }
	};

	using GameplayAttributeList = List<GameplayAttribute>;

	inline GameplayAttribute* FindGameplayAttribute(GameplayAttributeList& attributes, const GameplayTag& id)
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

	inline const GameplayAttribute* FindGameplayAttribute(const GameplayAttributeList& attributes, const GameplayTag& id)
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
		const GameplayTag& id,
		float fallback = 0.f
	)
	{
		const GameplayAttribute* attribute = FindGameplayAttribute(attributes, id);
		return attribute ? attribute->currentValue : fallback;
	}
}


