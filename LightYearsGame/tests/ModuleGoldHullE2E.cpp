#include "framework/EngineContext.h"
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
			Fixture f{ report, application }; auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet); const float baseline = f.Owner(OwnerAttributeIds::Armor);
			const auto id = f.Install("Module.Defense.GoldPlating.Rare", 0, level); const float cap = 45.f + 15.f * (level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id)); wallet->Award(9); f.ship->GetModuleApplier().Tick(0.f); f.Equal("less than ten Gold grants no Armor", baseline, f.Owner(OwnerAttributeIds::Armor));
			wallet->Award(1); f.ship->GetModuleApplier().Tick(0.f); f.Equal("ten Gold grants one Armor", baseline + 1.f, f.Owner(OwnerAttributeIds::Armor));
			wallet->Award(10000); f.ship->GetModuleApplier().Tick(0.f); f.Equal("Gold Plating respects each Rare level cap", baseline + cap, f.Owner(OwnerAttributeIds::Armor));
			report.Check(!wallet->TrySpend(wallet->GetAmount() + 1), "Gold owner rejects overdraft"); f.ship->GetModuleApplier().Tick(0.f); f.Equal("rejected spending cannot change Armor", baseline + cap, f.Owner(OwnerAttributeIds::Armor));
			report.Check(wallet->TrySpend(wallet->GetAmount() - 20), "Gold owner spends without underflow"); f.ship->GetModuleApplier().Tick(0.f); f.Equal("spending recomputes Armor from remaining Gold", baseline + 2.f, f.Owner(OwnerAttributeIds::Armor));
			f.Remove(id); f.Equal("uninstall removes only Gold Armor", baseline, f.Owner(OwnerAttributeIds::Armor));
		}
		Fixture f{ report, application }; auto wallet = std::make_shared<RunGold>(); wallet->Award(200); f.ship->GetModuleApplier().SetGoldWallet(wallet); auto& attributes = f.ship->GetAbilitySystemComponent().GetAttributes(); attributes.SetBaseValue(OwnerAttributeIds::Armor, 80.f); attributes.SetBaseValue(OwnerAttributeIds::AttackPower, 100.f); attributes.SetBaseValue(OwnerAttributeIds::EnergyPower, 50.f);
		f.Install("Module.Defense.GoldPlating.Rare"); const float speed = f.Owner(OwnerAttributeIds::AttackSpeed); const auto conversion = f.Install("Module.Defense.ArmorConversion.Uncommon", 1, 1, "Module.SideFeature.ArmorConversion.RapidAlloy");
		const float fraction = .35f + .05f * (1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), conversion));
		f.Equal("Gold Armor feeds the accepted Armor Conversion", 100.f + 100.f * fraction, f.Owner(OwnerAttributeIds::AttackPower)); f.Equal("Rapid Alloy includes Gold Armor", speed + 10.f, f.Owner(OwnerAttributeIds::AttackSpeed)); report.Check(wallet->TrySpend(200), "spend the synergy's Gold"); f.ship->GetModuleApplier().Tick(0.f); f.Equal("spending updates conversion without feedback", 100.f + 80.f * fraction, f.Owner(OwnerAttributeIds::AttackPower)); f.Equal("spending updates Rapid Alloy", speed + 8.f, f.Owner(OwnerAttributeIds::AttackSpeed));
	}
	void GildedFrameChecks(Report& report, Application& application)
		{
			Fixture f{ report, application }; auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet);
			const float baseMaxHealth = f.Owner(OwnerAttributeIds::MaxHealth), baseArmor = f.Owner(OwnerAttributeIds::Armor);
			const auto* module = content::ModuleContentCatalog::FindById("Module.Defense.GoldPlating.Rare");
			report.Check(module && module->sizeLadderCellCounts == List<int>{ 2, 3 } && module->sideFeatures.size() == 1 && module->sideFeatures.front() == sas::ContentId{ "Module.SideFeature.GoldPlating.GildedFrame" } && module->sharedPools.size() == 3, "Gold Plating is a 2/3 ladder with private Gilded Frame and three selected shared sides");
			const auto id = f.Install("Module.Defense.GoldPlating.Rare", 1, 1, "Module.SideFeature.GoldPlating.GildedFrame");
			f.Equal("Gilded Frame adds no Max Health before Gold", baseMaxHealth, f.Owner(OwnerAttributeIds::MaxHealth));
			wallet->Award(500); f.ship->GetModuleApplier().Tick(0.f);
			f.Equal("500 Gold grants 50 Gold Armor", baseArmor + 50.f, f.Owner(OwnerAttributeIds::Armor));
			f.Equal("Gilded Frame matches this module's Gold Armor one to one", baseMaxHealth + 50.f, f.Owner(OwnerAttributeIds::MaxHealth));
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, baseArmor + 200.f); f.ship->GetModuleApplier().Tick(0.f);
			f.Equal("Armor from other sources does not feed Gilded Frame", baseMaxHealth + 50.f, f.Owner(OwnerAttributeIds::MaxHealth));
			report.Check(wallet->TrySpend(400), "Gilded Frame spends Gold through the real wallet"); f.ship->GetModuleApplier().Tick(0.f);
			f.Equal("spending recomputes Gilded Frame from remaining Gold", baseMaxHealth + 10.f, f.Owner(OwnerAttributeIds::MaxHealth));
			f.Remove(id); f.Equal("uninstall removes Gilded Frame", baseMaxHealth, f.Owner(OwnerAttributeIds::MaxHealth));
		}
		void HullChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto target = Enemy(f); const float before = Hit(*target, f.ship.get(), 10.f, Plain()); const auto id = f.Install("Module.Utility.StrippedHull.Rare", 0, level); const float perCell = .0125f + .0025f * (level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id)); const int empty = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			f.Equal("Uncommon Hull scales actual damage with current empty cells", before * (1.f + empty * perCell), Hit(*target, f.ship.get(), 10.f, Plain()));
			const auto filler = f.Install("Module.Utility.ZoneCooler.Basic"); const int after = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements()); report.Check(after < empty, "new equipment reduces free cells");
			f.Equal("Hull damage falls when more cells are occupied", before * (1.f + after * perCell), Hit(*target, f.ship.get(), 10.f, Plain())); f.Remove(filler); f.Remove(id); f.Equal("removing Hull restores normal hit damage", before, Hit(*target, f.ship.get(), 10.f, Plain()));
		}
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
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"); if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Gold/Hull content registration failed" };
		const auto* hull = content::ModuleContentCatalog::FindById("Module.Utility.StrippedHull.Rare"); report.Check(hull && hull->rarity == ModuleRarity::Uncommon, "stable Hull ContentId now declares Uncommon rarity");
		std::string title = "Gold and Hull E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false); GoldChecks(report, application); GildedFrameChecks(report, application); HullChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleGoldHullE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
