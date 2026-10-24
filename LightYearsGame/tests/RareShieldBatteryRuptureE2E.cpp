#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"

#include "gameplay/module/ModuleResolver.h"

#include <algorithm>

namespace
{
	using namespace uncommon_p6_e2e;

	// Battery cap: 30 / 35 / 40% of Max Shield over three levels; the level table extrapolates linearly past level 3.
	float ExpectedCapFraction(int effectiveLevel) { return 0.30f + 0.05f * static_cast<float>(effectiveLevel - 1); }

	void SetMaxShield(Fixture& f, float maximum)
	{
		auto& shield = f.ship->GetShieldComponent();
		f.report.Check(f.ship->GetShipRuntime().SetBaseMaxShieldContribution("Test.ShieldBattery.Capacity", maximum - shield.GetMaxShield()), "test capacity uses the canonical ShipRuntime owner");
		shield.ChangeShield(maximum);
		f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
	}

	void GiveEnemyShield(Fixture& f, SpaceShip& enemy, float maximum)
	{
		f.report.Check(enemy.GetShipRuntime().SetBaseMaxShieldContribution("Test.ShieldRupture.Target", maximum - enemy.GetShieldComponent().GetMaxShield()), "test enemy shield uses the canonical ShipRuntime owner");
		enemy.GetShieldComponent().ChangeShield(maximum);
	}

	void BatteryChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			auto& shield = f.ship->GetShieldComponent();
			SetMaxShield(f, 400.f);
			const auto id = f.Install("Module.Defense.ShieldBattery.Rare", 0, level);
			const int effectiveLevel = level + vega::ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			const float cap = ExpectedCapFraction(effectiveLevel) * 400.f;
			f.Equal("cap follows the level table at the zone-adjusted level", cap, shield.GetBatteryCap());
			const auto enemy = Enemy(f);
			shield.ClearRechargeDelay();
			shield.TickBatteryReserve(1.f, 30.f, true);
			f.Equal("reserve fills at the regen rate while the normal shield is full", 30.f, shield.GetBatteryReserve());
			shield.TickBatteryReserve(1000.f, 30.f, true);
			f.Equal("reserve stops at its cap", cap, shield.GetBatteryReserve());
			f.Equal("reserve never enters the normal shield value", 400.f, shield.GetShield());

			const float health = f.ship->GetHealthComponent().GetHealth();
			Hit(*f.ship, enemy.get(), 50.f, Plain());
			f.Equal("reserve absorbs damage before the normal shield", cap - 50.f, shield.GetBatteryReserve());
			f.Equal("normal shield stays full while the reserve absorbs", 400.f, shield.GetShield());
			f.Equal("hull is untouched by reserve damage", health, f.ship->GetHealthComponent().GetHealth());

			const float delay = shield.GetRechargeDelayRemaining();
			report.Check(delay > 0.f, "reserve damage starts the recharge delay");
			shield.TickBatteryReserve(delay, 30.f, true);
			f.Equal("base reserve waits out the recharge delay", cap - 50.f, shield.GetBatteryReserve());
			shield.TickBatteryReserve(1.f, 30.f, true);
			f.Equal("reserve refills at the full rate after the delay", cap - 20.f, shield.GetBatteryReserve());

			Hit(*f.ship, enemy.get(), cap + 5.f, Plain());
			f.Equal("the spend empties the reserve", 0.f, shield.GetBatteryReserve());
			f.Equal("only damage beyond the reserve reaches the normal shield", 375.f, shield.GetShield());

			shield.ChangeShield(25.f);
			shield.ClearRechargeDelay();
			shield.TickBatteryReserve(1000.f, 30.f, true);
			f.Equal("reserve refills once the normal shield is full again", cap, shield.GetBatteryReserve());
			f.Remove(id);
			f.Equal("uninstall drops the reserve with its cap", 0.f, shield.GetBatteryReserve());
		}
	}

	void CapacitorChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& shield = f.ship->GetShieldComponent();
		SetMaxShield(f, 400.f);
		// Side features open only on the largest (3-cell) variant, which is variant 1.
		f.Install("Module.Defense.ShieldBattery.Rare", 1, 1, "Module.SideFeature.ShieldBattery.CapacitorFocus");
		const auto enemy = Enemy(f);
		const float baseEnergy = f.Owner(OwnerAttributeIds::EnergyPower);
		shield.ClearRechargeDelay();
		shield.TickBatteryReserve(1000.f, 30.f, true);
		const float reserve = shield.GetBatteryReserve();
		report.Check(reserve > 0.f, "Capacitor Focus test fills a reserve");
		f.ship->GetModuleApplier().Tick(0.f);
		f.Equal("Capacitor Focus adds 25% of the current reserve to Energy Power", 0.25f * reserve, f.Owner(OwnerAttributeIds::EnergyPower) - baseEnergy);
		Hit(*f.ship, enemy.get(), 40.f, Plain());
		f.ship->GetModuleApplier().Tick(0.f);
		f.Equal("Capacitor Focus shrinks as the reserve is spent", 0.25f * (reserve - 40.f), f.Owner(OwnerAttributeIds::EnergyPower) - baseEnergy);
	}

	void UnbrokenChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& shield = f.ship->GetShieldComponent();
		SetMaxShield(f, 400.f);
		f.Install("Module.Defense.ShieldBattery.Rare", 1, 1, "Module.SideFeature.ShieldBattery.UnbrokenStorage");
		const auto enemy = Enemy(f);
		const float cap = shield.GetBatteryCap();
		shield.ClearRechargeDelay();
		shield.TickBatteryReserve(1000.f, 30.f, true);
		f.Equal("reserve starts at its cap", cap, shield.GetBatteryReserve());
		Hit(*f.ship, enemy.get(), 60.f, Plain());
		const float delay = shield.GetRechargeDelayRemaining();
		report.Check(delay > 0.f, "damage starts the recharge delay");
		const float before = shield.GetBatteryReserve();
		shield.TickBatteryReserve(delay, 30.f, true);
		f.Equal("damaged fill adds 25% of the regen rate during the recharge delay", std::min(cap, before + 0.25f * 30.f * delay), shield.GetBatteryReserve());
		f.Equal("normal shield stays full through the damaged fill", 400.f, shield.GetShield());
	}

	void RuptureChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install("Module.Offense.ShieldRupture.Rare");
		const sf::Vector2f origin = f.ship->GetActorLocation();
		const auto target = Enemy(f), neighbour = Enemy(f), chained = Enemy(f);
		target->SetActorLocation(origin + sf::Vector2f{ 300.f, 0.f });
		neighbour->SetActorLocation(origin + sf::Vector2f{ 340.f, 0.f });
		chained->SetActorLocation(origin + sf::Vector2f{ 260.f, 0.f });
		const auto expandedRadiusTarget = Enemy(f), outsideRadiusTarget = Enemy(f);
		expandedRadiusTarget->SetActorLocation(origin + sf::Vector2f{ 500.f, 0.f });
		outsideRadiusTarget->SetActorLocation(origin + sf::Vector2f{ 551.f, 0.f });
		GiveEnemyShield(f, *target, 300.f);
		GiveEnemyShield(f, *chained, 100.f);

		Hit(*target, f.ship.get(), 100.f, Plain());
		f.Equal("a hit that keeps the shield up only spends shield", 200.f, target->GetShieldComponent().GetShield());
		report.Check(Actors<ModuleDelayedExplosionActor>(f).empty(), "a hit that does not break the shield spawns no blast");

		Hit(*target, f.ship.get(), 250.f, Plain());
		const auto blasts = Actors<ModuleDelayedExplosionActor>(f);
		report.Check(blasts.size() == 1, "breaking the shield spawns one Energy blast");
		for (const auto& blast : blasts) blast->Tick(0.f);
		// 50% of the 300 shield lost within the window (100 + 200) is 150, dealt to every enemy in the blast radius.
		f.Equal("blast deals 50% of the shield lost to the neighbour in radius", 850.f, neighbour->GetHealthComponent().GetHealth());
		f.Equal("250-radius blast reaches a target 200 units away", 850.f, expandedRadiusTarget->GetHealthComponent().GetHealth());
		f.Equal("250-radius blast excludes a target 251 units away", 1000.f, outsideRadiusTarget->GetHealthComponent().GetHealth());
		report.Check(chained->GetShieldComponent().GetShield() <= 0.f, "blast breaks the chained enemy's shield");
		f.Equal("blast does not chain into another blast", 850.f, neighbour->GetHealthComponent().GetHealth());

		const auto distantTarget = Enemy(f), closeNeighbour = Enemy(f);
		distantTarget->SetActorLocation(origin + sf::Vector2f{ -400.f, 0.f });
		closeNeighbour->SetActorLocation(origin + sf::Vector2f{ -360.f, 0.f });
		GiveEnemyShield(f, *distantTarget, 300.f);
		Hit(*distantTarget, f.ship.get(), 100.f, Plain());
		// The first 100 of shield loss is older than the 3 sec history window, so it no longer sizes the next blast.
		f.ship->GetModuleApplier().Tick(4.f);
		Hit(*distantTarget, f.ship.get(), 250.f, Plain());
		for (const auto& blast : Actors<ModuleDelayedExplosionActor>(f)) blast->Tick(0.f);
		f.Equal("history window forgets losses older than 3 sec: blast sized by the 200 recent loss", 900.f, closeNeighbour->GetHealthComponent().GetHealth());
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Shield Battery and Rupture content registration failed" };
		const auto* battery = content::ModuleContentCatalog::FindById("Module.Defense.ShieldBattery.Rare");
		report.Check(battery && battery->sizeVariants.size() == 2 && battery->sizeVariants[0].shape.cells.size() == 2 && battery->sizeVariants[1].shape.cells.size() == 3 && battery->sideFeatures.size() == 2 && battery->sharedPools.empty(), "Shield Battery is a 2/3-cell ladder with two private side features");
		const auto* rupture = content::ModuleContentCatalog::FindById("Module.Offense.ShieldRupture.Rare");
		report.Check(rupture && rupture->sizeVariants.size() == 1 && rupture->sizeVariants.front().shape.cells.size() == 2 && rupture->sideFeatures.empty(), "Shield Rupture is fixed two cells with no side features");
		std::string title = "Rare Shield Battery and Rupture E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		BatteryChecks(report, application);
		CapacitorChecks(report, application);
		UnbrokenChecks(report, application);
		RuptureChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "RareShieldBatteryRuptureE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
