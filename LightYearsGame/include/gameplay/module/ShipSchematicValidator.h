#pragma once

#include "gameplay/module/ModuleDefinition.h"
#include "gameplay/module/ShipSchematic.h"

#include <string>

namespace ly
{
	// Cross-catalog rules for a ship schematic: they need the module catalog, so
	// this runs in the content bootstrap after modules and ships are both loaded.
	// Grid syntax (legend, rectangular rows, rotation range) is checked earlier by
	// ShipLoader.
	class ShipSchematicValidator final
	{
	public:
		static bool Validate(
			const ShipSchematic& schematic,
			const List<ModuleDefinition>& modules,
			const ModuleZoneTable& zoneTable,
			std::string* failureReason = nullptr
		);
	};
}
