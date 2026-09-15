#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "gameplay/math/MultiplierMath.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ly
{
	namespace
	{
		float ResolveProduct(
			const std::unordered_map<std::string, ShipRuntimeModifier>& modifiers,
			float ShipRuntimeModifier::*member
		)
		{
			std::vector<float> values;
			values.reserve(modifiers.size());
			for (const auto& [sourceId, modifier] : modifiers)
			{
				(void)sourceId;
				values.push_back(modifier.*member);
			}
			float resolvedMultiplier = 0.f;
			return math::TryResolveMultiplierProduct(
				values.data(),
				values.size(),
				resolvedMultiplier
			) ? resolvedMultiplier : 0.f;
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

		if (!math::IsFiniteNonNegativeMultiplier(modifier.movementSpeedMultiplier) ||
			!std::isfinite(modifier.thrustBonus) ||
			!math::IsFiniteNonNegativeMultiplier(modifier.turnCapabilityMultiplier) ||
			!math::IsFiniteNonNegativeMultiplier(modifier.shieldRegenMultiplier) ||
			!math::IsFiniteNonNegativeMultiplier(modifier.afterburnerRegenMultiplier) ||
			!math::IsFiniteNonNegativeMultiplier(modifier.afterburnerEnergyDrainMultiplier))
		{
			return;
		}
		modifier.thrustBonus = std::max(-1.f, modifier.thrustBonus);
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

	float ShipRuntimeModifiers::GetThrustBonus() const
	{
		float total = 0.f;
		for (const auto& [sourceId, modifier] : mModifiers)
		{
			(void)sourceId;
			total += modifier.thrustBonus;
		}
		return total;
	}

	float ShipRuntimeModifiers::GetTurnCapabilityMultiplier() const
	{
		return ResolveProduct(mModifiers, &ShipRuntimeModifier::turnCapabilityMultiplier);
	}

	float ShipRuntimeModifiers::GetConditionalMovementSpeedMultiplier(
		const sf::Vector2f& movementDirection
	) const
	{
		std::vector<float> values;
		values.reserve(mModifiers.size());
		for (const auto& [sourceId, modifier] : mModifiers)
		{
			(void)sourceId;
			if (!modifier.movementSpeedResolver)
			{
				continue;
			}

			values.push_back(modifier.movementSpeedResolver(movementDirection));
		}
		float resolvedMultiplier = 0.f;
		return math::TryResolveMultiplierProduct(
			values.data(),
			values.size(),
			resolvedMultiplier
		) ? resolvedMultiplier : 0.f;
	}

	float ShipRuntimeModifiers::GetShieldRegenMultiplier() const
	{
		return ResolveProduct(mModifiers, &ShipRuntimeModifier::shieldRegenMultiplier);
	}

	float ShipRuntimeModifiers::GetAfterburnerRegenMultiplier() const
	{
		return ResolveProduct(mModifiers, &ShipRuntimeModifier::afterburnerRegenMultiplier);
	}

	float ShipRuntimeModifiers::GetAfterburnerEnergyDrainMultiplier() const
	{
		return ResolveProduct(
			mModifiers,
			&ShipRuntimeModifier::afterburnerEnergyDrainMultiplier
		);
	}
}
