#pragma once

#include "content/ContentId.h"
#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/module/ShipSchematic.h"

#include <SFML/System/Vector2.hpp>

#include <cstdint>
#include <functional>

namespace ly
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
		DuplicateModule
	};

	namespace ModulePlacementRules
	{
		// Grid cells the placement covers (rotated shape + origin). Empty when the
		// variant is invalid.
		List<sf::Vector2i> GetCoveredCells(const ModuleDefinition& module, const ModulePlacement& placement);

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

		const char* Describe(ModulePlacementFailure failure);
	}
}
