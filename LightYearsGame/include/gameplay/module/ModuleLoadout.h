#pragma once

#include "attributes/AttributeSystem.h"
#include "content/ContentId.h"
#include "framework/Core.h"
#include "gameplay/module/ModulePlacementRules.h"

#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace ly
{
	enum class ModuleRarity
	{
		Common,
		Rare,
		Epic,
		Legendary
	};

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
		List<ModulePlacement> mPlacements;
		List<ModulePlacement> mInventory;
		std::uint64_t mRevision = 0;
		std::uint32_t mNextInstanceId = 1;
	};
}
