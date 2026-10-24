#include "gameFramework/LightYearsAssets.h"
#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenHUDController.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "presentation/hud/modules/ModuleScreenView.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
	using Json = nlohmann::json;

	class HeadlessModuleLevel final : public vega::GameLevel
	{
	public:
		explicit HeadlessModuleLevel(vega::Application* application) : GameLevel{ application } {}
		bool Dispatch(const sf::Event& event) { return DispatchEvent(event); }
		std::shared_ptr<vega::ModuleScreenHUDController> controller;

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override
		{
			controller = std::make_shared<vega::ModuleScreenHUDController>(std::weak_ptr<vega::GameHUD>{});
			AddHUDController(controller);
		}
		void OnGameStart() override {}
	};

	struct PlayerManagerResetGuard
	{
		vega::PlayerManager& manager;
		~PlayerManagerResetGuard() { manager.Reset(); }
	};

	struct Report
	{
		Json artifact{ { "test", "ModuleScreen" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, const Json& actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = actual;
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}
	};

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open ModuleScreen artifact path" };
		output << artifact.dump(2) << '\n';
	}

	Json CellsJson(const vega::List<sf::Vector2i>& cells)
	{
		Json result = Json::array();
		for (const sf::Vector2i& cell : cells) result.push_back({ cell.x, cell.y });
		return result;
	}

	std::uint32_t FindInventoryId(const vega::Player& player, const std::string& moduleId, int variant = 0)
	{
		for (const vega::ModulePlacement& item : player.GetModuleLoadout().GetInventory())
			if (item.moduleId.ToString() == moduleId && item.variantIndex == variant) return item.instanceId;
		return 0;
	}

	void RunTests(Report& report, const std::filesystem::path& artifacts)
	{
		using namespace vega;
		const std::string piercingTip = "Module.Primary.PiercingTip.Basic";

		std::string title = "Module Screen E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		Player& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(ship), "player ship spawned");
		level->BeginPlayInternal();
		level->TickInternal(0.f);
		const auto presenter = level->controller->GetPresenter();
		const auto model = presenter->GetViewModel();
		report.Check(!model->isOpen && !model->hasSchematic, "screen starts closed and does no work");

		// Tab opens and closes; key repeat does not flicker it.
		const sf::Event tabDown{ sf::Event::KeyPressed{ sf::Keyboard::Key::Tab } };
		const sf::Event tabUp{ sf::Event::KeyReleased{ sf::Keyboard::Key::Tab } };
		report.Check(level->Dispatch(tabDown), "Tab press is consumed by the module screen");
		level->Dispatch(tabDown);
		level->TickInternal(0.f);
		report.Check(model->isOpen, "Tab opens the screen and held-key repeat does not close it");
		level->Dispatch(tabUp);
		level->Dispatch(tabDown);
		level->Dispatch(tabUp);
		level->TickInternal(0.f);
		report.Check(!model->isOpen, "second Tab press closes the screen");
		level->Dispatch(tabDown);
		level->Dispatch(tabUp);
		level->TickInternal(0.f);
		report.Check(model->isOpen && !level->IsPaused(), "screen is open and the level is not paused");

		// ViewModel reflects the default loadout.
		const ModuleLoadout& loadout = player.GetModuleLoadout();
		report.Check(model->hasSchematic && model->gridWidth == 16 && model->gridHeight == 10 && model->cellZones.size() == 160, "grid size comes from the schematic");

		// Layout rules every player ship follows: one horizontal Core rectangle, one contiguous Ability Bay row,
		// Life Support in the Nose and a mirror-symmetric silhouette.
		for (const std::string& shipId : content::ShipContentCatalog::GetPlayerShipIds())
		{
			const ShipDefinition& shipDefinition = content::ShipContentCatalog::GetPlayerShipDefinition(shipId);
			if (!shipDefinition.schematic) continue;
			const ShipSchematic& layout = *shipDefinition.schematic;
			int coreCells = 0, coreMinX = layout.width, coreMaxX = -1, coreMinY = layout.height, coreMaxY = -1;
			int bayCells = 0, bayMinX = layout.width, bayMaxX = -1, bayMinY = layout.height, bayMaxY = -1;
			bool symmetric = true;
			for (int y = 0; y < layout.height; ++y)
				for (int x = 0; x < layout.width; ++x)
				{
					const GameplayTag& zone = layout.GetZoneAt(x, y);
					if (zone != layout.GetZoneAt(layout.width - 1 - x, y)) symmetric = false;
					if (zone == GameplayTags::Module::Zone::Core) { ++coreCells; coreMinX = std::min(coreMinX, x); coreMaxX = std::max(coreMaxX, x); coreMinY = std::min(coreMinY, y); coreMaxY = std::max(coreMaxY, y); }
					if (zone == GameplayTags::Module::Zone::AbilityBay) { ++bayCells; bayMinX = std::min(bayMinX, x); bayMaxX = std::max(bayMaxX, x); bayMinY = std::min(bayMinY, y); bayMaxY = std::max(bayMaxY, y); }
				}
			const int coreWidth = coreMaxX - coreMinX + 1, coreHeight = coreMaxY - coreMinY + 1;
			report.Check(coreCells > 0 && coreWidth >= 1 && coreHeight >= 1, shipId + " has a Core area");
			report.Check(bayCells >= 4 && bayMinY == bayMaxY, shipId + " Ability Bay is one row of at least four cells");
			report.Check(symmetric, shipId + " schematic is mirror-symmetric");
			bool lifeSupportInNose = false;
			for (const DefaultModulePlacement& placement : layout.defaultModules)
				if (placement.moduleId == "Module.Utility.LifeSupportUnit.Standard") lifeSupportInNose = layout.GetZoneAt(placement.origin.x, placement.origin.y) == GameplayTags::Module::Zone::Nose;
			report.Check(lifeSupportInNose, shipId + " Life Support sits in the Nose");
		}
		report.Check(model->legend.size() == 7 && model->legend[0].label == "Nose  Primary  +1 Lv" && model->legend[2].label == "Hull  Any  +2 Lv", "legend lists the seven zones with level bonuses",
			{ { "nose", model->legend[0].label }, { "hull", model->legend[2].label } });
		report.Check(model->cellZones[1 * 16 + 7] == ModuleZoneKind::Nose && model->cellZones[0] == ModuleZoneKind::None && model->cellZones[6 * 16 + 5] == ModuleZoneKind::Hull && model->cellZones[5 * 16 + 5] == ModuleZoneKind::AbilityBay && model->cellLocked.size() == 160 && model->cellLocked[0 * 16 + 7] != 0 && model->cellLocked[1 * 16 + 7] == 0, "cell zones map the schematic rows");
		report.Check(model->installed.size() == loadout.GetPlacements().size() && model->inventory.size() == loadout.GetInventory().size() && !model->inventory.empty(),
			"installed and inventory lists match the loadout", { { "installed", model->installed.size() }, { "inventory", model->inventory.size() } });
		bool hasSystem = false;
		for (const ModuleEntryView& entry : model->installed) hasSystem = hasSystem || (entry.isSystem && !entry.cells.empty() && !entry.abbreviation.empty());
		report.Check(hasSystem && !model->installed.front().rebooting, "default system modules are flagged and not rebooting");

		// Preview legality, bonus and rotation.
		const std::uint32_t tipId = FindInventoryId(player, piercingTip);
		report.Check(tipId != 0, "test inventory holds the piercing tip");
		// The Nose has no free two-cell spot while the default weapon sits there; take it out for the preview checks.
		for (const ModulePlacement& placement : loadout.GetPlacements())
			if (placement.moduleId == "Module.Primary.PrimaryWeapon.Standard") { player.TryUninstallModule(placement.instanceId); break; }
		level->TickInternal(0.f);
		presenter->PickUpInventory(tipId);
		presenter->SetHoverCell(sf::Vector2i{ 7, 1 });
		level->TickInternal(0.f);
		report.Check(model->heldInstanceId == tipId && model->preview.active && model->preview.legal && model->preview.zoneLevelBonus == 1 && model->preview.message.empty(),
			"nose cells give a legal preview with +1 Lv", { { "zoneLevelBonus", model->preview.zoneLevelBonus },{ "cells", CellsJson(model->preview.cells) } });
		report.Check(model->preview.cells == List<sf::Vector2i>{ { 7, 1 }, { 8, 1 } }, "preview shows the unrotated shape", CellsJson(model->preview.cells));
		presenter->RotateHeld();
		level->TickInternal(0.f);
		report.Check(model->heldRotation == 1 && model->preview.cells == List<sf::Vector2i>{ { 7, 1 }, { 7, 2 } } && !model->preview.legal, "rotating changes the preview shape (and overlaps Life Support)",
			CellsJson(model->preview.cells));
		presenter->RotateHeld(3);
		presenter->SetHoverCell(sf::Vector2i{ 5, 6 });
		level->TickInternal(0.f);
		report.Check(model->preview.legal && model->preview.zoneLevelBonus == 2, "hull cells give a +2 Lv preview", { { "zoneLevelBonus", model->preview.zoneLevelBonus } });
		presenter->SetHoverCell(sf::Vector2i{ 6, 6 });
		level->TickInternal(0.f);
		report.Check(model->preview.legal && model->preview.zoneLevelBonus == 0 && model->preview.message.empty(), "a legal cell with no bonus has no label and no warning");
		presenter->SetHoverCell(sf::Vector2i{ 0, 0 });
		level->TickInternal(0.f);
		report.Check(model->preview.active && !model->preview.legal && !model->preview.message.empty() && model->preview.zoneLevelBonus == 0, "a void cell previews as illegal with the Describe text",
			{ { "message", model->preview.message } });
		// Locked cells show their zone but take nothing.
		presenter->SetHoverCell(sf::Vector2i{ 7, 0 });
		level->TickInternal(0.f);
		report.Check(model->preview.active && !model->preview.legal && model->preview.message.find("locked") != std::string::npos, "a locked cell previews as illegal and says it is locked", { { "message", model->preview.message } });
		const std::size_t installedBefore = model->installed.size();
		report.Check(!presenter->DropOnGrid(), "dropping on an illegal cell fails");
		level->TickInternal(0.f);
		report.Check(model->statusMessage == model->preview.message && model->heldInstanceId == tipId && model->installed.size() == installedBefore,
			"failure shows briefly, keeps the hold and installs nothing", { { "status", model->statusMessage } });
		level->TickInternal(3.f);
		report.Check(model->statusMessage.empty(), "failure text clears after a moment");

		// Install in combat: routed through Player, the new module reboots, the world keeps ticking.
		ship->SetInvulnerability(false);
		ship->ApplyDamage(1.f);
		report.Check(ship->IsInCombat(), "damage puts the ship in combat");
		presenter->SetHoverCell(sf::Vector2i{ 7, 1 });
		report.Check(presenter->DropOnGrid(), "dropping on a legal nose cell installs through the player");
		report.Check(player.GetModuleLoadout().FindPlacement(tipId) != nullptr && player.GetModuleLoadout().FindInventoryItem(tipId) == nullptr, "player loadout now holds the module installed");
		level->TickInternal(0.f);
		report.Check(model->installed.size() == installedBefore + 1 && model->heldInstanceId == 0 && !model->preview.active, "view model updates next tick after install");
		const ModuleEntryView* installedTip = nullptr;
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == tipId) installedTip = &entry;
		report.Check(installedTip && installedTip->rebooting && installedTip->rebootRemaining > 1.f && installedTip->zoneLevelBonus == 1 && installedTip->cells.size() == 2,
			"in-combat install shows a rebooting module with its level bonus",{ { "remaining", installedTip ? installedTip->rebootRemaining : -1.f } });
		ModuleScreenView rebootView{ application.GetEngineContext(), model, presenter };
		rebootView.Tick(0.f);
		const auto rebootCargoBuilds = rebootView.GetInventoryRefreshCount();
		const auto rebootContentBuilds = presenter->GetContentBuildCount();
		const auto rebootFormats = presenter->GetDescriptionFormatCount();
		const float remainingBefore = ship->GetModuleApplier().GetRebootRemainingSeconds(tipId);
		level->TickInternal(0.5f);
		report.Check(!level->IsPaused() && ship->GetModuleApplier().GetRebootRemainingSeconds(tipId) < remainingBefore, "world time advances while the screen is open",
			{ { "before", remainingBefore }, { "after", ship->GetModuleApplier().GetRebootRemainingSeconds(tipId) } });
		rebootView.Tick(0.f);
		report.Check(presenter->GetContentBuildCount() == rebootContentBuilds && presenter->GetDescriptionFormatCount() == rebootFormats && rebootView.GetInventoryRefreshCount() == rebootCargoBuilds,
			"reboot progress updates without rebuilding content, descriptions or cargo", { { "contentBuilds", rebootContentBuilds }, { "descriptionFormats", rebootFormats }, { "cargoBuilds", rebootCargoBuilds } });
		level->TickInternal(2.f);
		report.Check(!ship->GetModuleApplier().IsRebooting(tipId), "reboot finishes");
		level->TickInternal(0.f);
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == tipId) installedTip = &entry;
		report.Check(!installedTip->rebooting, "reboot indicator disappears");

		// Move the installed module.
		report.Check(presenter->PickUpAtCell({ 8, 1 }), "clicking an installed module picks it up");
		presenter->SetHoverCell(sf::Vector2i{ 5, 6 });
		level->TickInternal(0.f);
		report.Check(model->heldInstanceId == tipId && model->preview.legal && model->preview.cells == List<sf::Vector2i>{ { 5, 6 }, { 6, 6 } }, "moving previews a free Hull target as legal");
		report.Check(presenter->DropOnGrid(), "dropping moves through the player");
		level->TickInternal(0.f);
		const ModulePlacement* moved = player.GetModuleLoadout().FindPlacement(tipId);
		report.Check(moved && moved->origin == sf::Vector2i{ 5, 6 }, "player loadout reflects the move");
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == tipId) installedTip = &entry;
		report.Check(installedTip->cells == List<sf::Vector2i>{ { 5, 6 }, { 6, 6 } } && !installedTip->rebooting, "moved module shows new cells without rebooting");

		// Uninstall by dropping on the inventory, and by key via the view.
		const std::size_t inventoryBefore = model->inventory.size();
		report.Check(presenter->PickUpAtCell({ 5, 6 }) && presenter->DropOnInventory(), "dropping an installed module on the inventory uninstalls it");
		level->TickInternal(0.f);
		report.Check(player.GetModuleLoadout().FindInventoryItem(tipId) != nullptr && model->inventory.size() == inventoryBefore + 1 && model->installed.size() == installedBefore,
			"module returns to the inventory");
		// Movement Rhythm has a level table. In a Hull cell (+2 Lv zone bonus) the installed card describes level 3 and the cargo card level 1.
		const sas::ContentId rhythmModule{ std::string{ "Module.Movement.MovementRhythm.Uncommon" } };
		const ModuleDefinition* rhythm = content::ModuleContentCatalog::FindById(rhythmModule);
		report.Check(rhythm && rhythm->sizeVariants.size() > 0, "Movement Rhythm is shipped with a level table");
		const std::uint32_t rhythmId = player.AddModuleToInventory(rhythmModule, 0, rhythm->rarity);
		const std::string levelOneText = FormatModuleDescription(*rhythm, rhythm->sizeVariants[0], 1);
		const std::string effectiveText = FormatModuleDescription(*rhythm, rhythm->sizeVariants[0], 3);
		report.Check(effectiveText != levelOneText, "the level table changes the Movement Rhythm text between levels 1 and 3");
		ship->SetInvulnerability(false);
		ship->ApplyDamage(1.f);
		report.Check(player.TryInstallModule(rhythmId, sf::Vector2i{ 5, 6 }, 0) == ModulePlacementFailure::None, "Movement Rhythm installs into a free Hull cell");
		level->TickInternal(0.f);
		const ModuleEntryView* installedRhythm = nullptr;
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == rhythmId) installedRhythm = &entry;
		report.Check(installedRhythm && installedRhythm->level == 1 && installedRhythm->zoneLevelBonus == 2 && installedRhythm->rebooting && installedRhythm->description == effectiveText,
			"installed Hull module describes its effective level while rebooting", { { "description", installedRhythm ? installedRhythm->description : std::string{} }, { "expected", effectiveText } });
		report.Check(player.TryUninstallModule(rhythmId), "Movement Rhythm returns to cargo");
		level->TickInternal(0.f);
		const ModuleEntryView* cargoRhythm = nullptr;
		for (const ModuleEntryView& entry : model->inventory) if (entry.instanceId == rhythmId) cargoRhythm = &entry;
		report.Check(cargoRhythm && cargoRhythm->level == 1 && cargoRhythm->zoneLevelBonus == 0 && cargoRhythm->description == levelOneText,
			"the same module in cargo describes its placement level", { { "description", cargoRhythm ? cargoRhythm->description : std::string{} } });

		// Ability bay: only the Q/E/F/R slot modules fit it, they cannot leave it, and dropping one on another swaps them.
		{
			const auto slotId = [&](const char* id) { for (const ModulePlacement& placement : player.GetModuleLoadout().GetPlacements()) if (placement.moduleId == id) return placement.instanceId; return std::uint32_t{ 0 }; };
			const auto slotOrigin = [&](std::uint32_t id) { const ModulePlacement* placement = player.GetModuleLoadout().FindPlacement(id); return placement ? placement->origin : sf::Vector2i{ -1, -1 }; };
			const std::uint32_t qSlot = slotId("Module.Utility.AbilitySlot1.Standard"), eSlot = slotId("Module.Utility.AbilitySlot2.Standard");
			report.Check(qSlot != 0 && eSlot != 0 && slotOrigin(qSlot).y == slotOrigin(eSlot).y && slotOrigin(eSlot).x == slotOrigin(qSlot).x + 1, "Q and E slots sit side by side in the ability bay row");
			report.Check(player.TryInstallModule(tipId, slotOrigin(qSlot), 0) != ModulePlacementFailure::None, "a regular module cannot be installed into the ability bay");
			report.Check(player.TryMoveModule(qSlot, sf::Vector2i{ 5, 6 }, 0) == ModulePlacementFailure::SystemZoneMismatch, "an ability slot cannot leave the ability bay");
			const sf::Vector2i qBefore = slotOrigin(qSlot), eBefore = slotOrigin(eSlot);
			report.Check(player.TryMoveModule(qSlot, eBefore, 0) == ModulePlacementFailure::None && slotOrigin(qSlot) == eBefore && slotOrigin(eSlot) == qBefore, "dropping a slot on another slot swaps their cells");
			report.Check(player.TryMoveModule(qSlot, qBefore, 0) == ModulePlacementFailure::None && slotOrigin(qSlot) == qBefore && slotOrigin(eSlot) == eBefore, "swapping back restores the original order");
		}

		// Flush the slot swaps before measuring idle frames.
		level->TickInternal(0.f);
		// View input routing.
		ModuleScreenView view{ application.GetEngineContext(), presenter->GetViewModel(), std::weak_ptr<ModuleScreenPresenter>{ presenter } };
		view.Tick(0.f);
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true);
		report.Check(view.GetVisibility() && view.GetRefreshCount() == 1, "view builds when the model is open");
		const auto idleContentBuilds = presenter->GetContentBuildCount(), idleFormats = presenter->GetDescriptionFormatCount();
		const auto idleRefresh = view.GetRefreshCount(), idleCargo = view.GetInventoryRefreshCount();
		for (int i = 0; i < 120; ++i) { level->TickInternal(0.f); view.NativeTick(0.f); }
		report.Check(presenter->GetContentBuildCount() == idleContentBuilds && presenter->GetDescriptionFormatCount() == idleFormats && view.GetRefreshCount() == idleRefresh && view.GetInventoryRefreshCount() == idleCargo,
			"120 idle frames do no content, description or widget rebuilds", { { "frames", 120 }, { "contentBuildsBefore", idleContentBuilds }, { "contentBuildsAfter", presenter->GetContentBuildCount() }, { "descriptionFormatsBefore", idleFormats }, { "descriptionFormatsAfter", presenter->GetDescriptionFormatCount() }, { "cargoBuildsBefore", idleCargo }, { "cargoBuildsAfter", view.GetInventoryRefreshCount() } });
		const auto point = [](const sf::Vector2f& value) { return sf::Vector2i{ static_cast<int>(value.x), static_cast<int>(value.y) }; };
		int tipRow = -1;
		for (std::size_t i = 0; i < model->inventory.size(); ++i) if (model->inventory[i].instanceId == tipId) tipRow = static_cast<int>(i);
		report.Check(tipRow >= ModuleScreenView::InventoryVisibleRows, "the returned module sits past the first visible rows");
		while (view.GetInventoryScroll() + ModuleScreenView::InventoryVisibleRows <= tipRow)
			view.HandleEvent(sf::Event{ sf::Event::MouseWheelScrolled{ sf::Mouse::Wheel::Vertical, -1.f, point(view.InventoryRowCenter(0)) } });
		view.Tick(0.f);
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true);
		tipRow -= view.GetInventoryScroll();
		report.Check(tipRow >= 0 && tipRow < ModuleScreenView::InventoryVisibleRows && view.GetInventoryScroll() > 0 && view.InventoryItemAt(view.InventoryRowCenter(tipRow)) == tipId,
			"mouse wheel scrolls the inventory so the module is a visible row", { { "scroll", view.GetInventoryScroll() }, { "row", tipRow } });
		const auto press = [&](sf::Mouse::Button button, const sf::Vector2f& at) { return view.HandleEvent(sf::Event{ sf::Event::MouseButtonPressed{ button, point(at) } }); };
		const auto release = [&](sf::Mouse::Button button, const sf::Vector2f& at) { return view.HandleEvent(sf::Event{ sf::Event::MouseButtonReleased{ button, point(at) } }); };
		const auto move = [&](const sf::Vector2f& at) { return view.HandleEvent(sf::Event{ sf::Event::MouseMoved{ point(at) } }); };
		const auto tickAll = [&]() { level->TickInternal(0.f); view.Tick(0.f); view.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true); };
		const auto hoverContentBuilds = presenter->GetContentBuildCount(), hoverFormats = presenter->GetDescriptionFormatCount(), hoverCargo = view.GetInventoryRefreshCount();
		const auto labelsRevision = model->installedRevision.Get();
		for (int i = 0; i < 60; ++i) { move(view.InventoryRowCenter(i % ModuleScreenView::InventoryVisibleRows)); tickAll(); }
		report.Check(presenter->GetContentBuildCount() == hoverContentBuilds && presenter->GetDescriptionFormatCount() == hoverFormats && view.GetInventoryRefreshCount() == hoverCargo && model->installedRevision.Get() == labelsRevision,
			"60 cargo hover changes reuse content, descriptions, cargo and installed labels", { { "hoverChanges", 60 }, { "cargoBuilds", hoverCargo } });
		const auto screenshot = [&](const std::string& name)
		{
			if (!artifacts.empty()) std::filesystem::create_directories(artifacts);
			auto& window = application.GetRenderWindow();
			const auto originalView = window.getView();
			window.setSize({ 1440u, 944u });
			window.setView(sf::View{ sf::FloatRect{ { 0.f, 0.f }, { 1440.f, 944.f } } });
			view.ResolveLayoutTree({ { 0.f, 0.f }, { 1440.f, 944.f } }, true);
			window.clear(sf::Color{ 2, 5, 12 });
			view.NativeDraw(window);
			sf::Texture texture;
			report.Check(texture.resize(window.getSize()), "filter screenshot texture resized");
			// SFML copies the back buffer; capture before display swaps it away.
			texture.update(window);
			const auto pixels = texture.copyToImage();
			int brightPixels = 0;
			for (unsigned int y = 0; y < pixels.getSize().y; ++y)
				for (unsigned int x = 0; x < pixels.getSize().x; ++x)
				{
					const auto color = pixels.getPixel({ x, y });
					if (color.r > 80 || color.g > 80 || color.b > 80) ++brightPixels;
				}
			report.Check(brightPixels > 1000, "filter capture contains rendered UI", { { "brightPixels", brightPixels } });
			window.display();
			const auto path = artifacts / name;
			report.Check(pixels.saveToFile(path), "real module filter screen saved", path.generic_string());
			window.setSize({ 64u, 64u });
			window.setView(originalView);
			tickAll();
		};
		report.Check(press(sf::Mouse::Button::Left, view.InventoryRowCenter(tipRow)) && release(sf::Mouse::Button::Left, view.InventoryRowCenter(tipRow)), "inventory click is handled by the screen");
		tickAll();
		report.Check(model->heldInstanceId == tipId, "clicking an inventory row picks the module up");
		report.Check(view.GetInventoryRefreshCount() == hoverCargo && presenter->GetContentBuildCount() == hoverContentBuilds && presenter->GetDescriptionFormatCount() == hoverFormats,
			"holding a cargo module changes its highlight without rebuilding cargo or content");
		press(sf::Mouse::Button::Right, view.CellCenter({ 0, 0 }));
		tickAll();
		report.Check(model->heldRotation == 1, "right-click rotates the held module");
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		move(view.CellCenter({ 7, 1 }));
		tickAll();
		report.Check(model->heldRotation == 0 && model->preview.legal && model->preview.cells.size() == 2, "T rotates and mouse move sets the hover cell");
		report.Check(press(sf::Mouse::Button::Left, view.CellCenter({ 7, 1 })), "grid click is handled");
		release(sf::Mouse::Button::Left, view.CellCenter({ 7, 1 }));
		tickAll();
		report.Check(player.GetModuleLoadout().FindPlacement(tipId) != nullptr && model->heldInstanceId == 0, "clicking the grid installs the held module");
		report.Check(view.CellAt(view.CellCenter({ 3, 0 })) == sf::Vector2i{ 3, 0 } && !view.CellAt({ 0.f, 0.f }).has_value(), "cell hit testing matches the layout");

		// Drag an installed module back to the inventory.
		press(sf::Mouse::Button::Left, view.CellCenter({ 7, 1 }));
		tickAll();
		report.Check(model->heldInstanceId == tipId, "pressing an installed module picks it up");
		release(sf::Mouse::Button::Left, view.InventoryRowCenter(0));
		tickAll();
		report.Check(player.GetModuleLoadout().FindInventoryItem(tipId) != nullptr, "dragging onto the inventory uninstalls it");

		// Delete key uninstalls the held module.
		press(sf::Mouse::Button::Left, view.CellCenter({ 6, 3 }));
		tickAll();
		const std::uint32_t readoutId = model->heldInstanceId;
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::Delete } });
		tickAll();
		report.Check(readoutId != 0 && player.GetModuleLoadout().FindInventoryItem(readoutId) != nullptr, "Delete uninstalls the held module");

		// Real cargo filters, hit testing and text input share the same list that is rendered.
		const std::size_t cargoCount = player.GetModuleLoadout().GetInventory().size();
		const auto chooseFilter = [&](int column, const std::string& value)
		{
			List<std::string> values{ "" };
			for (const ModuleEntryView& item : model->inventory)
			{
				const std::string candidate = column == 0 ? item.rarity : (column == 1 ? item.family : std::to_string(item.cellCount));
				if (!candidate.empty() && std::find(values.begin(), values.end(), candidate) == values.end()) values.push_back(candidate);
			}
			if (column == 2) std::sort(values.begin() + 1, values.end(), [](const std::string& a, const std::string& b) { return std::stoi(a) < std::stoi(b); });
			const auto found = std::find(values.begin(), values.end(), value);
			report.Check(found != values.end(), "filter option is present: " + value);
			press(sf::Mouse::Button::Left, view.FilterControlCenter(column));
			release(sf::Mouse::Button::Left, view.FilterControlCenter(column));
			tickAll();
			press(sf::Mouse::Button::Left, view.FilterOptionCenter(column, static_cast<int>(found - values.begin())));
			release(sf::Mouse::Button::Left, view.FilterOptionCenter(column, static_cast<int>(found - values.begin())));
			tickAll();
		};
		chooseFilter(0, "Common");
		chooseFilter(1, "Primary");
		chooseFilter(2, "2");
		const auto expectedCount = std::count_if(model->inventory.begin(), model->inventory.end(), [](const ModuleEntryView& item) { return item.rarity == "Common" && item.family == "Primary" && item.cellCount == 2; });
		report.Check(expectedCount > 0 && view.GetFilteredInventoryCount() == expectedCount && view.GetInventoryScroll() == 0,
			"rarity, category and actual cell size combine and reset the scroll", { { "matches", view.GetFilteredInventoryCount() }, { "cargo", cargoCount } });
		screenshot("module-filters-combined.png");
		press(sf::Mouse::Button::Left, view.SearchFieldCenter());
		release(sf::Mouse::Button::Left, view.SearchFieldCenter());
		for (char c : std::string{ "pIeRcInG tIp" }) view.HandleEvent(sf::Event{ sf::Event::TextEntered{ static_cast<char32_t>(c) } });
		tickAll();
		report.Check(view.GetFilteredInventoryCount() > 0 && view.InventoryItemAt(view.InventoryRowCenter(0)) == tipId,
			"case-insensitive search combines with filters and row hit testing resolves the filtered instance", { { "firstId", view.InventoryItemAt(view.InventoryRowCenter(0)) }, { "tipId", tipId } });
		presenter->PickUpInventory(tipId);
		tickAll();
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::Delete } });
		tickAll();
		report.Check(model->heldInstanceId == tipId && model->heldRotation == 0 && player.GetModuleLoadout().GetInventory().size() == cargoCount,
			"typing focus consumes rotate and delete shortcuts without changing the loadout");
		for (char c : std::string{ "no-such-module-123" }) view.HandleEvent(sf::Event{ sf::Event::TextEntered{ static_cast<char32_t>(c) } });
		tickAll();
		report.Check(view.GetFilteredInventoryCount() == 0 && view.GetInventoryScroll() == 0 && view.InventoryItemAt(view.InventoryRowCenter(0)) == 0,
			"empty search results have no clickable cargo row");
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::Delete } });
		view.HandleEvent(sf::Event{ sf::Event::TextEntered{ U'\u011f' } });
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::Backspace } });
		tickAll();
		report.Check(view.GetFilteredInventoryCount() == expectedCount, "Backspace removes a complete UTF-8 character");
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::Enter } });
		presenter->CancelHold();
		tickAll();
		press(sf::Mouse::Button::Left, view.InventoryRowCenter(0));
		release(sf::Mouse::Button::Left, view.InventoryRowCenter(0));
		tickAll();
		report.Check(model->heldInstanceId == view.InventoryItemAt(view.InventoryRowCenter(0)), "clicking a filtered row picks its real instance");
		presenter->CancelHold();
		const sf::Vector2f reset = view.SearchFieldCenter() + sf::Vector2f{ 276.f, 0.f };
		press(sf::Mouse::Button::Left, reset);
		release(sf::Mouse::Button::Left, reset);
		tickAll();
		report.Check(view.GetFilteredInventoryCount() == static_cast<int>(cargoCount) && view.GetInventoryScroll() == 0 && model->heldInstanceId == 0,
			"Reset restores all cargo without picking or moving a module");
		screenshot("module-filters-all.png");
		const auto inventoryRefreshBeforeMenu = view.GetInventoryRefreshCount();
		press(sf::Mouse::Button::Left, view.FilterControlCenter(1));
		release(sf::Mouse::Button::Left, view.FilterControlCenter(1));
		tickAll();
		report.Check(view.GetInventoryRefreshCount() > inventoryRefreshBeforeMenu, "opening a category dropdown redraws the cargo controls");
		screenshot("module-filters-dropdown.png");

		// Replacing catalog storage must also replace indexes and cached screen text.
		const auto contentPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
		const auto catalogRevision = content::ModuleContentCatalog::GetRevision();
		std::string catalogFailure;
		report.Check(!content::ModuleContentCatalog::LoadFromFile(artifacts / "missing-module-catalog.json", &catalogFailure) && content::ModuleContentCatalog::GetRevision() == catalogRevision,
			"failed catalog reload preserves its generation and lookup indexes");
		const auto formatsBeforeReload = presenter->GetDescriptionFormatCount();
		report.Check(content::ModuleContentCatalog::LoadFromFiles(contentPath / "modules.json", contentPath / "module_side_features.json", &catalogFailure), "catalog successfully reloads", catalogFailure);
		tickAll();
		report.Check(content::ModuleContentCatalog::GetRevision() > catalogRevision && presenter->GetDescriptionFormatCount() > formatsBeforeReload, "successful catalog reload invalidates cached descriptions");
		for (const ModuleDefinition& definition : content::ModuleContentCatalog::GetDefinitions())
			report.Check(content::ModuleContentCatalog::FindById(definition.moduleId) == &definition, "module index resolves reloaded storage: " + definition.moduleId.ToString());
		for (const ModuleSideFeatureDefinition& definition : content::ModuleSideFeatureCatalog::GetDefinitions())
			report.Check(content::ModuleSideFeatureCatalog::FindById(definition.sideFeatureId) == &definition, "side index resolves reloaded storage: " + definition.sideFeatureId.ToString());
		for (const ModuleSideFeaturePool& pool : content::ModuleSideFeatureCatalog::GetPools())
			report.Check(content::ModuleSideFeatureCatalog::FindPool(pool.poolId) == &pool, "pool index resolves reloaded storage: " + pool.poolId.ToString());
		report.Check(!content::ModuleContentCatalog::FindById("Module.Unknown") && !content::ModuleSideFeatureCatalog::FindById("Side.Unknown") && !content::ModuleSideFeatureCatalog::FindPool("Pool.Unknown"), "unknown catalog identities still return null");

		// Closing drops the hold; a closed screen ignores input.
		press(sf::Mouse::Button::Left, view.SearchFieldCenter());
		release(sf::Mouse::Button::Left, view.SearchFieldCenter());
		presenter->PickUpInventory(tipId);
		level->Dispatch(tabDown);
		level->Dispatch(tabUp);
		tickAll();
		report.Check(!model->isOpen && !view.GetVisibility() && model->heldInstanceId == 0 && !move(view.CellCenter({ 3, 0 })), "closing clears the hold and the view ignores input");

		// Without a player there are no modules.
		manager.Reset();
		level->Dispatch(tabDown);
		level->Dispatch(tabUp);
		level->TickInternal(0.f);
		report.Check(model->isOpen && !model->hasSchematic && model->installed.empty() && model->inventory.empty(), "no player or schematic shows an empty screen");
		report.artifact["passed"] = true;
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2)
	{
		std::cerr << "Usage: LightYearsModuleScreenTests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!vega::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		RunTests(report, artifactPath.parent_path());
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << "ModuleScreen tests failed: " << exception.what() << '\n';
	}
	try
	{
		WriteArtifact(artifactPath, report.artifact);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write ModuleScreen artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
