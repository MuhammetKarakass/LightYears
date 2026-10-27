#include "gameplay/module/ModuleLoadout.h"

#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/content/ModuleContentCatalog.h"

#include <algorithm>
#include <utility>

namespace vega
{
	namespace
	{
		List<ModulePlacement>::const_iterator FindById(const List<ModulePlacement>& list, std::uint32_t instanceId)
		{
			return std::find_if(list.begin(), list.end(),
				[instanceId](const ModulePlacement& placement) { return placement.instanceId == instanceId; });
		}
	}

	const ModulePlacement* ModuleLoadout::FindPlacement(std::uint32_t instanceId) const
	{
		const auto found = FindById(mPlacements, instanceId);
		return found == mPlacements.end() ? nullptr : &*found;
	}

	const ModulePlacement* ModuleLoadout::FindInventoryItem(std::uint32_t instanceId) const
	{
		const auto found = FindById(mInventory, instanceId);
		return found == mInventory.end() ? nullptr : &*found;
	}

	std::uint32_t ModuleLoadout::AddToInventory(const sas::ContentId& moduleId, int variantIndex, ModuleRarity rarity, List<sas::AttributeModifier> rolledSideStats)
	{
		const auto* module = content::ModuleContentCatalog::FindById(moduleId);
		if (module && (module->abilityModule || module->unique) && !rolledSideStats.empty()) return 0;
		if (module && module->abilityModule && module->abilityModule->elementCore && module->abilityModule->elementCore->elements.empty()) return 0;
		ModulePlacement item;
		item.instanceId = mNextInstanceId++;
		item.moduleId = moduleId;
		if (module && module->abilityModule && module->abilityModule->elementCore)
		{
			const auto& elements = module->abilityModule->elementCore->elements;
			item.element = elements[std::uniform_int_distribution<std::size_t>{ 0, elements.size() - 1 }(mRandom)];
		}
		item.variantIndex = variantIndex;
		item.rarity = module && (module->abilityModule || module->unique) ? ModuleRarity::None : rarity;
		item.rolledSideStats = std::move(rolledSideStats);
		mInventory.push_back(std::move(item));
		++mRevision;
		return mInventory.back().instanceId;
	}

	std::uint32_t ModuleLoadout::AddToInventory(const ModuleRulesContext& rules, const ModuleDefinition& module, int variantIndex, ModuleRarity rarity, int level, sas::ContentId sideFeatureId, List<sas::AttributeModifier> rolledSideStats)
	{
		const ModuleDefinition* canonical = rules.lookup ? rules.lookup(module.moduleId) : nullptr;
		if (!canonical || variantIndex < 0 || static_cast<std::size_t>(variantIndex) >= canonical->sizeVariants.size() ||
			level < 1 || level > canonical->levelCount ||
			!HasModuleRulePackageLevelCoverage(canonical->sizeVariants[static_cast<std::size_t>(variantIndex)], level)) return 0;
		if ((canonical->abilityModule || canonical->unique) && (!rolledSideStats.empty() || sideFeatureId.IsValid())) return 0;
		if (canonical->abilityModule && canonical->abilityModule->elementCore && canonical->abilityModule->elementCore->elements.empty()) return 0;
		if (sideFeatureId.IsValid())
		{
			const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup ? rules.sideFeatureLookup(sideFeatureId) : nullptr;
			if (!IsSideFeatureUsableAtLevel(*canonical, feature, level)) return 0;
		}

		ModulePlacement item;
		item.instanceId = mNextInstanceId++;
		item.moduleId = canonical->moduleId;
		if (canonical->abilityModule && canonical->abilityModule->elementCore)
		{
			const auto& elements = canonical->abilityModule->elementCore->elements;
			item.element = elements[std::uniform_int_distribution<std::size_t>{ 0, elements.size() - 1 }(mRandom)];
		}
		item.variantIndex = variantIndex;
		item.rarity = (canonical->abilityModule || canonical->unique) ? ModuleRarity::None : rarity;
		item.rolledSideStats = std::move(rolledSideStats);
		item.level = level;
		item.sideFeatureId = std::move(sideFeatureId);
		mInventory.push_back(std::move(item));
		++mRevision;
		return mInventory.back().instanceId;
	}

	ModulePlacement* ModuleLoadout::FindOwnedItem(std::uint32_t instanceId)
	{
		for (ModulePlacement& placement : mPlacements) if (placement.instanceId == instanceId) return &placement;
		for (ModulePlacement& placement : mInventory) if (placement.instanceId == instanceId) return &placement;
		return nullptr;
	}

	bool ModuleLoadout::SetInventoryElement(const ModuleRulesContext& rules, std::uint32_t instanceId, const GameplayTag& element)
	{
		if (!FindInventoryItem(instanceId)) return false;
		auto* item = FindOwnedItem(instanceId);
		const auto* definition = rules.lookup ? rules.lookup(item->moduleId) : nullptr;
		if (!definition || !definition->abilityModule || !definition->abilityModule->elementCore) return false;
		const auto& elements = definition->abilityModule->elementCore->elements;
		if (std::find(elements.begin(), elements.end(), element) == elements.end()) return false;
		if (item->element != element) { item->element = element; ++mRevision; }
		return true;
	}

	bool ModuleLoadout::SetSecondaryWeapon(std::uint32_t instanceId, const std::string& weaponId)
	{
		ModulePlacement* item = FindOwnedItem(instanceId);
		if (!item || weaponId.empty()) return false;
		if (item->secondaryWeaponId == weaponId) return true;
		item->secondaryWeaponId = weaponId;
		++mRevision;
		return true;
	}

	bool ModuleLoadout::SetLevel(const ModuleRulesContext& rules, std::uint32_t instanceId, int level)
	{
		ModulePlacement* item = FindOwnedItem(instanceId);
		if (!item) return false;
		const ModuleDefinition* module = rules.lookup ? rules.lookup(item->moduleId) : nullptr;
		if (!module || item->variantIndex < 0 || static_cast<std::size_t>(item->variantIndex) >= module->sizeVariants.size() ||
		item->level < 1 || item->level > module->levelCount || level < 1 || level > module->levelCount ||
			!HasModuleRulePackageLevelCoverage(module->sizeVariants[static_cast<std::size_t>(item->variantIndex)], item->level) ||
			!HasModuleRulePackageLevelCoverage(module->sizeVariants[static_cast<std::size_t>(item->variantIndex)], level)) return false;
		if (item->sideFeatureId.IsValid())
		{
			const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup ? rules.sideFeatureLookup(item->sideFeatureId) : nullptr;
			if (!IsSideFeatureUsableAtLevel(*module, feature, level)) return false;
		}
		if (item->level == level) return true;
		item->level = level;
		++mRevision;
		return true;
	}

	bool ModuleLoadout::SetSideFeature(const ModuleRulesContext& rules, std::uint32_t instanceId, const sas::ContentId& sideFeatureId)
	{
		ModulePlacement* item = FindOwnedItem(instanceId);
		if (!item) return false;
		const ModuleDefinition* module = rules.lookup ? rules.lookup(item->moduleId) : nullptr;
		if (!module || item->variantIndex < 0 || static_cast<std::size_t>(item->variantIndex) >= module->sizeVariants.size() ||
		item->level < 1 || item->level > module->levelCount ||
		!HasModuleRulePackageLevelCoverage(module->sizeVariants[static_cast<std::size_t>(item->variantIndex)], item->level)) return false;
		if ((module->abilityModule || module->unique) && sideFeatureId.IsValid()) return false;
		if (sideFeatureId.IsValid())
		{
			const ModuleSideFeatureDefinition* feature = rules.sideFeatureLookup ? rules.sideFeatureLookup(sideFeatureId) : nullptr;
			if (!IsSideFeatureUsableAtLevel(*module, feature, item->level)) return false;
		}
		if (item->fragmented && item->sideFeatureId != sideFeatureId) return false;
		if (item->sideFeatureId == sideFeatureId) return true;
		item->sideFeatureId = sideFeatureId;
		++mRevision;
		return true;
	}

	ModulePlacementFailure ModuleLoadout::Install(const ModuleRulesContext& rules, std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns)
	{
		const auto found = FindById(mInventory, instanceId);
		if (found == mInventory.end()) return ModulePlacementFailure::UnknownModule;

		ModulePlacement candidate = *found;
		candidate.origin = origin;
		candidate.rotationQuarterTurns = rotationQuarterTurns;
		const auto* module = rules.lookup ? rules.lookup(candidate.moduleId) : nullptr;
		if (module && module->abilityModule) candidate.abilitySlot = AbilityModuleSlotAtCell(rules, origin);
		const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(rules, mPlacements, candidate);
		if (failure != ModulePlacementFailure::None) return failure;

		mInventory.erase(mInventory.begin() + (found - mInventory.cbegin()));
		mPlacements.push_back(std::move(candidate));
		++mRevision;
		return ModulePlacementFailure::None;
	}

	ModulePlacementFailure ModuleLoadout::Move(
		const ModuleRulesContext& rules,
		std::uint32_t instanceId,
		sf::Vector2i origin,
		int rotationQuarterTurns)
	{
		const auto found = FindById(mPlacements, instanceId);
		if (found == mPlacements.end()) return ModulePlacementFailure::UnknownModule;
		if (found->fragmented) return ModulePlacementFailure::FragmentedPlacement;

		ModulePlacement candidate = *found;
		candidate.origin = origin;
		candidate.rotationQuarterTurns = rotationQuarterTurns;
		const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(rules, mPlacements, candidate, instanceId);
		if (failure == ModulePlacementFailure::Overlap) return TrySwapAbilitySlots(rules, *found, origin);
		if (failure != ModulePlacementFailure::None) return failure;

		mPlacements[static_cast<std::size_t>(found - mPlacements.cbegin())] = std::move(candidate);
		++mRevision;
		return ModulePlacementFailure::None;
	}

	ModulePlacementFailure ModuleLoadout::InstallFragmented(const ModuleRulesContext& rules, std::uint32_t instanceId, const List<sf::Vector2i>& cells)
	{
		const auto found = FindById(mInventory, instanceId);
		if (found == mInventory.end()) return ModulePlacementFailure::UnknownModule;

		ModulePlacement candidate = *found;
		candidate.origin = sf::Vector2i{};
		candidate.rotationQuarterTurns = 0;
		candidate.fragmented = true;
		candidate.fragmentCells = cells;
		const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(rules, mPlacements, candidate);
		if (failure != ModulePlacementFailure::None) return failure;

		mInventory.erase(mInventory.begin() + (found - mInventory.cbegin()));
		mPlacements.push_back(std::move(candidate));
		++mRevision;
		return ModulePlacementFailure::None;
	}

	ModulePlacementFailure ModuleLoadout::MoveFragments(const ModuleRulesContext& rules, std::uint32_t instanceId, const List<sf::Vector2i>& cells)
	{
		const auto found = FindById(mPlacements, instanceId);
		if (found == mPlacements.end()) return ModulePlacementFailure::UnknownModule;
		if (!found->fragmented) return ModulePlacementFailure::FragmentationNotAllowed;

		ModulePlacement candidate = *found;
		candidate.fragmentCells = cells;
		const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(rules, mPlacements, candidate, instanceId);
		if (failure != ModulePlacementFailure::None) return failure;

		mPlacements[static_cast<std::size_t>(found - mPlacements.cbegin())] = std::move(candidate);
		++mRevision;
		return ModulePlacementFailure::None;
	}

	ModulePlacementFailure ModuleLoadout::TrySwapAbilitySlots(const ModuleRulesContext& rules, ModulePlacement moved, sf::Vector2i destination)
	{
		const ModuleDefinition* movedModule = rules.lookup ? rules.lookup(moved.moduleId) : nullptr;
		if (!movedModule || moved.fragmented || !GameplayTagSchema::IsModuleAbilitySlotSystem(ModuleSystemForPlacement(rules, *movedModule, moved))) return ModulePlacementFailure::Overlap;
		for (ModulePlacement& other : mPlacements)
		{
			if (other.instanceId == moved.instanceId || other.fragmented) continue;
			const ModuleDefinition* otherModule = rules.lookup(other.moduleId);
			if (!otherModule || !GameplayTagSchema::IsModuleAbilitySlotSystem(ModuleSystemForPlacement(rules, *otherModule, other))) continue;
			const List<sf::Vector2i> covered = ModulePlacementRules::GetCoveredCells(*otherModule, other);
			if (std::find(covered.begin(), covered.end(), destination) == covered.end()) continue;
			const sf::Vector2i otherOrigin = other.origin;
			other.origin = moved.origin;
			for (ModulePlacement& mine : mPlacements)
			{
				if (mine.instanceId == moved.instanceId) mine.origin = otherOrigin;
			}
			++mRevision;
			return ModulePlacementFailure::None;
		}
		return ModulePlacementFailure::Overlap;
	}

	bool ModuleLoadout::Uninstall(std::uint32_t instanceId)
	{
		const auto found = FindById(mPlacements, instanceId);
		if (found == mPlacements.end()) return false;

		ModulePlacement item = *found;
		item.origin = sf::Vector2i{};
		item.fragmented = false;
		item.fragmentCells.clear();
		item.rotationQuarterTurns = 0;
		mPlacements.erase(mPlacements.begin() + (found - mPlacements.cbegin()));
		mInventory.push_back(std::move(item));
		++mRevision;
		return true;
	}

	bool ModuleLoadout::Remove(std::uint32_t instanceId)
	{
		const auto placement = FindById(mPlacements, instanceId);
		if (placement != mPlacements.end())
		{
			mPlacements.erase(mPlacements.begin() + (placement - mPlacements.cbegin()));
			++mRevision;
			return true;
		}
		const auto item = FindById(mInventory, instanceId);
		if (item != mInventory.end())
		{
			mInventory.erase(mInventory.begin() + (item - mInventory.cbegin()));
			++mRevision;
			return true;
		}
		return false;
	}

	bool ModuleLoadout::AccrueRunCounters(const ModuleRulesContext& rules, double units)
	{
		if (!(units > 0.0)) return false;
		bool stacksChanged = false;
		for (ModulePlacement& placement : mPlacements)
		{
			const ModuleDefinition* module = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!module || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= module->sizeVariants.size()) continue;
			const List<ModuleRunCounterRule>& counters = module->sizeVariants[static_cast<std::size_t>(placement.variantIndex)].runCounterRules;
			const bool hasDistanceRule = std::any_of(counters.begin(), counters.end(), [](const ModuleRunCounterRule& rule) { return rule.source == ModuleRunCounterSource::Distance; });
			if (!hasDistanceRule) continue;
			for (const ModuleRunCounterRule& rule : counters)
			{
				if (rule.source != ModuleRunCounterSource::Distance) continue;
				if (GetModuleRunCounterStacks(rule, placement.runCounter) != GetModuleRunCounterStacks(rule, placement.runCounter + units)) stacksChanged = true;
			}
			placement.runCounter += units;
		}
		if (stacksChanged) ++mRevision;
		return stacksChanged;
	}

	double ModuleLoadout::GetRunCounter(std::uint32_t instanceId) const
	{
		if (const ModulePlacement* placement = FindPlacement(instanceId)) return placement->runCounter;
		if (const ModulePlacement* item = FindInventoryItem(instanceId)) return item->runCounter;
		return 0.0;
	}

	bool ModuleLoadout::AddRunCounterCredit(std::uint32_t instanceId, double units)
	{
		auto* placement = FindOwnedItem(instanceId);
		if (!placement || !(units > 0.0) || !std::isfinite(units)) return false;
		placement->runCounter += units; ++mRevision; return true;
	}
	bool ModuleLoadout::AddKillCredit(std::uint32_t instanceId, double stacks, double damageBonus)
	{
		ModulePlacement* placement = FindOwnedItem(instanceId);
		if (!placement || !(stacks >= 0.0) || !(damageBonus >= 0.0)) return false;
		placement->killStacks += stacks;
		placement->killDamageBonus += damageBonus;
		return true;
	}

	bool ModuleLoadout::AddReviveChargeSpent(std::uint32_t instanceId)
	{
		ModulePlacement* placement = FindOwnedItem(instanceId);
		if (!placement) return false;
		++placement->reviveChargesSpent;
		return true;
	}

	double ModuleLoadout::GetKillStacks(std::uint32_t instanceId) const
	{
		if (const ModulePlacement* placement = FindPlacement(instanceId)) return placement->killStacks;
		if (const ModulePlacement* item = FindInventoryItem(instanceId)) return item->killStacks;
		return 0.0;
	}

	double ModuleLoadout::GetKillDamageBonus(std::uint32_t instanceId) const
	{
		if (const ModulePlacement* placement = FindPlacement(instanceId)) return placement->killDamageBonus;
		if (const ModulePlacement* item = FindInventoryItem(instanceId)) return item->killDamageBonus;
		return 0.0;
	}

	bool ModuleLoadout::SeedDefaults(const ModuleRulesContext& rules)
	{
		for (const DefaultModulePlacement& defaults : rules.schematic.defaultModules)
		{
			const std::uint32_t id = AddToInventory(defaults.moduleId, defaults.variantIndex);
			if (Install(rules, id, defaults.origin, defaults.rotationQuarterTurns) != ModulePlacementFailure::None)
			{
				mPlacements.clear();
				mInventory.clear();
				++mRevision;
				return false;
			}
		}
		return true;
	}
}
