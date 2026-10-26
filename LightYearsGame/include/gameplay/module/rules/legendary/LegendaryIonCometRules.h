#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryIonCometRule
	{
		ModuleLevelTable<float> tickDamageByLevel;
		ModuleLevelTable<float> tickEnergyPowerCoefficientByLevel;
		ModuleLevelTable<float> burstDamageByLevel;
		ModuleLevelTable<float> burstEnergyPowerCoefficientByLevel;
		float minimumSpeed = 0.f;
		float baseRadius = 0.f;
		float radiusPerSpeed = 0.f;
		float damageSpeedDivisor = 0.f;
		float diminishingSpeedThreshold = 0.f;
		float diminishingSpeedInterval = 0.f;
		float diminishingSpeedFactor = 0.f;
		float maximumCharge = 0.f;
		float chargePerSecond = 0.f;
		float chargeSpeedSpan = 0.f;
		float brakeWindowSeconds = 0.f;
		float brakeSpeedLossFraction = 0.f;
		int tickElectricStacks = 0;
		int burstElectricStacks = 0;
	};

	struct ResolvedModuleLegendaryIonCometRule
	{
		std::uint32_t instanceId = 0;
		float tickDamage = 0.f;
		float tickEnergyPowerCoefficient = 0.f;
		float burstDamage = 0.f;
		float burstEnergyPowerCoefficient = 0.f;
		float minimumSpeed = 0.f;
		float baseRadius = 0.f;
		float radiusPerSpeed = 0.f;
		float damageSpeedDivisor = 0.f;
		float diminishingSpeedThreshold = 0.f;
		float diminishingSpeedInterval = 0.f;
		float diminishingSpeedFactor = 0.f;
		float maximumCharge = 0.f;
		float chargePerSecond = 0.f;
		float chargeSpeedSpan = 0.f;
		float brakeWindowSeconds = 0.f;
		float brakeSpeedLossFraction = 0.f;
		int tickElectricStacks = 0;
		int burstElectricStacks = 0;
	};
}
