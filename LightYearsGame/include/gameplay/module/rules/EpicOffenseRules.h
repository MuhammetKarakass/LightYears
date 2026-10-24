#pragma once

#include "framework/Core.h"
#include "gameplay/module/ModuleLevelTable.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <string>

namespace vega
{
	class Actor;

	// Epic Offense rules (Prism Core, Blood Contract and their private side features). Main-package rules are level dependent
	// through their tables; side features carry constants only. Resolved values live in EpicOffenseEffects.

	// Prism Core: every Offense use advances a Thermal, Cryo, Electric, Kinetic cycle. The use's hits apply the cycle status as an
	// extra status with extraStacks stacks; its duration is scaled by statusDurationMultiplier at the placement level.
	struct ModulePrismCoreRule
	{
		int extraStacks = 1;
		float statusDurationMultiplier = 1.f;
		ModuleLevelTable<float> statusDurationMultiplierByLevel;
	};

	// Storm Capacitor (side feature): Q/E/F/R uses charge up to chargesRequired. The next eligible Primary hit discharges the charges
	// onto targetCount random opposing combatants in radius: each takes primaryDamageFraction of the triggering hit and electricStacks Electric stacks.
	struct ModuleStormCapacitorRule
	{
		int chargesRequired = 4;
		int targetCount = 3;
		int electricStacks = 2;
		float radius = 500.f;
		float primaryDamageFraction = 0.5f;
	};

	// Alternating Current (side feature): a direct ability hit on an enemy whose previous direct ability hit had another damage type
	// deals damageMultiplier. DoT ticks and Primary hits neither change the sequence nor receive the bonus.
	struct ModuleAlternatingCurrentRule
	{
		float damageMultiplier = 1.2f;
	};

	// Blood Contract: every Offense use spends costFraction of current health (never lethal, shield bypassed). An enemy hit by an
	// Offense ability that dies within healWindowSeconds heals its owner healFraction of MaxHealth. Offense damage rises linearly
	// from fullHealthMultiplier (full health) to emptyHealthMultiplier (no health).
	struct ModuleBloodContractRule
	{
		float costFraction = 0.06f;
		float healFraction = 0.01f;
		float healWindowSeconds = 4.f;
		float fullHealthMultiplier = 1.f;
		ModuleLevelTable<float> fullHealthMultiplierByLevel;
		float emptyHealthMultiplier = 1.f;
		ModuleLevelTable<float> emptyHealthMultiplierByLevel;
	};

	// Clot Armor (side feature of Blood Contract): each Offense use grants Armor of armorPerCost x the health it spent, for seconds.
	// The grant refreshes and replaces the previous one; it does not stack.
	struct ModuleClotArmorRule
	{
		float armorPerCost = 2.f;
		float seconds = 4.f;
	};

	// Debt Collection (side feature of Blood Contract): the heal window becomes healWindowSeconds.
	struct ModuleDebtCollectionRule
	{
		float healWindowSeconds = 6.f;
	};

	// Package-level list of Epic Offense rules, one entry per declared rule.
	struct ModuleEpicOffenseRules
	{
		List<ModulePrismCoreRule> prismCores;
		List<ModuleStormCapacitorRule> stormCapacitors;
		List<ModuleAlternatingCurrentRule> alternatingCurrents;
		List<ModuleBloodContractRule> bloodContracts;
		List<ModuleClotArmorRule> clotArmors;
		List<ModuleDebtCollectionRule> debtCollections;

		bool Empty() const
		{
			return prismCores.empty() && stormCapacitors.empty() && alternatingCurrents.empty() && bloodContracts.empty() && clotArmors.empty() && debtCollections.empty();
		}
	};

	// Resolved values of one ship, aggregated over every placed Epic Offense rule. Numeric fields take the strongest value (max),
	// flags are set when any placement provides the rule.
	struct EpicOffenseEffects
	{
		bool prismCore = false;
		float prismDurationMultiplier = 1.f;
		int prismExtraStacks = 0;

		bool stormCapacitor = false;
		int stormChargesRequired = 0;
		int stormTargetCount = 0;
		int stormElectricStacks = 0;
		float stormRadius = 0.f;
		float stormPrimaryDamageFraction = 0.f;

		bool alternatingCurrent = false;
		float alternatingMultiplier = 1.f;

		bool bloodContract = false;
		float bloodCostFraction = 0.f;
		float bloodHealFraction = 0.f;
		float bloodHealWindowSeconds = 0.f;
		float bloodFullHealthMultiplier = 1.f;
		float bloodEmptyHealthMultiplier = 1.f;
		// Debt Collection window; the effective heal window is the larger of this and bloodHealWindowSeconds.
		float debtHealWindowSeconds = 0.f;

		float clotArmorPerCost = 0.f;
		float clotArmorSeconds = 0.f;

		bool Empty() const { return !prismCore && !stormCapacitor && !alternatingCurrent && !bloodContract && clotArmorPerCost <= 0.f && debtHealWindowSeconds <= 0.f; }
		float EffectiveBloodHealWindowSeconds() const { return std::max(bloodHealWindowSeconds, debtHealWindowSeconds); }
	};

	// Run-time counters of one ship. Owned by its CombatRuntime; keys are Object::GetUniqueID values and weak_ptr targets.
	struct EpicOffenseState
	{
		struct DirectHit
		{
			std::weak_ptr<Actor> target;
			std::uint64_t targetId = 0;
			// Index into the damage types below, -1 when the hit had none.
			int damageType = -1;
		};
		struct BloodMark
		{
			std::weak_ptr<Actor> target;
			std::uint64_t targetId = 0;
			float remainingSeconds = 0.f;
			float healFraction = 0.f;
		};

		// Offense uses since the Prism Core was placed; the cycle position follows from it.
		int prismUses = 0;
		int stormCharges = 0;
		List<DirectHit> directHits;
		List<BloodMark> bloodMarks;
		// Heap-allocated on first use: an inline engine is ~5 KB and every combatant embeds this state.
		std::unique_ptr<std::mt19937> random;
	};

	// Storm Capacitor target choice; the engine is created on first use so ships without the module never pay for it.
	inline std::mt19937& EpicOffenseRandom(EpicOffenseState& state)
	{
		if (!state.random) state.random = std::make_unique<std::mt19937>(std::random_device{}());
		return *state.random;
	}

	// Prism Core cycle position of the most recent Offense use: 0 Thermal, 1 Cryo, 2 Electric, 3 Kinetic. Before any use: 0.
	inline int EpicPrismCycleIndex(const EpicOffenseState& state) { return state.prismUses > 0 ? (state.prismUses - 1) % 4 : 0; }

	// Epic damage type indexes used by EpicOffenseState::DirectHit.
	enum class EpicDamageType : int { Thermal = 0, Cryo = 1, Electric = 2, Kinetic = 3, Energy = 4 };

	// Merges one placement's Epic Offense rules into the ship's aggregate; level selects the level-dependent values.
	void MergeEpicOffenseRules(EpicOffenseEffects& effects, const ModuleEpicOffenseRules& rules, int level);

	// Description placeholders for the Epic Offense rules of one package, keyed by rule kind and index.
	void VisitEpicOffenseDescriptionValues(const ModuleEpicOffenseRules& rules, int level, const std::function<void(const std::string&, float)>& visitor);
}
