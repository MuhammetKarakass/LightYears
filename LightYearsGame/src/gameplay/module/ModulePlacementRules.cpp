#include "gameplay/module/ModulePlacementRules.h"

#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

#include <set>
#include <utility>

namespace vega
{
	namespace ModulePlacementRules
	{
		namespace
		{
			bool AreConflicting(const ModuleDefinition& left, const ModuleDefinition& right)
				{
					for (const sas::ContentId& id : left.conflictsWith) if (id == right.moduleId) return true;
					for (const sas::ContentId& id : right.conflictsWith) if (id == left.moduleId) return true;
					return false;
				}

				const ModuleSizeVariant* FindVariant(const ModuleDefinition& module, int variantIndex)
			{
				if (variantIndex < 0 || static_cast<std::size_t>(variantIndex) >= module.sizeVariants.size()) return nullptr;
				return &module.sizeVariants[static_cast<std::size_t>(variantIndex)];
			}

			// F1 Parçalama: a placement may split only at the largest size and only while its side feature is the fragmentation switch
			// that is usable at its level (so Trophy Core and every module without F1 in a shared pool stay whole).
			bool CanFragment(const ModuleRulesContext& rules, const ModuleDefinition& module, const ModulePlacement& placement)
			{
				if (!IsLargestCellCountVariant(module, placement.variantIndex) || !placement.sideFeatureId.IsValid() || !rules.sideFeatureLookup) return false;
				const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup(placement.sideFeatureId);
				return IsFragmentationSideFeature(feature) && IsSideFeatureUsableAtLevel(module, feature, placement.level);
			}
		}

		List<sf::Vector2i> GetCoveredCells(const ModuleDefinition& module, const ModulePlacement& placement)
		{
			const ModuleSizeVariant* variant = FindVariant(module, placement.variantIndex);
			if (!variant) return {};
			if (placement.fragmented) return placement.fragmentCells;
			List<sf::Vector2i> cells = ModuleShapeRules::Rotate(variant->shape.cells, placement.rotationQuarterTurns);
			for (sf::Vector2i& cell : cells)
			{
				cell += placement.origin;
			}
			return cells;
		}

		sf::Vector2i FacingOffset(int rotationQuarterTurns)
		{
			const sf::Vector2i directions[4] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
			return directions[(rotationQuarterTurns % 4 + 4) % 4];
		}

		List<std::uint32_t> GetAdjacentPlacements(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& placement)
		{
			List<std::uint32_t> result;
			const auto* owner = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!owner) return result;
			for (const auto& other : installed)
			{
				const auto* module = rules.lookup(other.moduleId);
				if (other.instanceId != placement.instanceId && module && SharesCellEdge(*owner, placement, *module, other)) result.push_back(other.instanceId);
			}
			return result;
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
				if (!module || !GameplayTagSchema::IsModuleAbilitySlotSystem(ModuleSystemForPlacement(rules, *module, candidate)) ||
					!SharesCellEdge(*owner, placement, *module, candidate)) continue;
				const auto system = ModuleSystemForPlacement(rules, *module, candidate);
				if (std::find(result.begin(), result.end(), system) == result.end()) result.push_back(system);
			}
			std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
			if (result.size() > maximumCount) result.resize(maximumCount);
			return result;
		}

		ModulePlacementFailure CanPlace(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& candidate, std::uint32_t ignoredInstanceId)
		{
			const ModuleDefinition* module = rules.lookup ? rules.lookup(candidate.moduleId) : nullptr;
			if (!module) return ModulePlacementFailure::UnknownModule;
			if (!FindVariant(*module, candidate.variantIndex)) return ModulePlacementFailure::InvalidVariant;
			if ((module->abilityModule || module->unique) && (candidate.sideFeatureId.IsValid() || !candidate.rolledSideStats.empty())) return ModulePlacementFailure::InvalidSideFeature;
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
			if (candidate.fragmented && !CanFragment(rules, *module, candidate)) return ModulePlacementFailure::FragmentationNotAllowed;

			const List<sf::Vector2i> cells = GetCoveredCells(*module, candidate);
			const GameplayTag* systemZone = nullptr;
			const auto system = ModuleSystemForPlacement(rules, *module, candidate);
			if (module->abilityModule && !system.IsValid()) return ModulePlacementFailure::SystemZoneMismatch;
			if (system.IsValid())
			{
				systemZone = rules.zoneTable.FindSystemZone(system);
				if (!systemZone) return ModulePlacementFailure::SystemZoneMismatch;
			}
			const bool isAbilitySlot = GameplayTagSchema::IsModuleAbilitySlotSystem(system);
			for (const sf::Vector2i& cell : cells)
			{
				if (!rules.schematic.IsInBounds(cell.x, cell.y)) return ModulePlacementFailure::OutOfBounds;
				const GameplayTag& zone = rules.schematic.GetZoneAt(cell.x, cell.y);
				if (!zone.IsValid()) return ModulePlacementFailure::CellBlocked;
				if (rules.schematic.IsLockedAt(cell.x, cell.y)) return ModulePlacementFailure::CellLocked;
				if (systemZone && zone != *systemZone) return ModulePlacementFailure::SystemZoneMismatch;
				// The ability bay is reserved for the Q/E/F/R slot modules.
				if (!isAbilitySlot && zone == GameplayTags::Module::Zone::AbilityBay) return ModulePlacementFailure::SystemZoneMismatch;
			}

			std::set<std::pair<int, int>> taken;
			for (const sf::Vector2i& cell : cells)
			{
				taken.insert({ cell.x, cell.y });
			}
			// A fragmented placement needs exactly one distinct cell per fragment, one fragment for each cell of its largest variant.
			if (candidate.fragmented && (taken.size() != cells.size() || cells.size() != FindVariant(*module, candidate.variantIndex)->shape.cells.size()))
				return ModulePlacementFailure::InvalidFragmentCells;
			bool duplicateSystem = false;
			for (const ModulePlacement& other : installed)
			{
				if (ignoredInstanceId != 0 && other.instanceId == ignoredInstanceId) continue;
				const ModuleDefinition* otherModule = rules.lookup(other.moduleId);
				if (!otherModule) continue;
				if (!module->abilityModule && other.moduleId == candidate.moduleId) return ModulePlacementFailure::DuplicateModule;
				if (AreConflicting(*module, *otherModule)) return ModulePlacementFailure::ModuleConflict;
				for (const sf::Vector2i& cell : GetCoveredCells(*otherModule, other))
				{
					if (taken.count({ cell.x, cell.y })) return ModulePlacementFailure::Overlap;
				}
				if (system.IsValid() && ModuleSystemForPlacement(rules, *otherModule, other) == system)
				{
					duplicateSystem = true;
				}
			}
			return duplicateSystem ? ModulePlacementFailure::DuplicateSystem : ModulePlacementFailure::None;
		}

		ModulePlacementFailure CanPlace(const ModuleRulesContext& rules, const ModuleLoadout& loadout, const ModulePlacement& candidate, std::uint32_t ignoredInstanceId)
		{
			return CanPlace(rules, loadout.GetPlacements(), candidate, ignoredInstanceId);
		}

		List<std::uint32_t> FindConflictingInstances(const ModuleRulesContext& rules, const List<ModulePlacement>& installed, const ModulePlacement& candidate, std::uint32_t ignoredInstanceId)
		{
			List<std::uint32_t> conflicts;
			const ModuleDefinition* module = rules.lookup ? rules.lookup(candidate.moduleId) : nullptr;
			if (!module) return conflicts;
			for (const ModulePlacement& other : installed)
			{
				if (ignoredInstanceId != 0 && other.instanceId == ignoredInstanceId) continue;
				const ModuleDefinition* otherModule = rules.lookup ? rules.lookup(other.moduleId) : nullptr;
				if (otherModule && AreConflicting(*module, *otherModule)) conflicts.push_back(other.instanceId);
			}
			return conflicts;
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
			case ModulePlacementFailure::CellLocked: return "covers a locked area";
			case ModulePlacementFailure::ModuleConflict: return "conflicts with an installed module";
			case ModulePlacementFailure::FragmentationNotAllowed: return "cannot be split into fragments";
			case ModulePlacementFailure::InvalidFragmentCells: return "has an invalid fragment layout";
			case ModulePlacementFailure::FragmentedPlacement: return "is split into fragments; move its fragments instead";
			}
			return "is invalid";
		}
	}
}
