#include "gameplay/module/ShipSchematicValidator.h"

#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cstdint>
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

		for (const GameplayTag& zone : schematic.cellZones)
		{
			if (!zone.IsValid()) continue;
			if (!zoneTable.FindZone(zone))
			{
				return Fail(failureReason, "Schematic uses a zone missing from the zone table: " + zone.name);
			}
		}
		const ModuleRulesContext rules{
			schematic,
			[&modules](const sas::ContentId& moduleId) { return FindModule(modules, moduleId); },
			zoneTable
		};
		List<ModulePlacement> installed;
		List<GameplayTag> defaultSystems;
		std::set<std::pair<int, int>> occupied;
		for (const DefaultModulePlacement& defaults : schematic.defaultModules)
		{
			const std::string name = defaults.moduleId.ToString();
			const ModuleDefinition* module = FindModule(modules, defaults.moduleId);
			if (module && !module->systemCategory.IsValid())
			{
				return Fail(failureReason, "Default module must be a system module: " + name);
			}
			ModulePlacement placement;
			placement.instanceId = static_cast<std::uint32_t>(installed.size() + 1);
			placement.moduleId = defaults.moduleId;
			placement.variantIndex = defaults.variantIndex;
			placement.origin = defaults.origin;
			placement.rotationQuarterTurns = defaults.rotationQuarterTurns;
			const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(rules, installed, placement);
			if (failure != ModulePlacementFailure::None)
			{
				return Fail(failureReason, "Default module " + name + " " + ModulePlacementRules::Describe(failure));
			}
			defaultSystems.push_back(module->systemCategory);
			for (const sf::Vector2i& cell : ModulePlacementRules::GetCoveredCells(*module, placement))
			{
				occupied.insert({ cell.x, cell.y });
			}
			installed.push_back(std::move(placement));
		}

		for (const GameplayTag* required : GameplayTagSchema::GetRequiredDefaultModuleSystems())
		{
			if (std::find(defaultSystems.begin(), defaultSystems.end(), *required) == defaultSystems.end())
			{
				return Fail(failureReason, "Default modules are missing " + required->name);
			}
		}

		return true;
	}
}
