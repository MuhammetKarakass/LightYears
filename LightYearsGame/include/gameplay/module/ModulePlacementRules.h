#pragma once

#include "content/ContentId.h"
#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/module/ModuleSideFeature.h"
#include "gameplay/module/ShipSchematic.h"

#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <functional>

namespace vega
{
	struct ModulePlacement;
	class ModuleLoadout;

	// Works with the content catalog and with hand-built test data alike.
	using ModuleLookup = std::function<const ModuleDefinition*(const sas::ContentId&)>;

	// Everything the pure rules read; the referenced objects must outlive it.
	struct ModuleRulesContext
	{
		const ShipSchematic& schematic;
		ModuleLookup lookup;
		const ModuleZoneTable& zoneTable;
		ModuleSideFeatureLookup sideFeatureLookup;
		std::function<int(const GameplayTag&)> countLoadoutAbilities;
		// Ship level; perShipLevel attribute terms grow by (shipLevel - 1) steps. Defaults to 1 for hand-built contexts.
		int shipLevel = 1;
	};

	enum class ModulePlacementFailure
	{
		None,
		OutOfBounds,
		CellBlocked,
		Overlap,
		SystemZoneMismatch,
		DuplicateSystem,
		UnknownModule,
		InvalidVariant,
		InvalidRotation,
		DuplicateModule,
		InvalidLevel,
		InvalidSideFeature,
		CellLocked,
		ModuleConflict,
		FragmentationNotAllowed,
		InvalidFragmentCells,
		FragmentedPlacement,
		InvalidElement
	};

	namespace ModulePlacementRules
	{
		// Grid cells the placement covers (rotated shape + origin, or its fragment cells when fragmented). Empty when the
		// variant is invalid.
		List<sf::Vector2i> GetCoveredCells(const ModuleDefinition& module, const ModulePlacement& placement);
		List<std::uint32_t> GetAdjacentPlacements(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& placement);
		sf::Vector2i FacingOffset(int rotationQuarterTurns);
		bool SharesCellEdge(const ModuleDefinition& leftModule, const ModulePlacement& left, const ModuleDefinition& rightModule, const ModulePlacement& right);
		List<GameplayTag> GetAdjacentAbilitySlotSystems(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& placement, std::size_t maximumCount = 2);

		// ignoredInstanceId skips one installed placement (a move over itself).
		ModulePlacementFailure CanPlace(
			const ModuleRulesContext& rules,
			const List<ModulePlacement>& installed,
			const ModulePlacement& candidate,
			std::uint32_t ignoredInstanceId = 0
		);
		ModulePlacementFailure CanPlace(
			const ModuleRulesContext& rules,
			const ModuleLoadout& loadout,
			const ModulePlacement& candidate,
			std::uint32_t ignoredInstanceId = 0
		);

		// Installed instance ids whose module conflicts with the candidate's module, in installed order. The
		// relation is symmetric and ignores geometry. ignoredInstanceId is skipped as in CanPlace.
		List<std::uint32_t> FindConflictingInstances(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& candidate, std::uint32_t ignoredInstanceId = 0);

		const char* Describe(ModulePlacementFailure failure);
	}
}
