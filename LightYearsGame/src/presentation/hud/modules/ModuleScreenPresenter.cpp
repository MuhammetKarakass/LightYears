#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModuleResolver.h"
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

namespace ly
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

		sas::AbilitySlot AbilitySlotForSystem(const GameplayTag& system)
		{
			if (system == GameplayTags::Module::System::AbilitySlot1) return sas::AbilitySlot::Ability1;
			if (system == GameplayTags::Module::System::AbilitySlot2) return sas::AbilitySlot::Ability2;
			if (system == GameplayTags::Module::System::AbilitySlot3) return sas::AbilitySlot::Ability3;
			if (system == GameplayTags::Module::System::AbilitySlot4) return sas::AbilitySlot::Ability4;
			return sas::AbilitySlot::None;
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

		int ToPercent(float bonus)
		{
			return static_cast<int>(std::lround(bonus * 100.f));
		}

		std::string FormatMagnitude(float magnitude)
		{
			char buffer[32];
			std::snprintf(buffer, sizeof(buffer), "%g", magnitude);
			std::string value{ buffer };
			std::replace(value.begin(), value.end(), '.', ',');
			return value;
		}

		void ReplaceAll(std::string& text, const std::string& token, const std::string& replacement)
		{
			std::size_t position = 0;
			while ((position = text.find(token, position)) != std::string::npos)
			{
				text.replace(position, token.size(), replacement);
				position += replacement.size();
			}
		}

		std::string FormatDescription(const ModuleDefinition& definition, const ModuleSizeVariant& variant)
		{
			std::string description = definition.description;
			const auto replaceModifier = [&description](const sas::AttributeModifier& modifier)
			{
				const std::string token = "{" + std::string{ modifier.attributeId.GetName() } + "}";
				ReplaceAll(description, token, FormatMagnitude(modifier.magnitude));
			};
			for (const ModuleAttributeModifier& modifier : variant.attributeModifiers) replaceModifier(modifier.modifier);
			for (const ModuleAbilityModifier& modifier : variant.abilityModifiers) replaceModifier(modifier.modifier);
			return description;
		}

		bool BuildEntry(const ModuleRulesContext& rules, const ModulePlacement& placement, bool installed, const PlayerSpaceShip* ship, float rebootTotal, std::uint32_t heldId, ModuleEntryView& out)
		{
			const ModuleDefinition* definition = rules.lookup ? rules.lookup(placement.moduleId) : nullptr;
			if (!definition || placement.variantIndex < 0 || static_cast<std::size_t>(placement.variantIndex) >= definition->sizeVariants.size()) return false;
			out = {};
			out.instanceId = placement.instanceId;
			out.name = definition->displayName;
			out.description = FormatDescription(*definition, definition->sizeVariants[static_cast<std::size_t>(placement.variantIndex)]);
			out.abbreviation = Abbreviate(placement.moduleId.ToString());
			out.family = FamilyLabel(definition->family);
			out.isSystem = definition->systemCategory.IsValid();
			const sas::AbilitySlot abilitySlot = AbilitySlotForSystem(definition->systemCategory);
			if (abilitySlot != sas::AbilitySlot::None)
			{
				const char* inputLabel = AbilityInputSchema::GetLabel(abilitySlot);
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
					out.family = AbilityFamilyLabel(abilityDefinition);
				}
				else
				{
					out.name += " Ability Slot (empty)";
					out.family = "Ability";
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
				out.bonusPercent = AbilitySlotForSystem(definition->systemCategory) == sas::AbilitySlot::None
					? ToPercent(ModuleResolver::GetZoneBonus(rules, *definition, placement))
					: 0;
				if (ship && ship->GetModuleApplier().IsRebooting(placement.instanceId))
				{
					const float remaining = ship->GetModuleApplier().GetRebootRemainingSeconds(placement.instanceId);
					out.rebooting = true;
					// Tenths keep the model from changing every frame.
					out.rebootRemaining = std::round(remaining * 10.f) / 10.f;
					out.rebootFraction = rebootTotal > 0.f ? std::clamp(1.f - remaining / rebootTotal, 0.f, 1.f) : 0.f;
				}
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
			ShowFailure(ModulePlacementRules::Describe(failure));
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
			ClearHold();
			SetIfChanged(vm.hasSchematic, false, vm.revision);
			SetIfChanged(vm.gridWidth, 0, vm.revision);
			SetIfChanged(vm.gridHeight, 0, vm.revision);
			SetIfChanged(vm.cellZones, List<ModuleZoneKind>{}, vm.revision);
			SetIfChanged(vm.legend, List<ModuleLegendEntry>{}, vm.revision);
			SetIfChanged(vm.installed, List<ModuleEntryView>{}, vm.revision);
			SetIfChanged(vm.inventory, List<ModuleEntryView>{}, vm.revision);
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

		List<ModuleZoneKind> zones;
		zones.reserve(schematic.cellZones.size());
		for (const GameplayTag& tag : schematic.cellZones) zones.push_back(ZoneKindOf(tag));

		List<ModuleLegendEntry> legend;
		for (const ZoneInfo& zone : GetZones())
		{
			const ModuleZoneEntry* entry = zoneTable.FindZone(zone.tag);
			std::string label = zone.name;
			if (entry) label += "  " + (entry->anyFamily ? std::string{ "Any" } : FamilyLabel(entry->family)) + "  +" + std::to_string(ToPercent(entry->bonus)) + "%";
			legend.push_back({ zone.kind, label });
		}

		List<ModuleEntryView> installed, inventory;
		ModuleEntryView entry;
		for (const ModulePlacement& placement : loadout.GetPlacements())
			if (BuildEntry(*rules, placement, true, livingShip, zoneTable.rebootDurationSeconds, mHeldId, entry)) installed.push_back(entry);
		for (const ModulePlacement& placement : loadout.GetInventory())
			if (BuildEntry(*rules, placement, false, livingShip, 0.f, mHeldId, entry)) inventory.push_back(entry);

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
			preview.legal = failure == ModulePlacementFailure::None;
			if (preview.legal && AbilitySlotForSystem(heldDefinition->systemCategory) == sas::AbilitySlot::None)
				preview.bonusPercent = ToPercent(ModuleResolver::GetZoneBonus(*rules, *heldDefinition, candidate));
			else if (!preview.legal) preview.message = ModulePlacementRules::Describe(failure);
		}

		SetIfChanged(vm.hasSchematic, true, vm.revision);
		SetIfChanged(vm.gridWidth, schematic.width, vm.revision);
		SetIfChanged(vm.gridHeight, schematic.height, vm.revision);
		SetIfChanged(vm.cellZones, zones, vm.revision);
		SetIfChanged(vm.legend, legend, vm.revision);
		SetIfChanged(vm.installed, installed, vm.revision);
		SetIfChanged(vm.inventory, inventory, vm.revision);
		SetIfChanged(vm.preview, preview, vm.revision);
		SetIfChanged(vm.heldInstanceId, mHeldId, vm.revision);
		SetIfChanged(vm.heldRotation, mHeldRotation, vm.revision);
		SetIfChanged(vm.statusMessage, mMessage, vm.revision);
	}
}
