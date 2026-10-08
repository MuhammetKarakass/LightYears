#include "gameplay/module/ModulePlacementRules.h"

#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/tags/GameplayTagSchema.h"

#include <set>
#include <utility>

namespace ly
{
	namespace ModulePlacementRules
	{
		namespace
		{
			const ModuleSizeVariant* FindVariant(const ModuleDefinition& module, int variantIndex)
			{
				if (variantIndex < 0 || static_cast<std::size_t>(variantIndex) >= module.sizeVariants.size()) return nullptr;
				return &module.sizeVariants[static_cast<std::size_t>(variantIndex)];
			}
		}

		List<sf::Vector2i> GetCoveredCells(const ModuleDefinition& module, const ModulePlacement& placement)
		{
			const ModuleSizeVariant* variant = FindVariant(module, placement.variantIndex);
			if (!variant) return {};
			List<sf::Vector2i> cells = ModuleShapeRules::Rotate(variant->shape.cells, placement.rotationQuarterTurns);
			for (sf::Vector2i& cell : cells)
			{
				cell += placement.origin;
			}
			return cells;
		}

		bool SharesCellEdge(const ModuleDefinition& leftModule, const ModulePlacement& left, const ModuleDefinition& rightModule, const ModulePlacement& right)
		{
			for (const auto& a : GetCoveredCells(leftModule, left))
				for (const auto& b : GetCoveredCells(rightModule, right))
					if (std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1) return true;
			return false;
		}

		List<GameplayTag> GetAdjacentAbilitySlotSystems(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& placement, std::size_t maximumCount)
		{
			List<GameplayTag> result;
			const auto* owner = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!owner || maximumCount == 0) return result;
			for (const auto& candidate : installed)
			{
				if (candidate.instanceId == placement.instanceId) continue;
				const auto* module = rules.lookup(candidate.moduleId);
				if (!module || !GameplayTagSchema::IsModuleAbilitySlotSystem(module->systemCategory) ||
					!SharesCellEdge(*owner, placement, *module, candidate)) continue;
				if (std::find(result.begin(), result.end(), module->systemCategory) == result.end()) result.push_back(module->systemCategory);
			}
			std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
			if (result.size() > maximumCount) result.resize(maximumCount);
			return result;
		}

		ModulePlacementFailure CanPlace(
			const ModuleRulesContext& rules,
			const List<ModulePlacement>& installed,
			const ModulePlacement& candidate,
			std::uint32_t ignoredInstanceId)
		{
			const ModuleDefinition* module = rules.lookup ? rules.lookup(candidate.moduleId) : nullptr;
			if (!module) return ModulePlacementFailure::UnknownModule;
			if (!FindVariant(*module, candidate.variantIndex)) return ModulePlacementFailure::InvalidVariant;
			if (candidate.level < 1 || candidate.level > module->levelCount ||
				!HasModuleRulePackageLevelCoverage(module->sizeVariants[static_cast<std::size_t>(candidate.variantIndex)], candidate.level))
				return ModulePlacementFailure::InvalidLevel;
			if (candidate.sideFeatureId.IsValid())
			{
				const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup ? rules.sideFeatureLookup(candidate.sideFeatureId) : nullptr;
				if (!IsSideFeatureUsableAtLevel(*module, feature, candidate.level))
				return ModulePlacementFailure::InvalidSideFeature;
			}
			if (candidate.rotationQuarterTurns < 0 || candidate.rotationQuarterTurns > 3) return ModulePlacementFailure::InvalidRotation;

			const List<sf::Vector2i> cells = GetCoveredCells(*module, candidate);
			const GameplayTag* systemZone = nullptr;
			if (module->systemCategory.IsValid() &&
				!GameplayTagSchema::IsModuleAbilitySlotSystem(module->systemCategory))
			{
				systemZone = rules.zoneTable.FindSystemZone(module->systemCategory);
				if (!systemZone) return ModulePlacementFailure::SystemZoneMismatch;
			}
			for (const sf::Vector2i& cell : cells)
			{
				if (!rules.schematic.IsInBounds(cell.x, cell.y)) return ModulePlacementFailure::OutOfBounds;
				const GameplayTag& zone = rules.schematic.GetZoneAt(cell.x, cell.y);
				if (!zone.IsValid()) return ModulePlacementFailure::CellBlocked;
				if (systemZone && zone != *systemZone) return ModulePlacementFailure::SystemZoneMismatch;
			}

			std::set<std::pair<int, int>> taken;
			for (const sf::Vector2i& cell : cells)
			{
				taken.insert({ cell.x, cell.y });
			}
			bool duplicateSystem = false;
			for (const ModulePlacement& other : installed)
			{
				if (ignoredInstanceId != 0 && other.instanceId == ignoredInstanceId) continue;
				const ModuleDefinition* otherModule = rules.lookup(other.moduleId);
				if (!otherModule) continue;
				if (other.moduleId == candidate.moduleId) return ModulePlacementFailure::DuplicateModule;
				for (const sf::Vector2i& cell : GetCoveredCells(*otherModule, other))
				{
					if (taken.count({ cell.x, cell.y })) return ModulePlacementFailure::Overlap;
				}
				if (module->systemCategory.IsValid() && otherModule->systemCategory == module->systemCategory)
				{
					duplicateSystem = true;
				}
			}
			return duplicateSystem ? ModulePlacementFailure::DuplicateSystem : ModulePlacementFailure::None;
		}

		ModulePlacementFailure CanPlace(
			const ModuleRulesContext& rules,
			const ModuleLoadout& loadout,
			const ModulePlacement& candidate,
			std::uint32_t ignoredInstanceId)
		{
			return CanPlace(rules, loadout.GetPlacements(), candidate, ignoredInstanceId);
		}

		const char* Describe(ModulePlacementFailure failure)
		{
			switch (failure)
			{
			case ModulePlacementFailure::None: return "none";
			case ModulePlacementFailure::OutOfBounds: return "leaves the schematic";
			case ModulePlacementFailure::CellBlocked: return "covers a blocked cell";
			case ModulePlacementFailure::Overlap: return "overlaps another module";
			case ModulePlacementFailure::SystemZoneMismatch: return "is outside its system zone";
			case ModulePlacementFailure::DuplicateModule: return "duplicates an installed module";
			case ModulePlacementFailure::DuplicateSystem: return "duplicates an installed system";
			case ModulePlacementFailure::UnknownModule: return "is not in the module catalog";
			case ModulePlacementFailure::InvalidVariant: return "has an invalid variant";
			case ModulePlacementFailure::InvalidRotation: return "has a rotation outside 0..3";
			case ModulePlacementFailure::InvalidLevel: return "has an invalid level";
			case ModulePlacementFailure::InvalidSideFeature: return "has an invalid side feature";
			}
			return "is invalid";
		}
	}
}
