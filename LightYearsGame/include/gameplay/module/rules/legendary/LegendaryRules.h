#pragma once

#include "gameplay/module/rules/legendary/LegendarySecondHardpointRules.h"
#include "gameplay/module/rules/legendary/LegendaryMeltdownBarrelRules.h"
#include "gameplay/module/rules/legendary/LegendaryPrimedArsenalRules.h"
#include "gameplay/module/rules/legendary/LegendaryReactionCoreRules.h"
#include "gameplay/module/rules/legendary/LegendaryNeuralHijackerRules.h"
#include "gameplay/module/rules/legendary/LegendaryHardlightHullRules.h"
#include "gameplay/module/rules/legendary/LegendaryBulwarkPulseRules.h"
#include "gameplay/module/rules/legendary/LegendaryParasiteHullRules.h"
#include "gameplay/module/rules/legendary/LegendaryBlinkDriveRules.h"
#include "gameplay/module/rules/legendary/LegendaryIonCometRules.h"
#include "gameplay/module/rules/legendary/LegendaryTwinRackRules.h"
#include "gameplay/module/rules/legendary/LegendaryUnderdogCoreRules.h"

#include <variant>

namespace vega
{
	// The JSON kind is converted once to this closed, typed rule contract.
	using ModuleLegendaryRule = std::variant<
		ModuleLegendarySecondHardpointRule,
		ModuleLegendaryMeltdownBarrelRule,
		ModuleLegendaryPrimedArsenalRule,
		ModuleLegendaryReactionCoreRule,
		ModuleLegendaryNeuralHijackerRule,
		ModuleLegendaryHardlightHullRule,
		ModuleLegendaryBulwarkPulseRule,
		ModuleLegendaryParasiteHullRule,
		ModuleLegendaryBlinkDriveRule,
		ModuleLegendaryIonCometRule,
		ModuleLegendaryTwinRackRule,
		ModuleLegendaryUnderdogCoreRule>;

	struct ResolvedModuleLegendaryRules
	{
		List<ResolvedModuleLegendarySecondHardpointRule> secondHardpoints;
		List<ResolvedModuleLegendaryMeltdownBarrelRule> meltdownBarrels;
		List<ResolvedModuleLegendaryPrimedArsenalRule> primedArsenals;
		List<ResolvedModuleLegendaryReactionCoreRule> reactionCores;
		List<ResolvedModuleLegendaryNeuralHijackerRule> neuralHijackers;
		List<ResolvedModuleLegendaryHardlightHullRule> hardlightHulls;
		List<ResolvedModuleLegendaryBulwarkPulseRule> bulwarkPulses;
		List<ResolvedModuleLegendaryParasiteHullRule> parasiteHulls;
		List<ResolvedModuleLegendaryBlinkDriveRule> blinkDrives;
		List<ResolvedModuleLegendaryIonCometRule> ionComets;
		List<ResolvedModuleLegendaryTwinRackRule> twinRacks;
		List<ResolvedModuleLegendaryUnderdogCoreRule> underdogCores;
	};

	void ResolveLegendaryRules(ResolvedModuleLegendaryRules& effects, const ModuleLegendaryRule& rule, std::uint32_t instanceId, int effectiveLevel);
}
