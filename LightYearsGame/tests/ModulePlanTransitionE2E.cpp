#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"

namespace
{
	using namespace uncommon_p6_e2e;
	void ContentChecks(Report& report)
	{
		for (const auto* id : { "Module.Utility.LuckyCast.Uncommon", "Module.Defense.ColdRetort.Uncommon", "Module.Defense.ShatterShock.Rare" }) report.Check(!content::ModuleContentCatalog::FindById(id), "retired main module is absent", id);
		const auto* cooler = content::ModuleContentCatalog::FindById("Module.Utility.ZoneCooler.Basic");
		report.Check(cooler && cooler->sizeVariants.front().shape.cells.size() == 2, "Zone Cooler occupies two cells");
		const auto* spare = content::ModuleContentCatalog::FindById("Module.Utility.SpareCapacitor.Rare");
		report.Check(spare && spare->sideFeatures.empty() && spare->sharedPools.empty() && spare->sizeVariants.size() == 3, "Spare keeps its ladder without side permissions");
		for (const auto* name : { "Offense", "Defense", "Utility", "Movement" })
		{
			const auto* clock = content::ModuleContentCatalog::FindById(std::string{ "Module." } + name + ".HuntClock.Uncommon");
			report.Check(clock && IsModuleSideFeatureAllowed(*clock, content::ModuleSideFeatureCatalog::FindById("Module.SideFeature.Shared.LuckyCast")), "all four Hunt clocks allow Lucky Cast");
			report.Check(!IsModuleSideFeatureAllowed(*clock, content::ModuleSideFeatureCatalog::FindById("Module.SideFeature.Shared.G5")), "Lucky replaces G5 instead of adding a sixth choice");
		}
	}
	void ArcChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto module = f.Install("Module.Offense.ArcExtender.Uncommon", 1, level, "Module.SideFeature.ArcExtender.ArcDischarge");
			const auto original = Enemy(f, 50.f), near = Enemy(f, 60.f), second = Enemy(f, 70.f), third = Enemy(f, 80.f), far = Enemy(f, 600.f);
			const float health = near->GetHealthComponent().GetHealth();
			StatusHit(*original, f.ship.get(), DamageTypeSchema::Electric, 4);
			f.Equal("four stacks do not discharge", 0.f, static_cast<float>(Stacks(*near, DamageStatusEffectIds::ElectricEffectId)));
			StatusHit(*original, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("fifth stack remains on the original target", 5.f, static_cast<float>(Stacks(*original, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("nearest target receives two Electric stacks", 2.f, static_cast<float>(Stacks(*near, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("second nearest target receives two Electric stacks", 2.f, static_cast<float>(Stacks(*second, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("third target is untouched", 0.f, static_cast<float>(Stacks(*third, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("out of range target is untouched", 0.f, static_cast<float>(Stacks(*far, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("spread deals no damage", health, near->GetHealthComponent().GetHealth());
			StatusHit(*original, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("target cooldown prevents immediate repeat", 2.f, static_cast<float>(Stacks(*near, DamageStatusEffectIds::ElectricEffectId)));
			f.ship->GetCombatRuntime().Tick(4.f);
			StatusHit(*original, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("four seconds permits another spread", 4.f, static_cast<float>(Stacks(*near, DamageStatusEffectIds::ElectricEffectId)));
			f.Equal("generated spread never cascades to third target", 0.f, static_cast<float>(Stacks(*third, DamageStatusEffectIds::ElectricEffectId)));
			f.Remove(module);
			const auto fresh = Enemy(f, 100.f); StatusHit(*fresh, f.ship.get(), DamageTypeSchema::Electric, 5);
			f.Equal("removal restores the ordinary four-stack limit", 4.f, static_cast<float>(Stacks(*fresh, DamageStatusEffectIds::ElectricEffectId)));
		}
		Fixture f{ report, application }; const auto target = Enemy(f);
		f.Install("Module.Offense.ArcExtender.Uncommon", 0, 1, "Module.SideFeature.ArcExtender.ArcDischarge");
		const auto neighbor = Enemy(f, 60.f); StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric, 5);
		f.Equal("one-cell Arc keeps the assigned side locked", 0.f, static_cast<float>(Stacks(*neighbor, DamageStatusEffectIds::ElectricEffectId)));
	}
	void DurationChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto target = Enemy(f);
			StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric);
			const float base = Effect(*target, DamageStatusEffectIds::ElectricEffectId)->spec.duration;
			const std::pair<GameplayTag, const char*> statuses[] = { { DamageTypeSchema::Thermal, DamageStatusEffectIds::IgniteEffectId }, { DamageTypeSchema::Cryo, DamageStatusEffectIds::CryoSlowedEffectId }, { DamageTypeSchema::Kinetic, DamageStatusEffectIds::KineticEffectId } };
			const auto apply = [&](const GameplayTag& type)
			{
				if (type != DamageTypeSchema::Kinetic) { StatusHit(*target, f.ship.get(), type); return; }
				auto payload = Plain(); payload.kineticStacks = 1;
				Hit(*target, f.ship.get(), .1f, payload, {}, {}, DamageDeliveryType::Direct, { type });
			};
			float baseDurations[3]{};
			for (int i = 0; i < 3; ++i) { apply(statuses[i].first); baseDurations[i] = Effect(*target, statuses[i].second)->spec.duration; }
			const auto module = f.Install("Module.Offense.ArcExtender.Uncommon", 1, level, "Module.SideFeature.ArcExtender.StatusResonator");
			for (int i = 0; i < 3; ++i) { apply(statuses[i].first); f.Equal("Resonator extends Thermal/Cryo/Kinetic durations", baseDurations[i] * 1.2f, Effect(*target, statuses[i].second)->spec.duration); }
			StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("Resonator extends Electric duration by twenty percent", base * 1.2f, Effect(*target, DamageStatusEffectIds::ElectricEffectId)->spec.duration);
			StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("refresh does not compound the duration multiplier", base * 1.2f, Effect(*target, DamageStatusEffectIds::ElectricEffectId)->spec.duration);
			f.Install("Module.Utility.FrostLock.Uncommon");
			StatusHit(*target, f.ship.get(), DamageTypeSchema::Cryo, 4);
			f.Equal("Frozen duration is extended", 1.2f, Effect(*target, DamageStatusEffectIds::CryoFrozenEffectId)->spec.duration);
			f.Equal("freeze cooldown is not extended", 8.f, Effect(*target, DamageStatusEffectIds::CryoFreezeCooldownEffectId)->spec.duration);
			f.Remove(module); StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("removal restores base duration", base, Effect(*target, DamageStatusEffectIds::ElectricEffectId)->spec.duration);
		}
	}
	void LuckyChecks(Report& report, Application& application)
	{
		for (const auto* family : { "Offense", "Defense", "Utility", "Movement" })
		{
			Fixture f{ report, application };
			const float ordinary = CastOffense(f).GetCooldownRemaining();
			const float luck = f.Owner(OwnerAttributeIds::Luck);
			const auto id = std::string{ "Module." } + family + ".HuntClock.Uncommon";
			const auto module = f.Install(id.c_str(), 1, 1, "Module.SideFeature.Shared.LuckyCast");
			f.Equal("Lucky Cast supplies one Luck", luck + 1.f, f.Owner(OwnerAttributeIds::Luck));
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Luck, 100.f);
			f.Equal("guaranteed Luck roll refunds twenty-five percent of real activation cooldown", ordinary * .75f, CastOffense(f).GetCooldownRemaining());
			f.Remove(module); f.Equal("removing Lucky removes the refund", ordinary, CastOffense(f).GetCooldownRemaining());
		}
	}
	void DefenseChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install("Module.Defense.BarrierRecharge.Basic");
		f.Install("Module.Defense.BraceReflex.Basic");
		auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(-shield.GetMaxShield());
		const float armor = f.Owner(OwnerAttributeIds::Armor);
		CastOffense(f); f.Equal("Offense does not refill Defense shield", 0.f, shield.GetShield());
		auto& asc = f.ship->GetAbilitySystemComponent();
		if (auto* existing = asc.GetAbility(sas::AbilitySlot::Ability1)) report.Check(asc.RemoveAbility(existing->GetHandle()), "replace only the test Q slot");
		auto definition = *content::AbilityContentCatalog::FindById("Ability.Defense.DirectionalBarrier.Basic"); definition.slot = sas::AbilitySlot::Ability1;
		report.Check(asc.GrantAbility(definition).IsValid(), "grant shipped Defense ability to test Q slot");
		auto* ability = asc.GetAbility(sas::AbilitySlot::Ability1);
		report.Check(ability && std::any_of(ability->GetDefinition().abilityTags.begin(), ability->GetDefinition().abilityTags.end(), [](const GameplayTag& tag) { return tag == GameplayTags::Ability::Defense; }), "fixture uses a real Defense ability");
		report.Check(ability->TryActivate(), "activate real Defense ability");
		f.Equal("Defense replenishes ten percent MaxShield", shield.GetMaxShield() * .1f, shield.GetShield());
		f.Equal("Defense supplies temporary Armor", armor + 25.f, f.Owner(OwnerAttributeIds::Armor));
		f.ship->GetModuleTriggerRuntime().Tick(4.1f);
		f.Equal("temporary Armor expires", armor, f.Owner(OwnerAttributeIds::Armor));
	}
}
int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "module transition content registration failed" };
		std::string title = "Module plan transition E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentChecks(report); ArcChecks(report, application); DurationChecks(report, application); LuckyChecks(report, application); DefenseChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModulePlanTransitionE2E"; WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
