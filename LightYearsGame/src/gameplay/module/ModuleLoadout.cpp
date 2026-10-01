#include "gameplay/module/ModuleLoadout.h"

#include <algorithm>
#include <utility>

namespace ly
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

	std::uint32_t ModuleLoadout::AddToInventory(
		const sas::ContentId& moduleId,
		int variantIndex,
		ModuleRarity rarity,
		List<sas::AttributeModifier> rolledSideStats)
	{
		ModulePlacement item;
		item.instanceId = mNextInstanceId++;
		item.moduleId = moduleId;
		item.variantIndex = variantIndex;
		item.rarity = rarity;
		item.rolledSideStats = std::move(rolledSideStats);
		mInventory.push_back(std::move(item));
		++mRevision;
		return mInventory.back().instanceId;
	}

	ModulePlacementFailure ModuleLoadout::Install(
		const ModuleRulesContext& rules,
		std::uint32_t instanceId,
		sf::Vector2i origin,
		int rotationQuarterTurns)
	{
		const auto found = FindById(mInventory, instanceId);
		if (found == mInventory.end()) return ModulePlacementFailure::UnknownModule;

		ModulePlacement candidate = *found;
		candidate.origin = origin;
		candidate.rotationQuarterTurns = rotationQuarterTurns;
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

		ModulePlacement candidate = *found;
		candidate.origin = origin;
		candidate.rotationQuarterTurns = rotationQuarterTurns;
		const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(rules, mPlacements, candidate, instanceId);
		if (failure != ModulePlacementFailure::None) return failure;

		mPlacements[static_cast<std::size_t>(found - mPlacements.cbegin())] = std::move(candidate);
		++mRevision;
		return ModulePlacementFailure::None;
	}

	bool ModuleLoadout::Uninstall(std::uint32_t instanceId)
	{
		const auto found = FindById(mPlacements, instanceId);
		if (found == mPlacements.end()) return false;

		ModulePlacement item = *found;
		item.origin = sf::Vector2i{};
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
