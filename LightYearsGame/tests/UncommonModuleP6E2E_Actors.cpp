#include "UncommonModuleP6E2E.h"

namespace uncommon_p6_e2e
{
	namespace
	{
		struct RemoveDuringDamage
		{
			Fixture& fixture;
			std::uint32_t module;
			DelegateHandle handle;
			bool removed = false;
			RemoveDuringDamage(Fixture& f, std::uint32_t id) : fixture{ f }, module{ id }, handle{ f.ship->GetCombatRuntime().onDamageDealt.BindAction(this, &RemoveDuringDamage::Damage) } {}
			~RemoveDuringDamage() { fixture.ship->GetCombatRuntime().onDamageDealt.UnbindAction(handle); }
			void Damage(const DamageContext& context) { if (!removed && context.payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed) { removed = true; fixture.Remove(module); } }
		};
	}
	void ActorChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Offense.ScatterCore.Uncommon", 0, level);
			const auto first = Enemy(f, 200.f); const auto second = Enemy(f, 400.f); const auto third = Enemy(f, 600.f); const auto fourth = Enemy(f, 800.f);
			auto& ability = CastOffense(f);
			DamageObserver observer{ *f.ship };
			f.ship->GetCombatRuntime().SetRuntimeModifier("P6.double", { 2.f });
			const float hit = OffenseHit(f, *first, ability);
			report.Check(Actors<ModuleGuidedProjectileActor>(f).size() == 3, "Scatter first hit creates three real guided projectiles");
			const auto projectiles = Actors<ModuleGuidedProjectileActor>(f);
			for (const auto& projectile : projectiles)
			{
				f.Equal("guided damage snapshots first-hit level fraction", hit * (.12f + .03f * level), projectile->GetDamage());
				report.Check(projectile->GetDamagePayload().skipModuleDamageRules && projectile->GetDamagePayload().moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed, "guided projectile has suppressed module policy");
			}
			OffenseHit(f, *fourth, ability, 200.f);
			report.Check(Actors<ModuleGuidedProjectileActor>(f).size() == 3, "later hits in the same use cannot create another salvo");
			const float beforeFirst = first->GetHealthComponent().GetHealth();
			third->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 650.f, 50.f });
			for (const auto& projectile : projectiles) projectile->Tick(1.5f);
			f.Equal("guided projectile reaches the first nearest target", hit * (.12f + .03f * level), beforeFirst - first->GetHealthComponent().GetHealth());
			f.Equal("guided projectile reaches the second distinct target", hit * (.12f + .03f * level), 1000.f - second->GetHealthComponent().GetHealth());
			f.Equal("guided projectile follows a moving target", hit * (.12f + .03f * level), 1000.f - third->GetHealthComponent().GetHealth());
			report.Check(observer.generated == 3 && observer.allSuppressed && f.ship->GetModuleApplier().GetGeneratedActorCount() == 0, "guided impacts are finite and cannot trigger other modules");
			report.Check(Stacks(*second, DamageStatusEffectIds::KineticEffectId) == 0, "guided impacts apply no status");
			const auto oldGroup = ability.GetDamageHitGroup();
			const auto delayed = f.level->SpawnActor<AbilityWorldActor>(f.ship.get()).lock(); delayed->SetSourceAbilityInstance(&ability);
			auto& nextAbility = CastOffense(f);
			report.Check(delayed->GetDamagePayload().hitGroup == oldGroup && nextAbility.GetDamageHitGroup() != oldGroup, "delayed actors retain the activation budget across recasts");
			OffenseHit(f, *first, nextAbility, 50.f, oldGroup);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 0, "an old delayed hit cannot consume the newer use");
			OffenseHit(f, *first, nextAbility, 50.f);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 3, "new use receives a fresh first-hit salvo");
			f.Remove(module);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 0 && Actors<ModuleGuidedProjectileActor>(f).empty(), "module removal immediately cancels pending missiles");
		}
		{
			Fixture f{ report, application }; f.Install("Module.Offense.ScatterCore.Uncommon", 1, 1, "Module.SideFeature.ScatterCore.FourthMissile");
			const auto enemy = Enemy(f, 100.f); auto& ability = CastOffense(f);
			OffenseHit(f, *enemy, ability);
			report.Check(Actors<ModuleGuidedProjectileActor>(f).size() == 1, "fewer enemies produce one missile per distinct target");
			for (int index = 2; index <= 4; ++index) Enemy(f, index * 150.f);
			CastOffense(f); OffenseHit(f, *enemy, ability);
			report.Check(Actors<ModuleGuidedProjectileActor>(f).size() == 5, "Fourth Missile adds one distinct guided projectile to the next use");
		}
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Offense.ScatterCore.Uncommon", 1, 1, "Module.SideFeature.ScatterCore.Aftershock");
			const auto first = Enemy(f, 200.f); const auto inside = Enemy(f, 299.f); const auto outside = Enemy(f, 301.f);
			auto& ability = CastOffense(f); OffenseHit(f, *first, ability); OffenseHit(f, *outside, ability, 50.f);
			const auto explosions = Actors<ModuleDelayedExplosionActor>(f);
			report.Check(explosions.size() == 1, "Aftershock creates one actual delayed actor per use");
			const float before = inside->GetHealthComponent().GetHealth();
			explosions.front()->Tick(.79f); f.Equal("Aftershock waits until the full delay", before, inside->GetHealthComponent().GetHealth());
			explosions.front()->Tick(.011f); f.Equal("Aftershock uses the first hit and 100-unit radius", 25.f, before - inside->GetHealthComponent().GetHealth());
			f.Equal("Aftershock excludes the 101-unit target", 950.f, outside->GetHealthComponent().GetHealth());
			explosions.front()->Tick(.19f); report.Check(explosions.front()->GetIsPendingDestroy(), "Aftershock visual actor expires after impact");
			f.Remove(module);
		}
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Offense.ScatterCore.Uncommon", 1, 1, "Module.SideFeature.ScatterCore.Aftershock");
			const auto enemy = Enemy(f, 200.f); auto& ability = CastOffense(f);
			for (int use = 0; use < 40; ++use)
			{
				if (use > 0) CastOffense(f);
				OffenseHit(f, *enemy, ability, .1f);
			}
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 72 && Actors<ModuleDelayedExplosionActor>(f).size() == 32, "pending actor count includes the JSON-capped Aftershock population");
			const auto actor = Actors<ModuleGuidedProjectileActor>(f).front();
			enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 10000.f, 0.f });
			actor->Tick(3.01f); report.Check(actor->GetIsPendingDestroy(), "guided projectile expires at its configured lifetime");
			f.Remove(module); report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 0, "removal clears every pending actor generation");
		}
		{
			Fixture f{ report, application }; f.Install("Module.Offense.ScatterCore.Uncommon");
			const auto enemy = Enemy(f, 200.f); auto& ability = CastOffense(f);
			for (int use = 0; use < 70; ++use) { if (use > 0) CastOffense(f); OffenseHit(f, *enemy, ability, .01f); }
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 64, "Scatter respects the JSON technical actor cap");
			f.ship->Destroy();
			for (const auto& actor : Actors<ModuleGuidedProjectileActor>(f)) actor->Tick(.01f);
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == 0, "owner destruction cancels guided work without a teardown callback");
		}
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Primary.RicochetTip.Uncommon", 1, 1, "Module.SideFeature.RicochetTip.SecondBounce");
			const auto first = Enemy(f, 100.f); const auto second = Enemy(f, 200.f); const auto third = Enemy(f, 300.f);
			RemoveDuringDamage removal{ f, module };
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled; PrimaryHit(f, *first, shot);
			report.Check(removal.removed && f.ship->GetModuleApplier().GetGeneratedRuleCount() == 0, "nested damage callbacks safely replace the rule generation");
			f.Equal("nested removal cancels the remaining secondary bounce", 1000.f, third->GetHealthComponent().GetHealth());
		}
		{
			Fixture f{ report, application }; f.Install("Module.Offense.ThermalShockCoil.Uncommon", 1, 1, "Module.SideFeature.ThermalShockCoil.DeathBurst");
			const auto first = Enemy(f, 200.f); const auto inside = Enemy(f, 299.f); const auto outside = Enemy(f, 301.f);
			StatusHit(*first, f.ship.get(), DamageTypeSchema::Thermal); StatusHit(*first, f.ship.get(), DamageTypeSchema::Cryo);
			DamageObserver observer{ *f.ship };
			f.ship->GetCombatRuntime().SetRuntimeModifier("P6.double", { 2.f });
			Hit(*first, f.ship.get(), 10000.f, Plain());
			report.Check(first->GetIsPendingDestroy() && first->GetCombatRuntime().GetModuleStatusRules().Empty(), "death snapshot survives actual target teardown");
			const auto bursts = Actors<ModuleDelayedExplosionActor>(f); report.Check(bursts.size() == 1, "dual-status death creates a real Thermal burst");
			bursts.front()->Tick(0.f); f.Equal("death burst uses target max health and no second source scale", 30.f, 1000.f - inside->GetHealthComponent().GetHealth());
			f.Equal("death burst respects its configured radius", 1000.f, outside->GetHealthComponent().GetHealth());
			report.Check(observer.generated == 1 && observer.allSuppressed, "death burst cannot start other module chains");
			const auto ordinary = Enemy(f, 1000.f); Hit(*ordinary, f.ship.get(), 10000.f, Plain());
			report.Check(Actors<ModuleDelayedExplosionActor>(f).size() == 1, "ordinary kills cannot create a Thermal burst");
		}
		{
			Fixture f{ report, application }; f.Install("Module.Offense.ScatterCore.Uncommon");
			const auto first = Enemy(f, 200.f); const auto dying = Enemy(f, 400.f);
			Hit(*dying, nullptr, 999.f, Plain());
			auto& ability = CastOffense(f); OffenseHit(f, *first, ability);
			const auto count = f.ship->GetModuleApplier().GetGeneratedActorCount();
			const auto clock = f.Install("Module.Utility.HuntClock.Uncommon");
			report.Check(f.ship->GetModuleApplier().GetGeneratedActorCount() == count, "unrelated module reapply preserves an identical pending generation");
			const float cooldown = ability.GetCooldownRemaining();
			for (const auto& actor : Actors<ModuleGuidedProjectileActor>(f)) actor->Tick(1.5f);
			report.Check(dying->GetHealthComponent().GetHealth() == 0.f, "guided damage confirms a real generated kill");
			f.Equal("generated kill cannot trigger Hunt Clock", cooldown, ability.GetCooldownRemaining());
			f.Remove(clock);
		}
		{
			Fixture f{ report, application }; f.Install("Module.Offense.ScatterCore.Uncommon");
			const auto enemy = Enemy(f, 200.f); auto& ability = CastOffense(f); OffenseHit(f, *enemy, ability);
			const auto actor = Actors<ModuleGuidedProjectileActor>(f).front(); enemy->Destroy(); actor->Tick(.01f);
			report.Check(actor->GetIsPendingDestroy(), "destroyed guided target retires the missile through weak ownership");
		}
	}
}
