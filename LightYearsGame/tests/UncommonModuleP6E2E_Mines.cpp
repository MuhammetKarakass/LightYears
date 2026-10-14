#include "UncommonModuleP6E2E.h"

namespace uncommon_p6_e2e
{
	void SizingChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto* arc = f.rules.lookup("Module.Offense.ArcExtender.Uncommon");
		const auto* feed = f.rules.lookup("Module.Utility.DoubleFeedCore.Uncommon");
		report.Check(arc && arc->sizeVariants.size() == 1 && arc->sizeVariants.front().shape.cells.size() == 1, "Arc Extender occupies one cell");
		report.Check(feed && feed->sizeVariants.size() == 1 && feed->sizeVariants.front().shape.cells.size() == 2, "Double Feed Core occupies two cells");
		f.Install("Module.Offense.ArcExtender.Uncommon");
		const auto target = Enemy(f);
		StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric, 5);
		report.Check(Stacks(*target, DamageStatusEffectIds::ElectricEffectId) == 5, "one-cell Arc Extender keeps its five-stack behavior");
		const float before = f.Owner(OwnerAttributeIds::AttackPower);
		f.Install("Module.Utility.DoubleFeedCore.Uncommon");
		report.Check(f.Owner(OwnerAttributeIds::AttackPower) >= before + 15.f, "two-cell Double Feed Core still applies its AP bonus");
		report.Check(!content::ModuleSideFeatureCatalog::FindById(sas::ContentId{ "Module.SideFeature.LootShield.HuntFuel" }), "removed Hunt Fuel is absent from the live catalog");
	}
	void MineChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Movement.TrailMines.Uncommon", 0, level);
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::AttackPower, 100.f);
			CastMovement(f);
			f.ship->SetVelocity({}); f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(.1f);
			report.Check(Actors<ModuleMineActor>(f).empty(), "stationary Movement use leaves no mines");
			Move(f, 49.f); report.Check(Actors<ModuleMineActor>(f).empty(), "mine distance below fifty units carries forward");
			Move(f, 1.f); report.Check(Actors<ModuleMineActor>(f).size() == 1, "crossing fifty actual units creates one mine");
			const auto mine = Actors<ModuleMineActor>(f).front();
			const auto enemy = Enemy(f, 99.f); const auto outside = Enemy(f, 101.f);
			enemy->SetActorLocation(mine->GetActorLocation() + sf::Vector2f{ 99.f, 0.f }); outside->SetActorLocation(mine->GetActorLocation() + sf::Vector2f{ 101.f, 0.f });
			const float expectedDamage = 25.f + 5.f * level + (.25f + .05f * level) * 100.f;
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::AttackPower, 300.f);
			DamageObserver observer{ *f.ship };
			mine->Tick(.99f); f.Equal("mine waits for its full fuse", 1000.f, enemy->GetHealthComponent().GetHealth());
			mine->Tick(.011f); f.Equal("mine uses level base plus AP snapshotted at drop", expectedDamage, 1000.f - enemy->GetHealthComponent().GetHealth());
			f.Equal("mine excludes the target outside radius", 1000.f, outside->GetHealthComponent().GetHealth());
			report.Check(observer.generated == 1 && observer.allSuppressed, "mine hit disables critical and module triggers");
			mine->Tick(.19f); report.Check(mine->GetIsPendingDestroy(), "mine actor expires after its explosion");
			for (int step = 0; step < 12; ++step) Move(f, 100.f);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == static_cast<std::size_t>(10 + 2 * level), "mine count respects its level-dependent technical cap", { level, f.ship->GetModuleApplier().GetGeneratedActorCount() });
			f.Remove(module); report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 0 && Actors<ModuleMineActor>(f).empty(), "removing Trail Mines cancels every pending fuse");
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; f.Install("Module.Movement.TrailMines.Uncommon", 1, level, "Module.SideFeature.TrailMines.QuickFuse");
			CastMovement(f); for (int step = 0; step < 11; ++step) Move(f, 100.f);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == static_cast<std::size_t>(14 + 2 * level), "Quick Fuse adds four to every level's active limit");
			const auto mine = Actors<ModuleMineActor>(f).front(); const auto enemy = Enemy(f);
			enemy->SetActorLocation(mine->GetActorLocation());
			mine->Tick(.69f); f.Equal("Quick Fuse does not detonate early", 1000.f, enemy->GetHealthComponent().GetHealth());
			mine->Tick(.011f); f.Equal("Quick Fuse detonates at point-seven seconds", 25.f + 5.f * level + (.25f + .05f * level) * f.Owner(OwnerAttributeIds::AttackPower), 1000.f - enemy->GetHealthComponent().GetHealth());
		}
		{
			Fixture f{ report, application }; f.Install("Module.Movement.TrailMines.Uncommon", 1, 1, "Module.SideFeature.TrailMines.CryoMines");
			CastMovement(f); Move(f, 50.f); const auto mine = Actors<ModuleMineActor>(f).front(); const auto enemy = Enemy(f);
			enemy->SetActorLocation(mine->GetActorLocation()); mine->Tick(1.01f);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::CryoSlowedEffectId) == 1 && Stacks(*enemy, DamageStatusEffectIds::KineticEffectId) == 0, "Cryo Mines adds exactly one Cryo stack");
		}
		{
			Fixture f{ report, application }; f.Install("Module.Movement.TrailMines.Uncommon", 1, 1, "Module.SideFeature.TrailMines.CryoMines"); f.Install("Module.Utility.FrostLock.Uncommon");
			CastMovement(f); Move(f, 50.f); const auto mine = Actors<ModuleMineActor>(f).front(); const auto enemy = Enemy(f);
			enemy->SetActorLocation(mine->GetActorLocation()); StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo, 3);
			mine->Tick(1.01f);
			report.Check(Stacks(*enemy, DamageStatusEffectIds::CryoSlowedEffectId) == 4 && !Effect(*enemy, DamageStatusEffectIds::CryoFrozenEffectId), "generated Cryo cannot trigger Frost Lock even at its threshold");
		}
		{
			Fixture f{ report, application }; f.Install("Module.Movement.TrailMines.Uncommon");
			CastMovement(f);
			f.ship->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 1000.f, 0.f });
			f.ship->SetVelocity({}); f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(.1f);
			report.Check(Actors<ModuleMineActor>(f).empty(), "instant teleport is excluded from traveled mine distance");
			Move(f, 50.f); const auto count = f.ship->GetModuleApplier().GetGeneratedActorCount();
			f.ship->BeginPortalTransit(); f.ship->GetModuleApplier().Tick(.1f);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == count, "portal transit cannot replay the previous movement step");
			f.ship->CompletePortalTransit(f.ship->GetActorLocation());
			f.ship->SetVelocity({}); f.ship->GetMovementComponent().Tick(5.f); f.ship->GetModuleApplier().Tick(5.f);
			Move(f, 100.f); report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == count, "mine drop window ends after five seconds");
		}
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Movement.TrailMines.Uncommon");
			CastMovement(f); Move(f, 50.f);
			report.Check(f.loadout.SetLevel(f.rules, module, 2) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "P6 changes level through the real module API");
			report.Check(Actors<ModuleMineActor>(f).empty(), "changing mine rules retires pending actors from the old generation");
			CastMovement(f); Move(f, 50.f); report.Check(Actors<ModuleMineActor>(f).size() == 1, "new generation works after level change");
			const auto placement = *std::find_if(f.loadout.GetPlacements().begin(), f.loadout.GetPlacements().end(), [module](const auto& value) { return value.instanceId == module; });
			f.Remove(module);
			report.Check(f.loadout.Install(f.rules, module, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules, true), "P6 reinserts the real module in combat");
			report.Check(f.ship->GetModuleApplier().IsRebooting(module) && f.ship->GetModuleApplier().GetGeneratedRuleCount() == 0, "reboot holds generated rules inactive");
			CastMovement(f); Move(f, 50.f); report.Check(Actors<ModuleMineActor>(f).empty(), "Movement during reboot cannot arm mines");
			f.ship->GetModuleApplier().Tick(f.rules.zoneTable.rebootDurationSeconds + .01f);
			CastMovement(f); Move(f, 50.f); report.Check(Actors<ModuleMineActor>(f).size() == 1, "mine behavior returns after reboot expiry");
		}
		{
			Fixture f{ report, application }; const auto baseline = f.ship->GetModuleApplier().GetOwnedHandleCount();
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				const auto module = f.Install("Module.Movement.TrailMines.Uncommon", cycle % 2, cycle % 3 + 1);
				CastMovement(f); Move(f, 100.f); report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 2, "each mine install starts exactly one fresh generation", cycle);
				f.Remove(module);
				report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline && f.ship->GetModuleApplier().GetGeneratedActorCount() == 0 && f.ship->GetModuleApplier().GetGeneratedRuleCount() == 0, "ten cycles leave no handles, actors or generated rules", cycle);
			}
		}
	}
}
