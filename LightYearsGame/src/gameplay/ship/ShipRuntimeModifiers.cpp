#include "gameplay/ship/ShipRuntimeModifiers.h"

#include <algorithm>
#include <cmath>

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
		modifier.thrustBonus = std::isfinite(modifier.thrustBonus)
			? std::max(-1.f, modifier.thrustBonus)
			: 0.f;
		modifier.turnCapabilityMultiplier = std::isfinite(modifier.turnCapabilityMultiplier)
			? std::max(0.f, modifier.turnCapabilityMultiplier)
			: 1.f;
		modifier.shieldRegenMultiplier = std::max(0.f, modifier.shieldRegenMultiplier);
		modifier.afterburnerRegenMultiplier = std::max(0.f, modifier.afterburnerRegenMultiplier);
		modifier.afterburnerEnergyDrainMultiplier = std::max(
			0.f,
			modifier.afterburnerEnergyDrainMultiplier
		);
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
		float result = 1.f;
		for (const auto& [sourceId, modifier] : mModifiers)
		{
			(void)sourceId;
			if (!modifier.movementSpeedResolver)
			{
				continue;
			}

			result *= std::max(
				0.f,
				modifier.movementSpeedResolver(movementDirection)
			);
		}
		return std::max(0.f, result);
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
