#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/module/ModuleLoadout.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenHUDController.h"
#include "presentation/hud/modules/ModuleScreenView.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
	using Json = nlohmann::json;

	class HeadlessModuleLevel final : public ly::GameLevel
	{
	public:
		explicit HeadlessModuleLevel(ly::Application* application) : GameLevel{ application } {}
		bool Dispatch(const sf::Event& event) { return DispatchEvent(event); }
		std::shared_ptr<ly::ModuleScreenHUDController> controller;

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override
		{
			controller = std::make_shared<ly::ModuleScreenHUDController>(std::weak_ptr<ly::GameHUD>{});
			AddHUDController(controller);
		}
		void OnGameStart() override {}
	};

	struct PlayerManagerResetGuard
	{
		ly::PlayerManager& manager;
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

	Json CellsJson(const ly::List<sf::Vector2i>& cells)
	{
		Json result = Json::array();
		for (const sf::Vector2i& cell : cells) result.push_back({ cell.x, cell.y });
		return result;
	}

	std::uint32_t FindInventoryId(const ly::Player& player, const std::string& moduleId, int variant = 0)
	{
		for (const ly::ModulePlacement& item : player.GetModuleLoadout().GetInventory())
			if (item.moduleId.ToString() == moduleId && item.variantIndex == variant) return item.instanceId;
		return 0;
	}

	void RunTests(Report& report)
	{
		using namespace ly;
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
		report.Check(model->hasSchematic && model->gridWidth == 8 && model->gridHeight == 8 && model->cellZones.size() == 64, "grid size comes from the schematic");
		report.Check(model->legend.size() == 6 && model->legend[0].label == "Nose  Primary  +25%" && model->legend[2].label == "Hull  Any  +10%", "legend lists the six zones with bonuses",
			{ { "nose", model->legend[0].label }, { "hull", model->legend[2].label } });
		report.Check(model->cellZones[3] == ModuleZoneKind::Nose && model->cellZones[0] == ModuleZoneKind::None && model->cellZones[3 * 8 + 3] == ModuleZoneKind::Hull, "cell zones map the schematic rows");
		report.Check(model->installed.size() == loadout.GetPlacements().size() && model->inventory.size() == loadout.GetInventory().size() && !model->inventory.empty(),
			"installed and inventory lists match the loadout", { { "installed", model->installed.size() }, { "inventory", model->inventory.size() } });
		bool hasSystem = false;
		for (const ModuleEntryView& entry : model->installed) hasSystem = hasSystem || (entry.isSystem && !entry.cells.empty() && !entry.abbreviation.empty());
		report.Check(hasSystem && !model->installed.front().rebooting, "default system modules are flagged and not rebooting");

		// Preview legality, bonus and rotation.
		const std::uint32_t tipId = FindInventoryId(player, piercingTip);
		report.Check(tipId != 0, "test inventory holds the piercing tip");
		presenter->PickUpInventory(tipId);
		presenter->SetHoverCell(sf::Vector2i{ 3, 1 });
		level->TickInternal(0.f);
		report.Check(model->heldInstanceId == tipId && model->preview.active && model->preview.legal && model->preview.bonusPercent == 25 && model->preview.message.empty(),
			"nose cells give a legal preview with +25%", { { "bonus", model->preview.bonusPercent }, { "cells", CellsJson(model->preview.cells) } });
		report.Check(model->preview.cells == List<sf::Vector2i>{ { 3, 1 }, { 4, 1 } }, "preview shows the unrotated shape", CellsJson(model->preview.cells));
		presenter->RotateHeld();
		level->TickInternal(0.f);
		report.Check(model->heldRotation == 1 && model->preview.cells == List<sf::Vector2i>{ { 3, 1 }, { 3, 2 } } && model->preview.legal, "rotating changes the preview shape",
			CellsJson(model->preview.cells));
		presenter->RotateHeld(3);
		presenter->SetHoverCell(sf::Vector2i{ 3, 5 });
		level->TickInternal(0.f);
		report.Check(model->preview.legal && model->preview.bonusPercent == 10, "hull cells give a +10% preview", { { "bonus", model->preview.bonusPercent } });
		presenter->SetHoverCell(sf::Vector2i{ 5, 5 });
		level->TickInternal(0.f);
		report.Check(model->preview.legal && model->preview.bonusPercent == 0 && model->preview.message.empty(), "a legal cell with no bonus has no label and no warning");
		presenter->SetHoverCell(sf::Vector2i{ 0, 0 });
		level->TickInternal(0.f);
		report.Check(model->preview.active && !model->preview.legal && !model->preview.message.empty() && model->preview.bonusPercent == 0, "a void cell previews as illegal with the Describe text",
			{ { "message", model->preview.message } });
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
		presenter->SetHoverCell(sf::Vector2i{ 3, 1 });
		report.Check(presenter->DropOnGrid(), "dropping on a legal nose cell installs through the player");
		report.Check(player.GetModuleLoadout().FindPlacement(tipId) != nullptr && player.GetModuleLoadout().FindInventoryItem(tipId) == nullptr, "player loadout now holds the module installed");
		level->TickInternal(0.f);
		report.Check(model->installed.size() == installedBefore + 1 && model->heldInstanceId == 0 && !model->preview.active, "view model updates next tick after install");
		const ModuleEntryView* installedTip = nullptr;
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == tipId) installedTip = &entry;
		report.Check(installedTip && installedTip->rebooting && installedTip->rebootRemaining > 1.f && installedTip->bonusPercent == 25 && installedTip->cells.size() == 2,
			"in-combat install shows a rebooting module with its bonus", { { "remaining", installedTip ? installedTip->rebootRemaining : -1.f } });
		const float remainingBefore = ship->GetModuleApplier().GetRebootRemainingSeconds(tipId);
		level->TickInternal(0.5f);
		report.Check(!level->IsPaused() && ship->GetModuleApplier().GetRebootRemainingSeconds(tipId) < remainingBefore, "world time advances while the screen is open",
			{ { "before", remainingBefore }, { "after", ship->GetModuleApplier().GetRebootRemainingSeconds(tipId) } });
		level->TickInternal(2.f);
		report.Check(!ship->GetModuleApplier().IsRebooting(tipId), "reboot finishes");
		level->TickInternal(0.f);
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == tipId) installedTip = &entry;
		report.Check(!installedTip->rebooting, "reboot indicator disappears");

		// Move the installed module.
		report.Check(presenter->PickUpAtCell({ 4, 1 }), "clicking an installed module picks it up");
		presenter->SetHoverCell(sf::Vector2i{ 3, 2 });
		level->TickInternal(0.f);
		report.Check(model->heldInstanceId == tipId && model->preview.legal && model->preview.cells == List<sf::Vector2i>{ { 3, 2 }, { 4, 2 } }, "moving previews over its own old cells as legal");
		report.Check(presenter->DropOnGrid(), "dropping moves through the player");
		level->TickInternal(0.f);
		const ModulePlacement* moved = player.GetModuleLoadout().FindPlacement(tipId);
		report.Check(moved && moved->origin == sf::Vector2i{ 3, 2 }, "player loadout reflects the move");
		for (const ModuleEntryView& entry : model->installed) if (entry.instanceId == tipId) installedTip = &entry;
		report.Check(installedTip->cells == List<sf::Vector2i>{ { 3, 2 }, { 4, 2 } } && !installedTip->rebooting, "moved module shows new cells without rebooting");

		// Uninstall by dropping on the inventory, and by key via the view.
		const std::size_t inventoryBefore = model->inventory.size();
		report.Check(presenter->PickUpAtCell({ 3, 2 }) && presenter->DropOnInventory(), "dropping an installed module on the inventory uninstalls it");
		level->TickInternal(0.f);
		report.Check(player.GetModuleLoadout().FindInventoryItem(tipId) != nullptr && model->inventory.size() == inventoryBefore + 1 && model->installed.size() == installedBefore,
			"module returns to the inventory");

		// View input routing.
		ModuleScreenView view{ presenter->GetViewModel(), std::weak_ptr<ModuleScreenPresenter>{ presenter } };
		view.Tick(0.f);
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true);
		report.Check(view.GetVisibility() && view.GetRefreshCount() == 1, "view builds when the model is open");
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
		report.Check(press(sf::Mouse::Button::Left, view.InventoryRowCenter(tipRow)) && release(sf::Mouse::Button::Left, view.InventoryRowCenter(tipRow)), "inventory click is handled by the screen");
		tickAll();
		report.Check(model->heldInstanceId == tipId, "clicking an inventory row picks the module up");
		press(sf::Mouse::Button::Right, view.CellCenter({ 0, 0 }));
		tickAll();
		report.Check(model->heldRotation == 1, "right-click rotates the held module");
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::T } });
		move(view.CellCenter({ 3, 1 }));
		tickAll();
		report.Check(model->heldRotation == 0 && model->preview.legal && model->preview.cells.size() == 2, "T rotates and mouse move sets the hover cell");
		report.Check(press(sf::Mouse::Button::Left, view.CellCenter({ 3, 1 })), "grid click is handled");
		release(sf::Mouse::Button::Left, view.CellCenter({ 3, 1 }));
		tickAll();
		report.Check(player.GetModuleLoadout().FindPlacement(tipId) != nullptr && model->heldInstanceId == 0, "clicking the grid installs the held module");
		report.Check(view.CellAt(view.CellCenter({ 3, 0 })) == sf::Vector2i{ 3, 0 } && !view.CellAt({ 0.f, 0.f }).has_value(), "cell hit testing matches the layout");

		// Drag an installed module back to the inventory.
		press(sf::Mouse::Button::Left, view.CellCenter({ 3, 1 }));
		tickAll();
		report.Check(model->heldInstanceId == tipId, "pressing an installed module picks it up");
		release(sf::Mouse::Button::Left, view.InventoryRowCenter(0));
		tickAll();
		report.Check(player.GetModuleLoadout().FindInventoryItem(tipId) != nullptr, "dragging onto the inventory uninstalls it");

		// Delete key uninstalls the held module.
		press(sf::Mouse::Button::Left, view.CellCenter({ 2, 1 }));
		tickAll();
		const std::uint32_t readoutId = model->heldInstanceId;
		view.HandleEvent(sf::Event{ sf::Event::KeyPressed{ sf::Keyboard::Key::Delete } });
		tickAll();
		report.Check(readoutId != 0 && player.GetModuleLoadout().FindInventoryItem(readoutId) != nullptr, "Delete uninstalls the held module");

		// Closing drops the hold; a closed screen ignores input.
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
	if (argc != 2)
	{
		std::cerr << "Usage: LightYearsModuleScreenTests <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::filesystem::path artifactPath{ argv[1] };
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (!ly::GameContentBootstrap::Register()) throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		RunTests(report);
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
