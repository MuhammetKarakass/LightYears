#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/module/ModulePlacementRules.h"

#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace ly
{
	// One owned module. Origin and rotation are meaningful only while installed.
	struct ModulePlacement
	{
		std::uint32_t instanceId = 0;
		sas::ContentId moduleId;
		int variantIndex = 0;
		sf::Vector2i origin;
		int rotationQuarterTurns = 0;
		ModuleRarity rarity = ModuleRarity::Common;
		// Never scaled by zone bonuses.
		List<sas::AttributeModifier> rolledSideStats;
		int level = 1;
		sas::ContentId sideFeatureId;
		// Distance accrued while installed for run counter rules. Install, Uninstall and Move keep it; a new run clears it with the loadout.
		double runCounter = 0.0;
		// Kill credit for Kill run counter rules (Trophy Core), kept with the placement like runCounter. killStacks is the
		// weighted kill count (one normal kill adds 1); killDamageBonus is the accumulated permanent outgoing damage ratio.
		double killStacks = 0.0;
		double killDamageBonus = 0.0;
		// F1 Parçalama: a fragmented placement covers fragmentCells instead of origin+rotation+variant (one cell per fragment, the
		// largest variant's cell count). Only installed placements are fragmented; Uninstall clears both fields.
		bool fragmented = false;
		List<sf::Vector2i> fragmentCells;
	};

	// Installed placements plus the not-installed inventory. Every installing
	// mutation passes through ModulePlacementRules, so the loadout never holds an
	// illegal layout. Revision increments on each accepted mutation only.
	class ModuleLoadout
	{
	public:
		const List<ModulePlacement>& GetPlacements() const { return mPlacements; }
		const List<ModulePlacement>& GetInventory() const { return mInventory; }
		std::uint64_t GetRevision() const { return mRevision; }

		const ModulePlacement* FindPlacement(std::uint32_t instanceId) const;
		const ModulePlacement* FindInventoryItem(std::uint32_t instanceId) const;

		// Returns the new instance id.
		std::uint32_t AddToInventory(
			const sas::ContentId& moduleId,
			int variantIndex = 0,
			ModuleRarity rarity = ModuleRarity::Common,
			List<sas::AttributeModifier> rolledSideStats = {}
		);
		std::uint32_t AddToInventory(const ModuleRulesContext& rules, const ModuleDefinition& module, int variantIndex, ModuleRarity rarity, int level, sas::ContentId sideFeatureId = {}, List<sas::AttributeModifier> rolledSideStats = {});
		bool SetLevel(const ModuleRulesContext& rules, std::uint32_t instanceId, int level);
		bool SetSideFeature(const ModuleRulesContext& rules, std::uint32_t instanceId, const sas::ContentId& sideFeatureId);
		ModulePlacementFailure Install(
			const ModuleRulesContext& rules,
			std::uint32_t instanceId,
			sf::Vector2i origin,
			int rotationQuarterTurns
		);
		ModulePlacementFailure Move(
			const ModuleRulesContext& rules,
			std::uint32_t instanceId,
			sf::Vector2i origin,
			int rotationQuarterTurns
		);
		// F1 Parçalama: installs an inventory item as fragments, one cell each. The item must be fragmentable (see ModulePlacementRules).
		ModulePlacementFailure InstallFragmented(const ModuleRulesContext& rules, std::uint32_t instanceId, const List<sf::Vector2i>& cells);
		// Moves every fragment of an installed fragmented placement to new cells. Whole-placement Move rejects fragmented placements.
		ModulePlacementFailure MoveFragments(const ModuleRulesContext& rules, std::uint32_t instanceId, const List<sf::Vector2i>& cells);
		bool Uninstall(std::uint32_t instanceId);
		bool Remove(std::uint32_t instanceId);

			// Adds units to the run counter of every installed placement whose module has a run counter rule.
			// Returns true only when a stack count changed; that bumps the revision so the applier rebuilds.
			bool AccrueRunCounters(const ModuleRulesContext& rules, double units);
			// 0 for unknown ids and for placements without a run counter.
			double GetRunCounter(std::uint32_t instanceId) const;
			// Adds kill credit to an owned placement (installed or inventory). Kill credit does not change the layout, so the
			// revision stays put; the applier that consumes it keeps its own copy in step. False for unknown ids.
			bool AddRunCounterCredit(std::uint32_t instanceId, double units);
			bool AddKillCredit(std::uint32_t instanceId, double stacks, double damageBonus);
			// 0 for unknown ids and for placements without kill credit.
			double GetKillStacks(std::uint32_t instanceId) const;
			double GetKillDamageBonus(std::uint32_t instanceId) const;

		// Installs the schematic's default modules into an empty loadout. On any
		// failure the loadout is left empty and false is returned.
		bool SeedDefaults(const ModuleRulesContext& rules);

	private:
		ModulePlacement* FindOwnedItem(std::uint32_t instanceId);
		// Dropping an ability slot module on another one exchanges their cells; anything else stays an overlap.
		ModulePlacementFailure TrySwapAbilitySlots(const ModuleRulesContext& rules, ModulePlacement moved, sf::Vector2i destination);

		List<ModulePlacement> mPlacements;
		List<ModulePlacement> mInventory;
		std::uint64_t mRevision = 0;
		std::uint32_t mNextInstanceId = 1;
	};
}
