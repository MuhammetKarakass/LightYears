#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/generated/ModuleAnnularBurstActor.h"

namespace
{
	using namespace uncommon_p6_e2e;
	void CastUtility(Fixture& f)
	{
		auto* ability = f.ship->GetAbilitySystemComponent().GetAbilityById("Ability.Utility.CryoBola.Basic"); f.report.Check(ability != nullptr, "fixture uses shipped Utility Cryo Bola");
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed); ability->Tick(100.f); f.report.Check(ability->TryActivate(), "activate real Utility ability"); if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
	}
	void RingChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; DamageObserver observer{ *f.ship }; auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes(); attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 200.f); attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 2000.f);
			const auto id = f.Install("Module.Utility.ReleaseValve.Rare", 0, level); List<shared_ptr<SpaceShip>> targets; const List<float> distances{ 0.f, 599.f, 600.f, 750.f, 900.f, 901.f };
			for (float distance : distances) targets.push_back(Enemy(f, distance));
			CastOffense(f); for (const auto& target : targets) f.Equal("Offense does not fire a Utility ring", 1000.f, target->GetHealthComponent().GetHealth());
			CastUtility(f);
			for (std::size_t i = 0; i < targets.size(); ++i)
			{
				f.Equal("only the selected 600-900 band takes forty-percent EP", distances[i] >= 600.f && distances[i] <= 900.f ? 920.f : 1000.f, targets[i]->GetHealthComponent().GetHealth());
				f.Equal("Release Valve applies no displacement", distances[i], targets[i]->GetActorLocation().x - f.ship->GetActorLocation().x);
			}
			report.Check(observer.generated == 3 && observer.allSuppressed, "ring hits cannot trigger module chains or crits"); const auto actors = Actors<ModuleAnnularBurstActor>(f); report.Check(actors.size() == 1, "one Utility activation creates one annular presentation"); actors.front()->Tick(.25f);
			f.Equal("visual expiration cannot deal a second hit", 920.f, targets[3]->GetHealthComponent().GetHealth()); report.Check(actors.front()->GetIsPendingDestroy(), "annular presentation expires");
			attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 300.f); CastUtility(f); f.Equal("ring uses current EP without AP or duplicate scaling", 800.f, targets[3]->GetHealthComponent().GetHealth());
			f.Remove(id); CastUtility(f); f.Equal("uninstall ends ring damage", 800.f, targets[3]->GetHealthComponent().GetHealth());
		}
	}
	void ContentChecks(Report& report)
	{
		const auto* module = content::ModuleContentCatalog::FindById("Module.Utility.ReleaseValve.Rare"); report.Check(module && module->sizeVariants.size() == 1 && module->sizeVariants.front().shape.cells.size() == 2 && module->sideFeatures.empty() && module->sharedPools.empty(), "Release Valve remains fixed two cells with no sides");
		report.Check(PresentationProfileRegistry<ModuleAnnularBurstPresentationProfile>::Find(ModuleGeneratedPresentationIds::AnnularBurst.ToString()) != nullptr, "typed annular presentation is registered before spawn");
		report.Check(PresentationProfileRegistry<ModuleExplosionPresentationProfile>::Find(ModuleGeneratedPresentationIds::AnnularBurst.ToString()) == nullptr, "annular presentation cannot resolve as a full disk explosion");
	}
}
int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Release Valve content registration failed" };
		std::string title = "Release Valve E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentChecks(report); RingChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleReleaseValveE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
