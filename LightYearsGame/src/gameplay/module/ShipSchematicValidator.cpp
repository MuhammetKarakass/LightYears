#include "gameplay/module/ShipSchematicValidator.h"

#include <set>
#include <utility>

namespace ly
{
	namespace
	{
		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}

		const ModuleDefinition* FindModule(const List<ModuleDefinition>& modules, const sas::ContentId& moduleId)
		{
			for (const ModuleDefinition& module : modules)
			{
				if (module.moduleId == moduleId) return &module;
			}
			return nullptr;
		}
	}

	bool ShipSchematicValidator::Validate(
		const ShipSchematic& schematic,
		const List<ModuleDefinition>& modules,
		const ModuleZoneTable& zoneTable,
		std::string* failureReason)
	{
		if (schematic.width <= 0 || schematic.height <= 0 ||
			schematic.cellZones.size() != static_cast<std::size_t>(schematic.width * schematic.height))
		{
			return Fail(failureReason, "Schematic grid dimensions do not match its cells");
		}

		int totalCells = 0;
		for (const GameplayTag& zone : schematic.cellZones)
		{
			if (!zone.IsValid()) continue;
			if (!zoneTable.FindZone(zone))
			{
				return Fail(failureReason, "Schematic uses a zone missing from the zone table: " + zone.name);
			}
			++totalCells;
		}

		std::set<std::pair<int, int>> occupied;
		std::set<std::string> systems;
		for (const DefaultModulePlacement& placement : schematic.defaultModules)
		{
			const std::string name = placement.moduleId.ToString();
			const ModuleDefinition* module = FindModule(modules, placement.moduleId);
			if (!module)
			{
				return Fail(failureReason, "Default module is not in the module catalog: " + name);
			}
			if (!module->systemCategory.IsValid())
			{
				return Fail(failureReason, "Default module must be a system module: " + name);
			}
			if (!systems.insert(module->systemCategory.name).second)
			{
				return Fail(failureReason, "Schematic has more than one default module for " + module->systemCategory.name);
			}
			if (placement.variantIndex < 0 ||
				static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size())
			{
				return Fail(failureReason, "Default module has an invalid variant: " + name);
			}
			if (placement.rotationQuarterTurns < 0 || placement.rotationQuarterTurns > 3)
			{
				return Fail(failureReason, "Default module rotation must be 0..3: " + name);
			}
			const GameplayTag* systemZone = zoneTable.FindSystemZone(module->systemCategory);
			if (!systemZone)
			{
				return Fail(failureReason, "System module has no system zone: " + name);
			}
			const List<sf::Vector2i> cells = ModuleShapeRules::Rotate(
				module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)].shape.cells,
				placement.rotationQuarterTurns
			);
			for (const sf::Vector2i& offset : cells)
			{
				const int x = placement.origin.x + offset.x;
				const int y = placement.origin.y + offset.y;
				if (!schematic.IsInBounds(x, y) || !schematic.GetZoneAt(x, y).IsValid())
				{
					return Fail(failureReason, "Default module leaves the schematic: " + name);
				}
				if (schematic.GetZoneAt(x, y) != *systemZone)
				{
					return Fail(failureReason, "Default module is outside its system zone: " + name);
				}
				if (!occupied.insert({ x, y }).second)
				{
					return Fail(failureReason, "Default modules overlap: " + name);
				}
			}
		}

		const int freeCells = totalCells - static_cast<int>(occupied.size());
		if (freeCells != kFreeCellCapacity)
		{
			return Fail(
				failureReason,
				"Schematic must have " + std::to_string(kFreeCellCapacity) +
				" free cells excluding default modules, found " + std::to_string(freeCells)
			);
		}
		return true;
	}
}
