#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/vitals/VitalsPresenter.h"
#include "presentation/hud/vitals/VitalsView.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
	using Json = nlohmann::json;

	class HeadlessVitalsLevel final : public ly::GameLevel
	{
	public:
		explicit HeadlessVitalsLevel(ly::Application* application) : GameLevel{ application } {}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	struct PlayerManagerResetGuard
	{
		ly::PlayerManager& manager;
		~PlayerManagerResetGuard() { manager.Reset(); }
	};

	struct Report
	{
		Json artifact{ { "test", "VitalsHUD" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, const Json& actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = actual;
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}
	};

	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.001f;
	}

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open VitalsHUD artifact path" };
		output << artifact.dump(2) << '\n';
	}

	void RunTests(Report& report)
	{
		using namespace ly;
		VitalsViewModel colorModel;
		colorModel.healthMax = 0.f;
		report.Check(ComputeHealthBarColor(colorModel) == sf::Color{ 255, 0, 0, 255 }, "zero maximum health is red");
		colorModel.healthMax = 100.f;
		colorModel.displayHealthMax = 125.f;
		report.Check(ComputeHealthBarColor(colorModel) == sf::Color{ 80, 160, 255, 255 }, "overcap health is blue");
		colorModel.displayHealthMax = 100.f;
		colorModel.health = 100.f;
		report.Check(ComputeHealthBarColor(colorModel) == sf::Color{ 0, 255, 0, 255 }, "full health is green");
		colorModel.health = 50.f;
		report.Check(ComputeHealthBarColor(colorModel) == sf::Color{ 255, 255, 0, 255 }, "half health is yellow");
		colorModel.health = 0.f;
		report.Check(ComputeHealthBarColor(colorModel) == sf::Color{ 255, 0, 0, 255 }, "empty health is red");

		std::string title = "Vitals HUD E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessVitalsLevel>(&application);
		Player& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(ship), "shipped player ship spawned");
		level->BeginPlayInternal();
		level->TickInternal(0.f);

		auto mutableModel = std::make_shared<VitalsViewModel>();
		VitalsPresenter presenter;
		presenter.Tick();
		const auto initial = presenter.GetViewModel();
		report.Check(initial->hasShip, "presenter binds the current ship");
		report.Check(NearlyEqual(initial->health, ship->GetHealthComponent().GetHealth()) &&
			NearlyEqual(initial->healthMax, ship->GetHealthComponent().GetMaxHealth()), "view model matches shipped health",
			{ { "health", initial->health }, { "healthMax", initial->healthMax } });
		report.Check(NearlyEqual(initial->shield, ship->GetShieldComponent().GetShield()) &&
			NearlyEqual(initial->shieldMax, ship->GetShieldComponent().GetMaxShield()), "view model matches shipped shield",
			{ { "shield", initial->shield }, { "shieldMax", initial->shieldMax } });
		report.Check(NearlyEqual(initial->energy, ship->GetEnergyComponent().GetEnergy()) &&
			NearlyEqual(initial->energyMax, ship->GetEnergyComponent().GetMaxEnergy()), "view model matches shipped energy",
			{ { "energy", initial->energy }, { "energyMax", initial->energyMax } });

		const float healthBeforeDamage = initial->health;
		const UIRevision beforeDamage = initial->revision;
		constexpr float damageAmount = 200.f;
		ship->SetInvulnerability(false);
		ship->ApplyDamage(damageAmount);
		presenter.Tick();
		const auto damaged = presenter.GetViewModel();
		report.Check(damaged->health < healthBeforeDamage && NearlyEqual(damaged->health, ship->GetHealthComponent().GetHealth()) &&
			NearlyEqual(damaged->displayHealth, damaged->health), "damage updates health and display health",
			{ { "damage", damageAmount }, { "healthBeforeDamage", healthBeforeDamage },
				{ "health", damaged->health }, { "displayHealth", damaged->displayHealth } });
		report.Check(damaged->revision.Get() > beforeDamage.Get(), "damage advances revision",
			{ { "before", beforeDamage.Get() }, { "after", damaged->revision.Get() } });
		const UIRevision stableRevision = damaged->revision;
		presenter.Tick();
		presenter.Tick();
		report.Check(presenter.GetViewModel()->revision.Get() == stableRevision.Get(), "unchanged polls keep revision stable");

		ship->Destroy();
		level->TickInternal(0.f);
		presenter.Tick();
		const auto destroyed = presenter.GetViewModel();
		report.Check(!destroyed->hasShip && destroyed->health == 0.f && destroyed->healthMax == 1.f &&
			destroyed->displayHealth == 0.f && destroyed->displayHealthMax == 1.f && destroyed->shield == 0.f &&
			destroyed->shieldMax == 1.f && destroyed->energy == 0.f && destroyed->energyMax == 1.f,
			"destroyed ship clears all bars to 0/1",
			{ { "hasShip", destroyed->hasShip }, { "health", destroyed->health }, { "healthMax", destroyed->healthMax },
				{ "displayHealth", destroyed->displayHealth }, { "displayHealthMax", destroyed->displayHealthMax },
				{ "shield", destroyed->shield }, { "shieldMax", destroyed->shieldMax }, { "energy", destroyed->energy },
				{ "energyMax", destroyed->energyMax } });
		auto replacementShip = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(replacementShip), "player respawns a replacement ship");
		level->TickInternal(0.f);
		presenter.Tick();
		const auto respawned = presenter.GetViewModel();
			report.Check(respawned->hasShip && NearlyEqual(respawned->health, replacementShip->GetHealthComponent().GetHealth()) &&
			NearlyEqual(respawned->shield, replacementShip->GetShieldComponent().GetShield()) &&
			NearlyEqual(respawned->energy, replacementShip->GetEnergyComponent().GetEnergy()),
			"respawn binds new ship values without old ship values",
			{ { "health", respawned->health }, { "shield", respawned->shield }, { "energy", respawned->energy } });

		player.AddLifeCount(2);
		player.AddScore(31);
		const auto status = presenter.GetViewModel();
		report.Check(status->hasPlayer && status->life == player.GetLifeCount() && status->score == player.GetScore(),
			"life and score delegates update view model");
		manager.Reset();
		report.Check(!presenter.GetViewModel()->hasPlayer && presenter.GetViewModel()->life == 0 && presenter.GetViewModel()->score == 0,
			"player removal clears life and score before destruction");
		Player& replacementPlayer = manager.CreateNewPlayer();
		presenter.Tick();
		replacementPlayer.AddLifeCount(1);
		replacementPlayer.AddScore(7);
		report.Check(presenter.GetViewModel()->hasPlayer && presenter.GetViewModel()->life == replacementPlayer.GetLifeCount() &&
			presenter.GetViewModel()->score == replacementPlayer.GetScore(), "replacement player receives fresh subscriptions");

		mutableModel->revision = UIRevision{};
		VitalsView view{ std::shared_ptr<const VitalsViewModel>{ mutableModel } };
		view.Tick(0.f);
		report.Check(view.GetRefreshCount() == 1, "first view tick applies initial values");
		view.Tick(0.f);
		view.Tick(0.f);
		report.Check(view.GetRefreshCount() == 1, "unchanged view ticks do not refresh widgets");
		SetIfChanged(mutableModel->health, 45.f, mutableModel->revision);
		SetIfChanged(mutableModel->life, 9u, mutableModel->revision);
		SetIfChanged(mutableModel->score, 123u, mutableModel->revision);
		view.Tick(0.f);
		report.Check(view.GetRefreshCount() == 2, "multiple changes coalesce into one view refresh");
		auto shortLivedPresenter = std::make_unique<VitalsPresenter>();
		shortLivedPresenter->Tick();
		VitalsView retainedView{ shortLivedPresenter->GetViewModel() };
		retainedView.Tick(0.f);
		shortLivedPresenter.reset();
		retainedView.Tick(0.f);
		report.Check(retainedView.GetRefreshCount() == 1,
			"view safely ticks after presenter destruction while retaining const shared model");
		const UIRect viewport{ { 0.f, 0.f }, { 1280.f, 720.f } };
		view.ResolveLayoutTree(viewport, true);
		const UIRect panelRect = view.GetResolvedRect();
		const auto& childWidgets = view.GetChildren();
		const auto composition = std::dynamic_pointer_cast<StackPanel>(childWidgets.front());
		const auto gaugeWidgets = composition->GetChildren();
		const auto gauges = std::dynamic_pointer_cast<StackPanel>(gaugeWidgets.front());
		const UIRect healthRect = gauges->GetChildren().back()->GetResolvedRect();
		report.artifact["inputs"] = { { "viewport", { 1280, 720 } }, { "damage", damageAmount } };
		report.Check(panelRect.size.x == 510.f && panelRect.size.y == 92.f &&
			NearlyEqual(panelRect.position.x, 20.f) && NearlyEqual(panelRect.position.y, 608.f),
			"bottom-left vitals layout resolves inside configured viewport",
			{ { "position", { panelRect.position.x, panelRect.position.y } },
				{ "size", { panelRect.size.x, panelRect.size.y } } });
		report.Check(NearlyEqual(healthRect.position.y + healthRect.size.y, 700.f),
			"health gauge bottom edge respects 20px viewport inset",
			{ { "position", { healthRect.position.x, healthRect.position.y } },
				{ "size", { healthRect.size.x, healthRect.size.y } } });
		report.artifact["passed"] = true;
	}
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: LightYearsVitalsHUDTests <artifact-path>\n";
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
		std::cerr << "VitalsHUD tests failed: " << exception.what() << '\n';
	}
	try
	{
		WriteArtifact(artifactPath, report.artifact);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write VitalsHUD artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(artifactPath).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
