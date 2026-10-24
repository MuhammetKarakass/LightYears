#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/ModuleResolver.h"

namespace
{
	using namespace uncommon_p6_e2e;
	void PoolChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; auto& shield = f.ship->GetShieldComponent(); report.Check(f.ship->GetShipRuntime().SetBaseMaxShieldContribution("Test.Polar.Capacity", 400.f - shield.GetMaxShield()), "test capacity uses the canonical ShipRuntime owner"); shield.ChangeShield(400.f); f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			const float capacityBefore = shield.GetMaxShield(); const auto id = f.Install("Module.Defense.PolarShield.Rare", 0, level); const float effective = static_cast<float>(level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id)); f.Equal("Polar Shield adds its level bonus (+5/+10/+15% proposal) to total normal capacity", capacityBefore * (1.f + .05f * effective), shield.GetMaxShield()); const auto front = Enemy(f), rear = Enemy(f); const auto origin = f.ship->GetActorLocation(), facing = f.ship->GetActorForwardDirection(); front->SetActorLocation(origin + facing * 500.f); rear->SetActorLocation(origin - facing * 500.f); const float health = f.ship->GetHealthComponent().GetHealth();
			f.Equal("front capacity is half", 200.f, shield.GetFrontShield()); f.Equal("rear capacity is half", 200.f, shield.GetRearShield());
			Hit(*f.ship, front.get(), 50.f, Plain()); f.Equal("front hit spends only front pool", 150.f, shield.GetFrontShield()); f.Equal("front hit preserves rear pool", 200.f, shield.GetRearShield());
			Hit(*f.ship, rear.get(), 75.f, Plain()); f.Equal("rear hit preserves front pool", 150.f, shield.GetFrontShield()); f.Equal("rear hit spends rear pool", 125.f, shield.GetRearShield());
			Hit(*f.ship, front.get(), 200.f, Plain()); f.Equal("empty front cannot borrow rear shield", 125.f, shield.GetShield()); f.Equal("uncovered damage reaches hull", health - 50.f, f.ship->GetHealthComponent().GetHealth());
			Hit(*f.ship, front.get(), 10.f, Plain()); f.Equal("subsequent front hit still exposes hull", health - 60.f, f.ship->GetHealthComponent().GetHealth()); f.Equal("rear remains available independently", 125.f, shield.GetRearShield());
			shield.ClearRechargeDelay(); shield.Tick(1.f, 30.f); f.Equal("recharge fills lower front first", 30.f, shield.GetFrontShield()); f.Equal("recharge does not fill higher rear first", 125.f, shield.GetRearShield());
			shield.ChangeShield(100.f); f.Equal("positive grants equalize then share remaining refill", 127.5f, shield.GetFrontShield()); f.Equal("refill maintains independent caps", 127.5f, shield.GetRearShield());
			Hit(*f.ship, rear.get(), 25.f, Plain()); shield.ClearRechargeDelay(); shield.Tick(1.f, 50.f); f.Equal("rear deficit receives refill first", 140.f, shield.GetRearShield()); f.Equal("remaining regen divides equally", 140.f, shield.GetFrontShield());
			const float before = shield.GetShield(); f.Remove(id); f.Equal("uninstall cannot create shield", before, shield.GetShield()); report.Check(!shield.GetModuleBehavior().frontRearSplit, "uninstall restores scalar shield behavior");
		}
	}
	void SharedLayerChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; auto& shield = f.ship->GetShieldComponent(); report.Check(f.ship->GetShipRuntime().SetBaseMaxShieldContribution("Test.Polar.Capacity", 400.f - shield.GetMaxShield()), "test capacity uses the canonical ShipRuntime owner"); shield.ChangeShield(400.f); f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f); f.Install("Module.Defense.PolarShield.Rare"); const float capacity = shield.GetMaxShield();
		const auto front = Enemy(f), rear = Enemy(f); const auto origin = f.ship->GetActorLocation(), facing = f.ship->GetActorForwardDirection(); front->SetActorLocation(origin + facing * 500.f); rear->SetActorLocation(origin - facing * 500.f);
		Hit(*f.ship, front.get(), 200.f, Plain()); const float health = f.ship->GetHealthComponent().GetHealth(); shield.GrantModuleTemporaryShield("Test.SharedLayer", 80.f, 6.f); Hit(*f.ship, front.get(), 60.f, Plain()); f.Equal("shared temporary layer protects empty front", health, f.ship->GetHealthComponent().GetHealth()); f.Equal("temporary layer is consumed first", 20.f, shield.GetModuleTemporaryShield()); f.Equal("temporary layer leaves rear normal pool untouched", 200.f, shield.GetRearShield());
		Hit(*f.ship, rear.get(), 40.f, Plain()); f.Equal("rear also uses the same temporary layer", 0.f, shield.GetModuleTemporaryShield()); f.Equal("only remaining rear damage spends normal rear shield", 180.f, shield.GetRearShield());
		shield.SetMaxShield(800.f, true); f.Equal("capacity scaling preserves empty front", 0.f, shield.GetFrontShield()); f.Equal("percent-preserving capacity change scales rear", 180.f * 800.f / capacity, shield.GetRearShield()); shield.SetMaxShield(200.f); f.Equal("shrinking capacity clamps each pool independently", 100.f, shield.GetRearShield()); f.Equal("capacity shrink cannot transfer shield into front", 0.f, shield.GetFrontShield());
		shield.SetMaxShield(400.f); shield.ChangeShield(400.f); shield.GrantTemporaryOvershield("Test.LegacyLayer", 80.f, 0.f, 1.f); Hit(*f.ship, front.get(), 250.f, Plain()); f.Equal("legacy overshield remains a shared overcap layer", 30.f, shield.GetFrontShield()); f.Equal("legacy layer protects rear capacity", 200.f, shield.GetRearShield());
		f.Install("Module.Utility.ZoneCooler.Basic"); f.Equal("reapply cannot reset depleted front", 30.f, shield.GetFrontShield());
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
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"); if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Polar Shield content registration failed" };
		const auto* module = content::ModuleContentCatalog::FindById("Module.Defense.PolarShield.Rare"); report.Check(module && module->sizeVariants.size() == 1 && module->sizeVariants.front().shape.cells.size() == 2 && module->sideFeatures.empty() && module->sharedPools.empty(), "Polar remains fixed two cells with no sides");
		std::string title = "Polar Shield E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false); PoolChecks(report, application); SharedLayerChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModulePolarShieldE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
