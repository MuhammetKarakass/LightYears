#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <SFML/System/Vector2.hpp>

namespace ly
{
	// Temporary ship modifiers are owned by a source ID instead of by one
	// ability family. This keeps Phase Drift (and future buffs) from adding
	// ability-specific fields to SpaceShip or the resource components.
	struct ShipRuntimeModifier
	{
		float movementSpeedMultiplier = 1.f;
		float shieldRegenMultiplier = 1.f;
		float afterburnerRegenMultiplier = 1.f;
		// Returns a movement multiplier for the current movement direction. A
		// resolver is optional so ordinary constant modifiers remain unchanged.
		std::function<float(const sf::Vector2f&)> movementSpeedResolver;
	};

	class ShipRuntimeModifiers final
	{
	public:
		void Set(const std::string& sourceId, ShipRuntimeModifier modifier);
		void Remove(const std::string& sourceId);

		float GetMovementSpeedMultiplier() const;
		float GetConditionalMovementSpeedMultiplier(
			const sf::Vector2f& movementDirection
		) const;
		float GetShieldRegenMultiplier() const;
		float GetAfterburnerRegenMultiplier() const;

	private:
		std::unordered_map<std::string, ShipRuntimeModifier> mModifiers;
	};
}
