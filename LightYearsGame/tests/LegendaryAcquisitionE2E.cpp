#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "presentation/hud/weaponChoice/WeaponChoicePresenter.h"

#include <algorithm>

namespace
{
	using namespace vega;
	using namespace common_module_e2e;

	constexpr char kSecondHardpoint[] = "Module.Primary.SecondHardpoint.Legendary";
	constexpr char kTwinRack[] = "Module.Utility.TwinRack.Legendary";

	struct ChoiceCounter
	{
		int requests = 0;
		void OnRequest(std::uint32_t) { ++requests; }
	};

	// Installs an inventory item through the player at the first legal cell.
	bool InstallAnywhere(Player& player, std::uint32_t instanceId)
	{
		const auto rules = player.BuildModuleRules();
		if (!rules) return false;
		for (int rotation = 0; rotation < 4; ++rotation)
			for (int y = 0; y < rules->schematic.height; ++y)
				for (int x = 0; x < rules->schematic.width; ++x)
					if (player.TryInstallModule(instanceId, { x, y }, rotation) == ModulePlacementFailure::None) return true;
		return false;
	}

	void SecondHardpointChoice(Report& report, Application& application)
	{
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		PlayerManager::GetPlayerManager().Reset();
		Player& player = PlayerManager::GetPlayerManager().CreateNewPlayer();
		player.AddLifeCount(3);
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr, "the run player spawns its ship");
		level->TickInternal(0.f);

		// The options: every player Primary weapon except the ship's own.
		const std::string& ownWeapon = content::ShipContentCatalog::GetPlayerShipDefinition(player.GetShipId()).primaryWeaponId;
		List<std::string> expected;
		for (const std::string& shipId : content::ShipContentCatalog::GetPlayerShipIds())
		{
			const std::string& weaponId = content::ShipContentCatalog::GetPlayerShipDefinition(shipId).primaryWeaponId;
			if (weaponId != ownWeapon && std::find(expected.begin(), expected.end(), weaponId) == expected.end()) expected.push_back(weaponId);
		}
		const List<std::string> options = player.GetSecondaryWeaponOptions();
		report.Check(!options.empty() && options == expected, "options are every player Primary weapon except the ship's own", Json(options));

		// Entering the inventory asks for the choice at once; nothing else does.
		ChoiceCounter counter;
		player.onSecondaryWeaponChoiceRequested.BindAction(&counter, &ChoiceCounter::OnRequest);
		const auto other = player.AddModuleToInventory("Module.Utility.TwinRack.Legendary", 0, ModuleRarity::Legendary);
		report.Check(other != 0 && counter.requests == 0 && player.GetPendingSecondaryWeaponChoices().empty(), "other modules never ask for a weapon");
		const auto hardpoint = player.AddModuleToInventory(kSecondHardpoint, 0, ModuleRarity::Legendary);
		report.Check(hardpoint != 0 && counter.requests == 1 && player.GetPendingSecondaryWeaponChoices() == List<std::uint32_t>{ hardpoint }, "Second Hardpoint asks for its weapon when it enters the inventory");

		// The HUD presenter shows the pending choice with readable labels.
		WeaponChoicePresenter presenter;
		presenter.Tick(0.f);
		const auto viewModel = presenter.GetViewModel();
		report.Check(viewModel->open && viewModel->moduleInstanceId == hardpoint && viewModel->options.size() == options.size(), "the weapon choice screen opens with every option");
		report.Check(WeaponChoicePresenter::MakeWeaponLabel("Weapon.Projectile.FighterRapidLaser.Basic") == "Fighter Rapid Laser", "weapon labels are readable", WeaponChoicePresenter::MakeWeaponLabel("Weapon.Projectile.FighterRapidLaser.Basic"));

		// The own weapon and unknown ids are refused; a listed weapon is stored on the module.
		report.Check(!player.ChooseSecondaryWeapon(hardpoint, ownWeapon) && !player.ChooseSecondaryWeapon(hardpoint, "Weapon.Unknown") && !player.GetPendingSecondaryWeaponChoices().empty(), "the ship's own weapon is not an option");
		report.Check(!presenter.Choose(options.size()), "an out-of-range row picks nothing");
		report.Check(presenter.Choose(1), "picking a row chooses that weapon");
		report.Check(!viewModel->open && player.GetPendingSecondaryWeaponChoices().empty(), "the screen closes after the choice");
		const ModulePlacement* item = player.GetModuleLoadout().FindInventoryItem(hardpoint);
		report.Check(item && item->secondaryWeaponId == options[1], "the module keeps the chosen weapon", item ? item->secondaryWeaponId : std::string{});
		report.Check(!player.ChooseSecondaryWeapon(hardpoint, options[0]), "a made choice is final");

		// Installing the module gives the ship that second weapon; it survives uninstall and reinstall.
		const auto& runtime = ship->GetModuleApplier().GetLegendarySecondHardpointRuntime();
		report.Check(InstallAnywhere(player, hardpoint), "install the Second Hardpoint");
		report.Check(runtime.GetSecondWeaponId() == options[1] && runtime.GetSecondWeapon() != nullptr, "the installed module arms the chosen second weapon", runtime.GetSecondWeaponId());
		report.Check(player.TryUninstallModule(hardpoint) && runtime.GetSecondWeaponId().empty(), "uninstall disarms it");
		report.Check(InstallAnywhere(player, hardpoint) && runtime.GetSecondWeaponId() == options[1], "reinstall arms the same weapon");

		// Every new Second Hardpoint asks again.
		const auto second = player.AddModuleToInventory(kSecondHardpoint, 0, ModuleRarity::Legendary);
		presenter.Tick(0.f);
		report.Check(counter.requests == 2 && viewModel->open && viewModel->moduleInstanceId == second, "a second Second Hardpoint asks for its own weapon");
		PlayerManager::GetPlayerManager().Reset();
	}

	void TwinRackAcquisition(Report& report, Application& application)
	{
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		PlayerManager::GetPlayerManager().Reset();
		Player& player = PlayerManager::GetPlayerManager().CreateNewPlayer();
		player.AddLifeCount(3);
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr, "the run player spawns its ship");
		level->TickInternal(0.f);
		auto& loadout = ship->GetAbilityLoadout();
		const sas::AbilitySlot slots[]{ sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };
		report.Check(std::all_of(std::begin(slots), std::end(slots), [&loadout](sas::AbilitySlot slot) { return loadout.GetLoadout().FindAbility(slot) != nullptr; }), "the default kit fills all four slots");

		// Without Twin Rack a found ability only enters the inventory.
		std::string reason;
		report.Check(loadout.AcquireAbility("Ability.Defense.Shield.Basic", &reason) && loadout.GetLoadout().FindSecondAbility(sas::AbilitySlot::Ability1) == nullptr, "without Twin Rack a found ability stays in the inventory", reason);

		const auto rack = player.AddModuleToInventory(kTwinRack, 0, ModuleRarity::Legendary);
		report.Check(InstallAnywhere(player, rack) && ship->GetAbilitySystemComponent().IsTwinRackEnabled(), "install Twin Rack");

		// With all four slots full, found abilities fill the second set in Q, E, F, R order.
		const char* found[]{ "Ability.Defense.Cryostasis.Basic", "Ability.Defense.CrystalBarricade.Basic", "Ability.Defense.DirectionalBarrier.Basic", "Ability.Defense.IroncladProtocol.Basic" };
		for (std::size_t index = 0; index < 4; ++index)
		{
			report.Check(loadout.AcquireAbility(found[index], &reason), "acquire a found ability", reason);
			const std::string* second = loadout.GetLoadout().FindSecondAbility(slots[index]);
			report.Check(second && *second == found[index], "a found ability takes the next free second slot", second ? *second : std::string{});
		}
		// The second set is full: a fifth find stays in the inventory.
		report.Check(loadout.AcquireAbility("Ability.Defense.ReturnProtocol.Basic", &reason) && *loadout.GetLoadout().FindSecondAbility(sas::AbilitySlot::Ability1) == found[0], "a full second set leaves new finds in the inventory", reason);
		PlayerManager::GetPlayerManager().Reset();
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "register shipped game content");
		std::string title = "Legendary acquisition E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		SecondHardpointChoice(report, application);
		TwinRackAcquisition(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryAcquisitionE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
