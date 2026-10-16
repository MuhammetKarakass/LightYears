#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/progression/RunGold.h"
#include "gameplay/module/ModuleResolver.h"

namespace
{
	using namespace uncommon_p6_e2e;
	void GoldChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet); const float baseline = f.Owner(OwnerAttributeIds::Armor), cap = 45.f + 15.f * level;
			const auto id = f.Install("Module.Defense.GoldPlating.Rare", 0, level); wallet->Award(9); f.ship->GetModuleApplier().Tick(0.f); f.Equal("less than ten Gold grants no Armor", baseline, f.Owner(OwnerAttributeIds::Armor));
			wallet->Award(1); f.ship->GetModuleApplier().Tick(0.f); f.Equal("ten Gold grants one Armor", baseline + 1.f, f.Owner(OwnerAttributeIds::Armor));
			wallet->Award(10000); f.ship->GetModuleApplier().Tick(0.f); f.Equal("Gold Plating respects each Rare level cap", baseline + cap, f.Owner(OwnerAttributeIds::Armor));
			report.Check(!wallet->TrySpend(wallet->GetAmount() + 1), "Gold owner rejects overdraft"); f.ship->GetModuleApplier().Tick(0.f); f.Equal("rejected spending cannot change Armor", baseline + cap, f.Owner(OwnerAttributeIds::Armor));
			report.Check(wallet->TrySpend(wallet->GetAmount() - 20), "Gold owner spends without underflow"); f.ship->GetModuleApplier().Tick(0.f); f.Equal("spending recomputes Armor from remaining Gold", baseline + 2.f, f.Owner(OwnerAttributeIds::Armor));
			f.Remove(id); f.Equal("uninstall removes only Gold Armor", baseline, f.Owner(OwnerAttributeIds::Armor));
		}
		Fixture f{ report, application }; auto wallet = std::make_shared<RunGold>(); wallet->Award(200); f.ship->GetModuleApplier().SetGoldWallet(wallet); auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes(); attributes.SetBaseValue(OwnerAttributeIds::Armor, 80.f); attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 100.f); attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 50.f);
		f.Install("Module.Defense.GoldPlating.Rare"); const float speed = f.Owner(OwnerAttributeIds::AttackSpeed); f.Install("Module.Defense.ArmorConversion.Uncommon", 1, 1, "Module.SideFeature.ArmorConversion.RapidAlloy");
		f.Equal("Gold Armor feeds the accepted Armor Conversion", 140.f, f.Owner(OwnerAttributeIds::AttackPower)); f.Equal("Rapid Alloy includes Gold Armor", speed + 10.f, f.Owner(OwnerAttributeIds::AttackSpeed)); report.Check(wallet->TrySpend(200), "spend the synergy's Gold"); f.ship->GetModuleApplier().Tick(0.f); f.Equal("spending updates conversion without feedback", 132.f, f.Owner(OwnerAttributeIds::AttackPower)); f.Equal("spending updates Rapid Alloy", speed + 8.f, f.Owner(OwnerAttributeIds::AttackSpeed));
	}
	void HullChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto target = Enemy(f); const float before = Hit(*target, f.ship.get(), 10.f, Plain()); const auto id = f.Install("Module.Utility.StrippedHull.Rare", 0, level); const float perCell = .0125f + .0025f * level; const int empty = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			f.Equal("Uncommon Hull scales actual damage with current empty cells", before * (1.f + empty * perCell), Hit(*target, f.ship.get(), 10.f, Plain()));
			const auto filler = f.Install("Module.Utility.ZoneCooler.Basic"); const int after = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements()); report.Check(after < empty, "new equipment reduces free cells");
			f.Equal("Hull damage falls when more cells are occupied", before * (1.f + after * perCell), Hit(*target, f.ship.get(), 10.f, Plain())); f.Remove(filler); f.Remove(id); f.Equal("removing Hull restores normal hit damage", before, Hit(*target, f.ship.get(), 10.f, Plain()));
		}
	}
}
int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"); if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Gold/Hull content registration failed" };
		const auto* hull = content::ModuleContentCatalog::FindById("Module.Utility.StrippedHull.Rare"); report.Check(hull && hull->rarity == ModuleRarity::Uncommon, "stable Hull ContentId now declares Uncommon rarity");
		std::string title = "Gold and Hull E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false); GoldChecks(report, application); HullChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleGoldHullE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
