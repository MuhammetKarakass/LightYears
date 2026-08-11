#pragma once

#include <string>
#include <unordered_map>

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
	};

	class ShipRuntimeModifiers final
	{
	public:
		void Set(const std::string& sourceId, ShipRuntimeModifier modifier);
		void Remove(const std::string& sourceId);

		float GetMovementSpeedMultiplier() const;
		float GetShieldRegenMultiplier() const;
		float GetAfterburnerRegenMultiplier() const;

	private:
		std::unordered_map<std::string, ShipRuntimeModifier> mModifiers;
	};
}
