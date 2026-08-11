#include "gameplay/ship/ShipRuntimeModifiers.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		float ResolveProduct(
			const std::unordered_map<std::string, ShipRuntimeModifier>& modifiers,
			float ShipRuntimeModifier::*member
		)
		{
			float result = 1.f;
			for (const auto& [sourceId, modifier] : modifiers)
			{
				(void)sourceId;
				result *= std::max(0.f, modifier.*member);
			}
			return std::max(0.f, result);
		}
	}

	void ShipRuntimeModifiers::Set(
		const std::string& sourceId,
		ShipRuntimeModifier modifier
	)
	{
		if (sourceId.empty())
		{
			return;
		}

		modifier.movementSpeedMultiplier = std::max(0.f, modifier.movementSpeedMultiplier);
		modifier.shieldRegenMultiplier = std::max(0.f, modifier.shieldRegenMultiplier);
		modifier.afterburnerRegenMultiplier = std::max(0.f, modifier.afterburnerRegenMultiplier);
		mModifiers[sourceId] = modifier;
	}

	void ShipRuntimeModifiers::Remove(const std::string& sourceId)
	{
		mModifiers.erase(sourceId);
	}

	float ShipRuntimeModifiers::GetMovementSpeedMultiplier() const
	{
		return ResolveProduct(mModifiers, &ShipRuntimeModifier::movementSpeedMultiplier);
	}

	float ShipRuntimeModifiers::GetShieldRegenMultiplier() const
	{
		return ResolveProduct(mModifiers, &ShipRuntimeModifier::shieldRegenMultiplier);
	}

	float ShipRuntimeModifiers::GetAfterburnerRegenMultiplier() const
	{
		return ResolveProduct(mModifiers, &ShipRuntimeModifier::afterburnerRegenMultiplier);
	}
}
