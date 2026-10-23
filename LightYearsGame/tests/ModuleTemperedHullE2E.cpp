#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "player/PlayerManager.h"

namespace
{
	using namespace uncommon_p6_e2e;
	GameAbility& Defense(Fixture& f)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		auto* ability = asc.GetAbilityById("Ability.Defense.DirectionalBarrier.Basic");
		if (!ability)
		{
			if (auto* previous = asc.GetAbility(sas::AbilitySlot::Ability1)) f.report.Check(asc.RemoveAbility(previous->GetHandle()), "replace test Q without editing default loadout");
			auto definition = *content::AbilityContentCatalog::FindById("Ability.Defense.DirectionalBarrier.Basic"); definition.slot = sas::AbilitySlot::Ability1;
			f.report.Check(asc.GrantAbility(definition).IsValid(), "grant a shipped Defense ability");
			ability = asc.GetAbilityById("Ability.Defense.DirectionalBarrier.Basic");
		}
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		ability->Tick(100.f);
		return *ability;
	}
	void CastDefense(Fixture& f) { auto& ability = Defense(f); f.report.Check(ability.TryActivate(), "activate real Defense ability"); if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed); }

	struct CounterBinding
	{
		Fixture& f;
		explicit CounterBinding(Fixture& fixture) : f{ fixture } { f.ship->GetModuleApplier().SetRunCounterCreditSink([this](std::uint32_t id, double units) { f.report.Check(f.loadout.AddRunCounterCredit(id, units), "counter is credited to its inventory owner"); }); }
		~CounterBinding() { f.ship->GetModuleApplier().SetRunCounterCreditSink({}); }
	};
	void PlayerCounterChecks(Report& report, Application& application)
	{
		auto& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		struct ResetGuard { ~ResetGuard() { PlayerManager::GetPlayerManager().Reset(); } } guard;
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		auto& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr && player.GetWeakPtr().expired(), "counter owner is a real manager-owned Player stored by value");
		level->TickInternal(0.f);
		const auto rules = player.BuildModuleRules();
		report.Check(rules.has_value(), "manager Player has module placement rules");
		const auto id = player.AddModuleToInventory("Module.Defense.TemperedHull.Rare", 0, ModuleRarity::Rare, 1);
		bool installed = false;
		for (int rotation = 0; rotation < 4 && !installed; ++rotation)
			for (int y = 0; y < rules->schematic.height && !installed; ++y)
				for (int x = 0; x < rules->schematic.width && !installed; ++x)
					installed = player.TryInstallModule(id, { x, y }, rotation) == ModulePlacementFailure::None;
		report.Check(id != 0 && installed, "Tempered Hull installs through the Player mutation path");
		const auto placement = *player.GetModuleLoadout().FindPlacement(id);
		const auto cast = [&report](PlayerSpaceShip& owner)
		{
			auto& asc = owner.GetAbilitySystemComponent();
			asc.ClearAbilitySlot(sas::AbilitySlot::Ability1);
			auto definition = *content::AbilityContentCatalog::FindById("Ability.Defense.DirectionalBarrier.Basic");
			definition.slot = sas::AbilitySlot::Ability1;
			const auto handle = asc.GrantAbility(definition);
			auto* ability = asc.GetAbility(handle);
			report.Check(ability && ability->TryActivate(), "manager ship activates a real Defense ability");
			if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		};
		cast(*ship);
		report.Check(player.GetModuleLoadout().GetRunCounter(id) == 1.0, "Defense use credits the Player loadout counter");
		report.Check(player.TryUninstallModule(id) && player.TryInstallModule(id, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None,
			"Player uninstalls and reapplies the same module");
		report.Check(player.GetModuleLoadout().GetRunCounter(id) == 1.0, "counter survives loadout reapply");
		cast(*ship);
		report.Check(player.GetModuleLoadout().GetRunCounter(id) == 2.0, "counter continues from saved credit after reapply");
		ship->Destroy();
		ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr && player.GetModuleLoadout().GetRunCounter(id) == 2.0, "counter survives respawn");
		level->TickInternal(0.f);
		cast(*ship);
		report.Check(player.GetModuleLoadout().GetRunCounter(id) == 3.0, "respawned ship credits the same Player loadout");
		manager.Reset();
		auto& replacement = manager.CreateNewPlayer();
		cast(*ship);
		report.Check(replacement.GetModuleLoadout().GetRevision() == 0, "retained old ship cannot credit a replacement Player");
	}
	void ArmorChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; CounterBinding binding{ f }; const float baseline = f.Owner(OwnerAttributeIds::Armor);
			const auto id = f.Install("Module.Defense.TemperedHull.Rare", 0, level); const int cap = 20 + 10 * level;
			CastOffense(f); f.Equal("Offense does not accumulate Armor", baseline, f.Owner(OwnerAttributeIds::Armor));
			for (int count = 1; count <= cap + 2; ++count) { CastDefense(f); f.Equal("Armor advances only up to level cap", baseline + static_cast<float>(std::min(count, cap)), f.Owner(OwnerAttributeIds::Armor)); }
			f.Equal("persistent counter stops at cap", static_cast<float>(cap), static_cast<float>(f.loadout.FindPlacement(id)->runCounter));
			const auto placement = *f.loadout.FindPlacement(id); f.Remove(id); f.Equal("uninstall removes the active Armor", baseline, f.Owner(OwnerAttributeIds::Armor));
			report.Check(f.loadout.Install(f.rules, id, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "reinstall same run-owned module");
			f.Equal("reinstall retains accumulated Armor", baseline + cap, f.Owner(OwnerAttributeIds::Armor));
		}
		Fixture fresh{ report, application }; const float baseline = fresh.Owner(OwnerAttributeIds::Armor); fresh.Install("Module.Defense.TemperedHull.Rare"); fresh.Equal("fresh run starts without accumulated Armor", baseline, fresh.Owner(OwnerAttributeIds::Armor));
	}
	void BastionChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; CounterBinding binding{ f }; const auto near = Enemy(f, 100.f), outside = Enemy(f, 400.f); DamageObserver observer{ *f.ship };
		f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 80.f);
		const auto id = f.Install("Module.Defense.TemperedHull.Rare", 1, 1, "Module.SideFeature.TemperedHull.BastionSlam");
		CastDefense(f); f.Equal("Bastion uses accumulated bonus Armor including this activation", 998.5f, near->GetHealthComponent().GetHealth());
		f.Equal("Bastion excludes base Armor and respects radius", 1000.f, outside->GetHealthComponent().GetHealth());
		CastDefense(f); f.Equal("another cast cannot reset Bastion cooldown", 998.5f, near->GetHealthComponent().GetHealth());
		f.ship->GetModuleApplier().Tick(4.99f); CastDefense(f); f.Equal("Bastion is still blocked just before five seconds", 998.5f, near->GetHealthComponent().GetHealth());
		f.ship->GetModuleApplier().Tick(.02f); CastDefense(f); f.Equal("Bastion fires with current accumulated Armor after five seconds", 992.5f, near->GetHealthComponent().GetHealth());
		report.Check(observer.generated == 2 && observer.allSuppressed, "Bastion damage cannot recursively trigger module effects");
		f.Remove(id); f.ship->GetModuleApplier().Tick(5.f); CastDefense(f); f.Equal("uninstall ends Bastion", 992.5f, near->GetHealthComponent().GetHealth());
		Fixture small{ report, application }; small.Install("Module.Defense.TemperedHull.Rare", 0, 1, "Module.SideFeature.TemperedHull.BastionSlam"); const auto enemy = Enemy(small); CastDefense(small);
		small.Equal("two-cell variant keeps Bastion locked", 1000.f, enemy->GetHealthComponent().GetHealth());
	}
	void MedicChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; CounterBinding binding{ f }; auto& health = f.ship->GetHealthComponent(); health.SetMaxHealth(250.f); health.Regenerate(250.f); health.ChangeHealth(-100.f);
			const auto id = f.Install("Module.Defense.TemperedHull.Rare", 1, level, "Module.SideFeature.TemperedHull.FieldMedic"); CastDefense(f);
			f.Equal("Field Medic does not heal immediately", 150.f, health.GetHealth()); f.ship->GetModuleApplier().Tick(1.f); f.Equal("one second heals four percent", 160.f, health.GetHealth());
			CastDefense(f); f.ship->GetModuleApplier().Tick(2.f); f.Equal("repeat cast preserves original twelve percent total", 180.f, health.GetHealth());
			CastDefense(f); f.ship->GetModuleApplier().Tick(26.9f); f.Equal("Medic cannot restart during thirty-second cooldown", 180.f, health.GetHealth());
			CastDefense(f); f.ship->GetModuleApplier().Tick(.11f); f.Equal("cooldown boundary does not heal on its own", 180.f, health.GetHealth());
			CastDefense(f); f.ship->GetModuleApplier().Tick(1.f); f.Equal("Medic can activate again after thirty seconds", 190.f, health.GetHealth());
			f.Remove(id); f.ship->GetModuleApplier().Tick(3.f); f.Equal("uninstall cancels remaining healing", 190.f, health.GetHealth());
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
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Tempered Hull content registration failed" };
		std::string title = "Tempered Hull E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ArmorChecks(report, application); BastionChecks(report, application); MedicChecks(report, application); PlayerCounterChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleTemperedHullE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
