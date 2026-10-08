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
		bool Uninstall(std::uint32_t instanceId);
		bool Remove(std::uint32_t instanceId);

		// Installs the schematic's default modules into an empty loadout. On any
		// failure the loadout is left empty and false is returned.
		bool SeedDefaults(const ModuleRulesContext& rules);

	private:
		ModulePlacement* FindOwnedItem(std::uint32_t instanceId);

		List<ModulePlacement> mPlacements;
		List<ModulePlacement> mInventory;
		std::uint64_t mRevision = 0;
		std::uint32_t mNextInstanceId = 1;
	};
}
