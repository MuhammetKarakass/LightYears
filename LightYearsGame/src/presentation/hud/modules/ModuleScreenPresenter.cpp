#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace vega
{
	namespace
	{
		constexpr float FailureMessageSeconds = 2.5f;

		struct ZoneInfo
		{
			ModuleZoneKind kind;
			const GameplayTag& tag;
			const char* name;
		};

		const List<ZoneInfo>& GetZones()
		{
			static const List<ZoneInfo> zones{
				{ ModuleZoneKind::Nose, GameplayTags::Module::Zone::Nose, "Nose" },
				{ ModuleZoneKind::Wings, GameplayTags::Module::Zone::Wings, "Wings" },
				{ ModuleZoneKind::Hull, GameplayTags::Module::Zone::Hull, "Hull" },
				{ ModuleZoneKind::Core, GameplayTags::Module::Zone::Core, "Core" },
				{ ModuleZoneKind::Engine, GameplayTags::Module::Zone::Engine, "Engine" },
				{ ModuleZoneKind::Sensor, GameplayTags::Module::Zone::Sensor, "Sensor" },
				{ ModuleZoneKind::AbilityBay, GameplayTags::Module::Zone::AbilityBay, "Ability Bay" },
			};
			return zones;
		}

		ModuleZoneKind ZoneKindOf(const GameplayTag& tag)
		{
			for (const ZoneInfo& zone : GetZones())
				if (zone.tag == tag) return zone.kind;
			return ModuleZoneKind::None;
		}

		std::string FamilyLabel(const GameplayTag& family)
		{
			if (family == GameplayTags::Ability::Primary) return "Primary";
			if (family == GameplayTags::Ability::Offense) return "Offense";
			if (family == GameplayTags::Ability::Defense) return "Defense";
			if (family == GameplayTags::Ability::Movement) return "Movement";
			if (family == GameplayTags::Ability::Utility) return "Utility";
			return "Module";
		}

		std::string RarityLabel(ModuleRarity rarity)
		{
			switch (rarity)
			{
			case ModuleRarity::Common: return "Common";
			case ModuleRarity::Uncommon: return "Uncommon";
			case ModuleRarity::Rare: return "Rare";
			case ModuleRarity::Epic: return "Epic";
			case ModuleRarity::Legendary: return "Legendary";
			}
			return {};
		}


		std::string AbilityFamilyLabel(const GameAbilityDefinition& ability)
		{
			for (const GameplayTag& tag : ability.abilityTags)
				if (GameplayTagSchema::IsAbilityCategory(tag)) return FamilyLabel(tag);
			return "Ability";
		}

		// "Module.Primary.FocusedBarrel.Basic" -> "FB". ASCII only, so any font can draw it.
		std::string Abbreviate(const std::string& id)
		{
			std::string name = id;
			const std::size_t second = id.find('.', id.find('.') == std::string::npos ? 0 : id.find('.') + 1);
			if (second != std::string::npos)
			{
				const std::size_t third = id.find('.', second + 1);
				name = id.substr(second + 1, third == std::string::npos ? std::string::npos : third - second - 1);
			}
			std::string result;
			for (char c : name)
				if (std::isupper(static_cast<unsigned char>(c))) result += c;
			return result.empty() ? name.substr(0, 2) : result;
		}

		std::string ConflictText(const ModuleRulesContext& rules, const ModuleLoadout& loadout, std::uint32_t installedInstanceId)
		{
			const ModulePlacement* placement = loadout.FindPlacement(installedInstanceId);
			const ModuleDefinition* definition = placement && rules.lookup ? rules.lookup(placement->moduleId) : nullptr;
			return definition ? "Conflicts with " + definition->displayName : std::string{ "Conflicts with an installed module" };
		}

		// Text for a drop the rules refused as a conflict; names the first installed module that blocks it.
		std::string ConflictMessage(Player& player, std::uint32_t heldId, sf::Vector2i origin, int rotationQuarterTurns)
		{
			const ModuleLoadout& loadout = player.GetModuleLoadout();
			const std::optional<ModuleRulesContext> rules = player.BuildModuleRules();
			const ModulePlacement* held = loadout.FindPlacement(heldId);
			if (!held) held = loadout.FindInventoryItem(heldId);
			if (!rules || !held) return ModulePlacementRules::Describe(ModulePlacementFailure::ModuleConflict);
			ModulePlacement candidate = *held;
			candidate.origin = origin;
			candidate.rotationQuarterTurns = rotationQuarterTurns;
			const List<std::uint32_t> conflicts = ModulePlacementRules::FindConflictingInstances(*rules, loadout.GetPlacements(), candidate, heldId);
			return conflicts.empty() ? std::string{ ModulePlacementRules::Describe(ModulePlacementFailure::ModuleConflict) } : ConflictText(*rules, loadout, conflicts.front());
		}

		bool BuildEntry(const ModuleRulesContext& rules, const ModulePlacement& placement, bool installed, const PlayerSpaceShip* ship, std::uint32_t heldId, const std::string& description, const std::string& sideDescription, int zoneLevelBonus, ModuleEntryView& out)
		{
			const ModuleDefinition* definition = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!definition || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= definition->sizeVariants.size()) return false;
			out = {};
			out.instanceId = placement.instanceId;
			out.name = definition->displayName;
			out.level = placement.level;
			out.levelCount = definition->levelCount;
			out.description = description;
			for (const auto& variant : definition->sizeVariants) out.sizeLadder.push_back(static_cast<int>(variant.shape.cells.size()));
			std::sort(out.sizeLadder.begin(), out.sizeLadder.end());
			out.sizeLadder.erase(std::unique(out.sizeLadder.begin(), out.sizeLadder.end()), out.sizeLadder.end());
			if (definition->sizeVariants.size() > 1)
			{
				out.sideFeatureRequiredCells = static_cast<int>(definition->sizeVariants[static_cast<std::size_t>(GetLargestCellCountVariantIndex(*definition))].shape.cells.size());
				out.sideFeatureState = ModuleSideFeatureViewState::Unselected;
				if (placement.sideFeatureId.IsValid())
				{
					const auto* feature = rules.sideFeatureLookup ? rules.sideFeatureLookup(placement.sideFeatureId) : nullptr;
					if (feature) { out.sideFeatureName = feature->displayName; out.sideFeatureDescription = sideDescription; }
					if (!IsSideFeatureUsableAtLevel(*definition, feature, placement.level)) out.sideFeatureState = ModuleSideFeatureViewState::Unavailable;
					else out.sideFeatureState = IsLargestCellCountVariant(*definition, placement.variantIndex) ? ModuleSideFeatureViewState::Active : ModuleSideFeatureViewState::Locked;
				}
			}
			out.abbreviation = Abbreviate(placement.moduleId.ToString());
			out.family = FamilyLabel(definition->family);
			out.rarity = RarityLabel(definition->rarity);
			out.isSystem = definition->systemCategory.IsValid();
			const sas::AbilitySlot abilitySlot = AbilitySlotForModuleSystem(definition->systemCategory);
			if (abilitySlot != sas::AbilitySlot::None)
			{
				const char* inputLabel = AbilityInputSchema::GetLabel(abilitySlot);
				// Every ability slot shares one family (and so one color) whatever ability it holds.
				out.family = "Ability";
				const GameAbility* ability = ship
					? ship->GetAbilitySystemComponent().GetAbility(abilitySlot)
					: nullptr;
				out.name = inputLabel;
				out.abbreviation = inputLabel;
				if (ability)
				{
					const GameAbilityDefinition& abilityDefinition = ability->GetDefinition();
					out.name += ": " + (abilityDefinition.displayName.empty()
						? abilityDefinition.abilityId
						: abilityDefinition.displayName);
					out.abbreviation = Abbreviate(abilityDefinition.abilityId);
				}
				else
				{
					out.name += " Ability Slot (empty)";
				}
			}
			if (definition->systemCategory == GameplayTags::Module::System::LifeSupport)
				out.name += " (removal kills player)";
			out.variantIndex = placement.variantIndex;
			out.cellCount = static_cast<int>(definition->sizeVariants[static_cast<std::size_t>(placement.variantIndex)].shape.cells.size());
			out.held = placement.instanceId == heldId;
			if (installed)
			{
				out.cells = ModulePlacementRules::GetCoveredCells(*definition, placement);
				out.zoneLevelBonus = zoneLevelBonus;
			}
			return true;
		}
	}

	ModuleScreenPresenter::ModuleScreenPresenter() : mViewModel{ std::make_shared<ModuleScreenViewModel>() }
	{
	}

	void ModuleScreenPresenter::ToggleOpen()
	{
		SetOpen(!mOpen);
	}

	void ModuleScreenPresenter::SetOpen(bool open)
	{
		mOpen = open;
		if (!open)
		{
			mContentValid = false;
			ClearHold();
			mHoverCell.reset();
			mMessage.clear();
			mMessageRemaining = 0.f;
		}
	}

	void ModuleScreenPresenter::SetHoverCell(std::optional<sf::Vector2i> cell)
	{
		mHoverCell = cell;
	}

	void ModuleScreenPresenter::ClearHold()
	{
		mHeldId = 0;
		mHeldRotation = 0;
	}

	void ModuleScreenPresenter::CancelHold()
	{
		ClearHold();
	}

	void ModuleScreenPresenter::ShowFailure(const char* text)
	{
		mMessage = text;
		mMessageRemaining = FailureMessageSeconds;
	}

	void ModuleScreenPresenter::PickUpInventory(std::uint32_t instanceId)
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!mOpen || !player || !player->GetModuleLoadout().FindInventoryItem(instanceId)) return;
		mHeldId = instanceId;
		mHeldRotation = 0;
	}

	bool ModuleScreenPresenter::PickUpAtCell(sf::Vector2i cell)
	{
		if (!mOpen) return false;
		for (const ModuleEntryView& entry : mViewModel->installed)
		{
			if (std::find(entry.cells.begin(), entry.cells.end(), cell) == entry.cells.end()) continue;
			Player* player = PlayerManager::GetPlayerManager().GetPlayer();
			const ModulePlacement* placement = player ? player->GetModuleLoadout().FindPlacement(entry.instanceId) : nullptr;
			if (!placement) return false;
			mHeldId = placement->instanceId;
			mHeldRotation = placement->rotationQuarterTurns;
			return true;
		}
		return false;
	}

	void ModuleScreenPresenter::RotateHeld(int quarterTurns)
	{
		if (mHeldId != 0) mHeldRotation = ((mHeldRotation + quarterTurns) % 4 + 4) % 4;
	}

	bool ModuleScreenPresenter::DropOnGrid()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!mOpen || !player || mHeldId == 0 || !mHoverCell) return false;
		const bool installed = player->GetModuleLoadout().FindPlacement(mHeldId) != nullptr;
		const ModulePlacementFailure failure = installed ? player->TryMoveModule(mHeldId, *mHoverCell, mHeldRotation) : player->TryInstallModule(mHeldId, *mHoverCell, mHeldRotation);
		if (failure != ModulePlacementFailure::None)
		{
			const std::string text = failure == ModulePlacementFailure::ModuleConflict ? ConflictMessage(*player, mHeldId, *mHoverCell, mHeldRotation) : std::string{ ModulePlacementRules::Describe(failure) };
			ShowFailure(text.c_str());
			return false;
		}
		ClearHold();
		mMessage.clear();
		mMessageRemaining = 0.f;
		return true;
	}

	bool ModuleScreenPresenter::DropOnInventory()
	{
		if (mHeldId == 0) return false;
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player || !player->GetModuleLoadout().FindPlacement(mHeldId))
		{
			ClearHold();
			return false;
		}
		return UninstallHeld();
	}

	bool ModuleScreenPresenter::UninstallHeld()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!mOpen || !player || mHeldId == 0 || !player->GetModuleLoadout().FindPlacement(mHeldId)) return false;
		if (!player->TryUninstallModule(mHeldId))
		{
			ShowFailure("Module could not be uninstalled.");
			return false;
		}
		ClearHold();
		return true;
	}

	void ModuleScreenPresenter::Tick(float deltaTime)
	{
		if (mMessageRemaining > 0.f)
		{
			mMessageRemaining -= deltaTime;
			if (mMessageRemaining <= 0.f)
			{
				mMessageRemaining = 0.f;
				mMessage.clear();
			}
		}
		Rebuild();
	}

	void ModuleScreenPresenter::Rebuild()
	{
		ModuleScreenViewModel& vm = *mViewModel;
		SetIfChanged(vm.isOpen, mOpen, vm.revision);
		if (!mOpen) return;

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		const std::optional<ModuleRulesContext> rules = player ? player->BuildModuleRules() : std::nullopt;
		if (!rules)
		{
			mContentValid = false;
			ClearHold();
			SetIfChanged(vm.hasSchematic, false, vm.revision);
			SetIfChanged(vm.gridWidth, 0, vm.revision);
			SetIfChanged(vm.gridHeight, 0, vm.revision);
			SetIfChanged(vm.cellZones, List<ModuleZoneKind>{}, vm.revision);
			SetIfChanged(vm.cellLocked, List<char>{}, vm.revision);
			SetIfChanged(vm.legend, List<ModuleLegendEntry>{}, vm.revision);
			if (SetIfChanged(vm.installed, List<ModuleEntryView>{}, vm.revision)) vm.installedRevision.Bump();
			if (SetIfChanged(vm.inventory, List<ModuleEntryView>{}, vm.revision)) vm.inventoryRevision.Bump();
			SetIfChanged(vm.preview, ModulePreviewView{}, vm.revision);
			SetIfChanged(vm.heldInstanceId, 0u, vm.revision);
			SetIfChanged(vm.heldRotation, 0, vm.revision);
			SetIfChanged(vm.statusMessage, std::string{}, vm.revision);
			return;
		}

		const ModuleLoadout& loadout = player->GetModuleLoadout();
		const shared_ptr<PlayerSpaceShip> ship = player->GetCurrentSpaceShip().lock();
		const PlayerSpaceShip* livingShip = ship && !ship->GetIsPendingDestroy() ? ship.get() : nullptr;
		const ShipSchematic& schematic = rules->schematic;
		const ModuleZoneTable& zoneTable = rules->zoneTable;

		const ModulePlacement* held = mHeldId != 0 ? loadout.FindPlacement(mHeldId) : nullptr;
		const bool heldInstalled = held != nullptr;
		if (!held && mHeldId != 0) held = loadout.FindInventoryItem(mHeldId);
		if (!held) ClearHold();

		const unsigned int playerId = player->GetUniqueID(), shipId = livingShip ? livingShip->GetUniqueID() : 0;
		const std::uint64_t catalogRevision = content::ModuleContentCatalog::GetRevision();
		if (mPlayerId != playerId || mCatalogRevision != catalogRevision) mDescriptions.clear();
		if (!mContentValid || mPlayerId != playerId || mShipId != shipId || mLoadoutRevision != loadout.GetRevision() || mCatalogRevision != catalogRevision)
		{
			++mContentBuildCount;
			List<ModuleZoneKind> zones;
			zones.reserve(schematic.cellZones.size());
			for (const GameplayTag& tag : schematic.cellZones) zones.push_back(ZoneKindOf(tag));
			List<char> locked = schematic.cellLocked; locked.resize(schematic.cellZones.size(), 0);

			List<ModuleLegendEntry> legend;
			for (const ZoneInfo& zone : GetZones())
			{
				const ModuleZoneEntry* entry = zoneTable.FindZone(zone.tag);
				std::string label = zone.name;
				if (zone.kind == ModuleZoneKind::AbilityBay) label += "  Q/E/F/R only";
				else if (entry) label += "  " + (entry->anyFamily ? std::string{ "Any" } : FamilyLabel(entry->family)) + "  +" + std::to_string(entry->levelBonus) + " Lv";
				legend.push_back({ zone.kind, label });
			}

			List<ModuleEntryView> installed, inventory;
			ModuleEntryView entry;
			const auto build = [&](const ModulePlacement& placement, bool isInstalled, List<ModuleEntryView>& entries)
			{
				const ModuleDefinition* definition = rules->lookup(placement.moduleId);
				if (!definition || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= definition->sizeVariants.size()) return;
				// An installed module works at its zone level; the side feature stays at the placement level.
				const bool abilitySlotModule = AbilitySlotForModuleSystem(definition->systemCategory) != sas::AbilitySlot::None;
				const int zoneLevelBonus = isInstalled && !abilitySlotModule ? ModuleResolver::GetZoneLevelBonus(*rules, *definition, placement) : 0;
				const int mainLevel = placement.level + zoneLevelBonus;
				const DescriptionKey key{ placement.moduleId.ToString(), placement.variantIndex, mainLevel, placement.level, placement.sideFeatureId.ToString() };
				auto cached = mDescriptions.find(key);
				if (cached == mDescriptions.end())
				{
					++mDescriptionFormatCount;
					const auto* feature = placement.sideFeatureId.IsValid() && rules->sideFeatureLookup ? rules->sideFeatureLookup(placement.sideFeatureId) : nullptr;
					cached = mDescriptions.emplace(key, std::make_pair(FormatModuleDescription(*definition, definition->sizeVariants[static_cast<std::size_t>(placement.variantIndex)], mainLevel), feature ? FormatModuleSideFeatureDescription(*feature, placement.level) : std::string{})).first;
				}
				if (BuildEntry(*rules, placement, isInstalled, livingShip, mHeldId, cached->second.first, cached->second.second, zoneLevelBonus, entry)) entries.push_back(entry);
			};
			installed.reserve(loadout.GetPlacements().size());
			inventory.reserve(loadout.GetInventory().size());
			for (const ModulePlacement& placement : loadout.GetPlacements()) build(placement, true, installed);
			for (const ModulePlacement& placement : loadout.GetInventory()) build(placement, false, inventory);
			const auto fillConflicts = [&](List<ModuleEntryView>& entries)
			{
				for (ModuleEntryView& view : entries)
				{
					const ModulePlacement* item = loadout.FindPlacement(view.instanceId);
					if (!item) item = loadout.FindInventoryItem(view.instanceId);
					if (item) view.conflictingInstanceIds = ModulePlacementRules::FindConflictingInstances(*rules, loadout.GetPlacements(), *item, view.instanceId);
				}
			};
			fillConflicts(installed);
			fillConflicts(inventory);

			SetIfChanged(vm.hasSchematic, true, vm.revision);
			SetIfChanged(vm.gridWidth, schematic.width, vm.revision);
			SetIfChanged(vm.gridHeight, schematic.height, vm.revision);
			if (SetIfChanged(vm.cellZones, zones, vm.revision)) vm.installedRevision.Bump();
			SetIfChanged(vm.cellLocked, locked, vm.revision);
			if (SetIfChanged(vm.legend, legend, vm.revision)) vm.inventoryRevision.Bump();
			if (SetIfChanged(vm.installed, installed, vm.revision)) vm.installedRevision.Bump();
			if (SetIfChanged(vm.inventory, inventory, vm.revision)) vm.inventoryRevision.Bump();
			mContentValid = true;
			mPlayerId = playerId;
			mShipId = shipId;
			mLoadoutRevision = loadout.GetRevision();
			mCatalogRevision = catalogRevision;
		}

		const auto update = [&](List<ModuleEntryView>& entries, bool isInstalled, UIRevision& listRevision)
		{
			bool changed = false;
			for (ModuleEntryView& view : entries)
			{
				const bool heldChanged = SetIfChanged(view.held, view.instanceId == mHeldId, vm.revision);
				if (isInstalled) changed |= heldChanged;
				const bool rebooting = isInstalled && livingShip && livingShip->GetModuleApplier().IsRebooting(view.instanceId);
				const float remaining = rebooting ? livingShip->GetModuleApplier().GetRebootRemainingSeconds(view.instanceId) : 0.f;
				changed |= SetIfChanged(view.rebooting, rebooting, vm.revision);
				changed |= SetIfChanged(view.rebootRemaining, std::round(remaining * 10.f) / 10.f, vm.revision);
				const float fraction = rebooting && zoneTable.rebootDurationSeconds > 0.f ? std::round(std::clamp(1.f - remaining / zoneTable.rebootDurationSeconds, 0.f, 1.f) * 50.f) / 50.f : 0.f;
				changed |= SetIfChanged(view.rebootFraction, fraction, vm.revision);
				if (view.family != "Ability") continue;
				// Abilities can be equipped without mutating the module loadout.
				const ModulePlacement* placement = isInstalled ? loadout.FindPlacement(view.instanceId) : loadout.FindInventoryItem(view.instanceId);
				const ModuleDefinition* definition = placement ? rules->lookup(placement->moduleId) : nullptr;
				const sas::AbilitySlot slot = definition ? AbilitySlotForModuleSystem(definition->systemCategory) : sas::AbilitySlot::None;
				if (slot != sas::AbilitySlot::None)
				{
					const GameAbility* ability = livingShip ? livingShip->GetAbilitySystemComponent().GetAbility(slot) : nullptr;
					const std::string label = AbilityInputSchema::GetLabel(slot);
					const std::string name = ability ? label + ": " + (ability->GetDefinition().displayName.empty() ? ability->GetDefinition().abilityId : ability->GetDefinition().displayName) : label + " Ability Slot (empty)";
					changed |= SetIfChanged(view.name, name, vm.revision);
					changed |= SetIfChanged(view.abbreviation, ability ? Abbreviate(ability->GetDefinition().abilityId) : label, vm.revision);
				}
			}
			if (changed) listRevision.Bump();
		};
		update(vm.installed, true, vm.installedRevision);
		update(vm.inventory, false, vm.inventoryRevision);

		ModulePreviewView preview;
		const ModuleDefinition* heldDefinition = held && rules->lookup ? rules->lookup(held->moduleId) : nullptr;
		if (heldDefinition && mHoverCell)
		{
			ModulePlacement candidate = *held;
			candidate.origin = *mHoverCell;
			candidate.rotationQuarterTurns = mHeldRotation;
			const ModulePlacementFailure failure = ModulePlacementRules::CanPlace(*rules, loadout, candidate, heldInstalled ? held->instanceId : 0u);
			preview.active = true;
			preview.cells = ModulePlacementRules::GetCoveredCells(*heldDefinition, candidate);
			bool swapsSlots = false;
			if (failure == ModulePlacementFailure::Overlap && heldInstalled && AbilitySlotForModuleSystem(heldDefinition->systemCategory) != sas::AbilitySlot::None)
				for (const ModulePlacement& other : loadout.GetPlacements())
				{
					const ModuleDefinition* otherDefinition = other.instanceId != held->instanceId ? rules->lookup(other.moduleId) : nullptr;
					if (!otherDefinition || AbilitySlotForModuleSystem(otherDefinition->systemCategory) == sas::AbilitySlot::None) continue;
					const List<sf::Vector2i> covered = ModulePlacementRules::GetCoveredCells(*otherDefinition, other);
					if (std::find(covered.begin(), covered.end(), *mHoverCell) != covered.end()) { swapsSlots = true; break; }
				}
			preview.legal = failure == ModulePlacementFailure::None || swapsSlots;
			if (preview.legal && AbilitySlotForModuleSystem(heldDefinition->systemCategory) == sas::AbilitySlot::None)
				preview.zoneLevelBonus = ModuleResolver::GetZoneLevelBonus(*rules, *heldDefinition, candidate);
			else if (!preview.legal && failure == ModulePlacementFailure::ModuleConflict)
			{
				preview.conflictingInstanceIds = ModulePlacementRules::FindConflictingInstances(*rules, loadout.GetPlacements(), candidate, heldInstalled ? held->instanceId : 0u);
				preview.message = preview.conflictingInstanceIds.empty() ? std::string{ ModulePlacementRules::Describe(failure) } : ConflictText(*rules, loadout, preview.conflictingInstanceIds.front());
			}
			else if (!preview.legal) preview.message = ModulePlacementRules::Describe(failure);
		}

		SetIfChanged(vm.preview, preview, vm.revision);
		SetIfChanged(vm.heldInstanceId, mHeldId, vm.revision);
		SetIfChanged(vm.heldRotation, mHeldRotation, vm.revision);
		SetIfChanged(vm.statusMessage, mMessage, vm.revision);
	}
}
