#pragma once

#include "gameplay/module/ModuleLevelTable.h"

#include <cstdint>

namespace vega
{
	struct ModuleLegendaryReactionCoreRule
	{
		ModuleLevelTable<float> steamSecondsByLevel;
		float steamDeviationDegrees = 0.f;
		float magnetSeconds = 0.f;
		ModuleLevelTable<float> magnetSpeedByLevel;
		float magnetRadius = 0.f;
		ModuleLevelTable<float> plasmaDamageByLevel;
		ModuleLevelTable<float> plasmaEnergyPowerCoefficientByLevel;
		float plasmaRadius = 0.f;
		ModuleLevelTable<float> fractureSecondsByLevel;
		float bossArmorReductionFraction = 0.f;
		ModuleLevelTable<float> stunSecondsByLevel;
		int shrapnelCount = 0;
		ModuleLevelTable<float> shrapnelDamageByLevel;
		ModuleLevelTable<float> shrapnelAttackPowerCoefficientByLevel;
		ModuleLevelTable<float> shrapnelEnergyPowerCoefficientByLevel;
		int shrapnelMaxHitsPerTarget = 0;
		float shrapnelRadius = 0.f;
		float targetCooldownSeconds = 0.f;
	};

	struct ResolvedModuleLegendaryReactionCoreRule
	{
		std::uint32_t instanceId = 0;
		float steamSeconds = 0.f;
		float steamDeviationDegrees = 0.f;
		float magnetSeconds = 0.f;
		float magnetSpeed = 0.f;
		float magnetRadius = 0.f;
		float plasmaDamage = 0.f;
		float plasmaEnergyPowerCoefficient = 0.f;
		float plasmaRadius = 0.f;
		float fractureSeconds = 0.f;
		float bossArmorReductionFraction = 0.f;
		float stunSeconds = 0.f;
		int shrapnelCount = 0;
		float shrapnelDamage = 0.f;
		float shrapnelAttackPowerCoefficient = 0.f;
		float shrapnelEnergyPowerCoefficient = 0.f;
		int shrapnelMaxHitsPerTarget = 0;
		float shrapnelRadius = 0.f;
		float targetCooldownSeconds = 0.f;
	};
}
