#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/generated/ModuleSpeedLanceActor.h"

namespace
{
	using namespace uncommon_p6_e2e;
	shared_ptr<SpaceShip> Target(Fixture& f, float distance = 50.f) { const auto enemy = Enemy(f, distance); enemy->SetCollisionMask(CollisionLayer::PlayerBullet); return enemy; }
	void Sample(Fixture& f)
	{
		Move(f, 25.f, .25f); Move(f, 125.f, .25f); Move(f, 50.f, .25f); Move(f, 0.f, .25f);
	}
	shared_ptr<ModuleSpeedLanceActor> Lance(Fixture& f)
	{
		const auto actors = Actors<ModuleSpeedLanceActor>(f); f.report.Check(actors.size() == 1, "one Movement use produces one lance at window end"); return actors.front();
	}
	void MainChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; DamageObserver observer{ *f.ship }; const auto id = f.Install("Module.Movement.SpeedLance.Rare", 0, level);
			CastOffense(f); Sample(f); report.Check(Actors<ModuleSpeedLanceActor>(f).empty(), "Offense cannot open a lance window");
			CastMovement(f); Move(f, 25.f, .25f); Move(f, 125.f, .25f); Move(f, 50.f, .25f);
			report.Check(Actors<ModuleSpeedLanceActor>(f).empty(), "lance waits for the full one-second sample"); Move(f, 0.f, .25f); const auto lance = Lance(f);
			f.Equal("damage uses peak speed rather than final speed", 200.f, lance->GetDamage()); f.Equal("900 range at 1800 speed lasts half a second", .5f, lance->GetLifeTime());
			for (int index = 0; index < 8; ++index)
			{
				const auto enemy = Target(f, 50.f + index * 50.f); lance->OnActorBeginOverlap(enemy.get()); f.Equal("all pierced targets take full damage", 800.f, enemy->GetHealthComponent().GetHealth());
				lance->OnActorBeginOverlap(enemy.get()); f.Equal("each target can be hit only once per lance", 800.f, enemy->GetHealthComponent().GetHealth());
			}
			report.Check(observer.generated == 8 && observer.allSuppressed, "lance hits suppress module recursion and critical rolls");
			const auto origin = lance->GetActorLocation(); lance->Tick(.6f); const auto distance = lance->GetActorLocation() - origin;
			f.Equal("flight clamps at selected 900 range", 900.f, std::sqrt(distance.x * distance.x + distance.y * distance.y)); report.Check(lance->GetIsPendingDestroy(), "range completion ends lance");
			CastMovement(f); f.Remove(id); Sample(f); report.Check(Actors<ModuleSpeedLanceActor>(f).empty(), "uninstall cancels an in-progress speed window");
		}
	}
	void SweepChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; f.Install("Module.Movement.SpeedLance.Rare"); CastMovement(f); Sample(f); const auto lance = Lance(f); const auto origin = lance->GetActorLocation(), direction = lance->GetActorForwardDirection();
		List<shared_ptr<SpaceShip>> targets; for (float distance : { 100.f, 400.f, 850.f, 950.f }) { const auto target = Target(f); target->SetActorLocation(origin + direction * distance); targets.push_back(target); }
		lance->Tick(.6f); for (std::size_t i = 0; i < targets.size(); ++i) f.Equal("swept collision pierces in-range targets without tunneling", i < 3 ? 800.f : 1000.f, targets[i]->GetHealthComponent().GetHealth());
	}
	void SideChecks(Report& report, Application& application)
	{
		Fixture echo{ report, application }; const auto id = echo.Install("Module.Movement.SpeedLance.Rare", 1, 1, "Module.SideFeature.SpeedLance.EchoLance"); CastMovement(echo); Sample(echo); const auto first = Lance(echo); const auto direction = first->GetActorForwardDirection();
		echo.ship->SetActorRotation(echo.ship->GetActorRotation() + 90.f); echo.ship->GetModuleApplier().Tick(.39f); report.Check(Actors<ModuleSpeedLanceActor>(echo).size() == 1, "Echo waits four tenths of a second"); echo.ship->GetModuleApplier().Tick(.02f);
		const auto lances = Actors<ModuleSpeedLanceActor>(echo); report.Check(lances.size() == 2, "Echo creates the second lance"); const auto second = lances.back(); echo.Equal("Echo retains half of original peak damage", 100.f, second->GetDamage()); echo.Equal("Echo retains original forward x", direction.x, second->GetActorForwardDirection().x); echo.Equal("Echo retains original forward y", direction.y, second->GetActorForwardDirection().y);
		const auto enemy = Target(echo); first->OnActorBeginOverlap(enemy.get()); second->OnActorBeginOverlap(enemy.get()); echo.Equal("each lance has its own target hit budget", 700.f, enemy->GetHealthComponent().GetHealth());
		echo.Remove(id); report.Check(first->GetIsPendingDestroy() && second->GetIsPendingDestroy(), "uninstall destroys both leased lances");
		Fixture charged{ report, application }; charged.Install("Module.Movement.SpeedLance.Rare", 1, 1, "Module.SideFeature.SpeedLance.ChargedLance"); CastMovement(charged); Sample(charged); const auto lance = Lance(charged); const auto near = Target(charged), far = Target(charged, 100.f);
		lance->OnActorBeginOverlap(near.get()); lance->OnActorBeginOverlap(far.get()); charged.Equal("Charged Lance adds two Electric to first target", 2.f, static_cast<float>(Stacks(*near, DamageStatusEffectIds::ElectricEffectId))); charged.Equal("subsequent targets receive no Electric", 0.f, static_cast<float>(Stacks(*far, DamageStatusEffectIds::ElectricEffectId))); charged.Equal("Charged keeps original first-target damage", 800.f, near->GetHealthComponent().GetHealth()); charged.Equal("Charged keeps full piercing damage", 800.f, far->GetHealthComponent().GetHealth());
		Fixture small{ report, application }; small.Install("Module.Movement.SpeedLance.Rare", 0, 1, "Module.SideFeature.SpeedLance.ChargedLance"); CastMovement(small); Sample(small); const auto clean = Target(small); Lance(small)->OnActorBeginOverlap(clean.get()); small.Equal("two-cell side remains locked", 0.f, static_cast<float>(Stacks(*clean, DamageStatusEffectIds::ElectricEffectId)));
	}
	void ContentChecks(Report& report)
	{
		report.Check(PresentationProfileRegistry<ModuleSpeedLancePresentationProfile>::Find(ModuleGeneratedPresentationIds::SpeedLance.ToString()) != nullptr, "Speed Lance concrete profile is registered before spawn");
		report.Check(PresentationProfileRegistry<ModuleGuidedPresentationProfile>::Find(ModuleGeneratedPresentationIds::SpeedLance.ToString()) == nullptr, "Speed Lance profile cannot resolve as a guided sphere");
		const auto* module = content::ModuleContentCatalog::FindById("Module.Movement.SpeedLance.Rare"); report.Check(module && module->sizeLadderCellCounts == List<int>{ 2, 3 } && module->sideFeatures.size() == 2 && module->sharedPools.front().allowedFeatureIds.size() == 3, "complete 2/3 ladder with two special and three Rare common choices");
	}
}
int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Speed Lance content registration failed" };
		std::string title = "Speed Lance E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentChecks(report); MainChecks(report, application); SweepChecks(report, application); SideChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleSpeedLanceE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
