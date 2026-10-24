#pragma once

#include "framework/Core.h"
#include "gameplay/module/ModuleLevelTable.h"

#include <cstddef>
#include <cstdint>

namespace vega
{
	// Epic Utility / Movement rule structs. Included by ModuleDefinition.h; the authored structs are the
	// only definitions, resolved structs carry one placement's values at its effective level.

	// Cascade Reactor (Utility). Each loadout use from a different family than the previous one advances the chain by one
	// step, up to maxSteps. stepBonusByLevel is the outgoing damage fraction gained per step. windowSeconds is refreshed by
	// every valid use; a repeated family or a timeout restarts the chain.
	struct ModuleCascadeRule
	{
		ModuleLevelTable<float> stepBonusByLevel;
		int maxSteps = 3;
		float windowSeconds = 5.f;
	};

	// Field Mastery (Utility). usesPerLevelUpByLevel is the use count of one Utility ability that grants it one run level.
	// transferUses is the side-feature (Knowledge Transfer) amount added to the other installed Utility abilities whenever
	// this module levels a Utility ability; a rule carries either the main table or transferUses, never both.
	struct ModuleFieldMasteryRule
	{
		ModuleLevelTable<int> usesPerLevelUpByLevel;
		float transferUses = 0.f;
	};

	// Assault Vector (Movement). Moving toward the nearest enemy within targetRange (direction cosine >= minimumCosine,
	// speed >= minimumSpeed) adds damageBonusByLevel to outgoing damage; moving away adds armorBonusByLevel Armor.
	struct ModuleAssaultVectorRule
	{
		ModuleLevelTable<float> damageBonusByLevel;
		ModuleLevelTable<float> armorBonusByLevel;
		float targetRange = 900.f;
		float minimumCosine = 0.5f;
		float minimumSpeed = 100.f;
	};

	// Phase Slip (Movement). An Afterburner start opens phaseSeconds of intangibility during which Primary and loadout
	// abilities are locked. The first Primary hit afterwards detonates an area of explosionRadius at
	// primaryDamageMultiplierByLevel x its damage. cooldownSeconds counts from the phase start.
	struct ModulePhaseSlipRule
	{
		float phaseSeconds = 0.35f;
		float cooldownSeconds = 6.f;
		float explosionRadius = 120.f;
		ModuleLevelTable<float> primaryDamageMultiplierByLevel;
	};

	struct ResolvedModuleCascadeRule
	{
		std::uint32_t instanceId = 0;
		float stepBonus = 0.f;
		int maxSteps = 3;
		float windowSeconds = 5.f;
	};

	struct ResolvedModuleFieldMasteryRule
	{
		std::uint32_t instanceId = 0;
		int usesPerLevelUp = 0;
		float transferUses = 0.f;
	};

	struct ResolvedModuleAssaultVectorRule
	{
		std::uint32_t instanceId = 0;
		float damageBonus = 0.f;
		float armorBonus = 0.f;
		float targetRange = 0.f;
		float minimumCosine = 0.f;
		float minimumSpeed = 0.f;
	};

	struct ResolvedModulePhaseSlipRule
	{
		std::uint32_t instanceId = 0;
		float phaseSeconds = 0.f;
		float cooldownSeconds = 0.f;
		float explosionRadius = 0.f;
		float primaryDamageMultiplier = 1.f;
	};

	struct ResolvedModuleEpicRules
	{
		List<ResolvedModuleCascadeRule> cascades;
		List<ResolvedModuleFieldMasteryRule> fieldMasteries;
		List<ResolvedModuleAssaultVectorRule> assaultVectors;
		List<ResolvedModulePhaseSlipRule> phaseSlips;
	};
}
