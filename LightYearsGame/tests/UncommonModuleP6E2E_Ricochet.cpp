#include "UncommonModuleP6E2E.h"

namespace uncommon_p6_e2e
{
	void RicochetChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto module = f.Install("Module.Primary.RicochetTip.Uncommon", 0, level);
			const auto first = Enemy(f, 400.f); const auto second = Enemy(f, 699.f); const auto outside = Enemy(f, 1000.f);
			DamageObserver observer{ *f.ship };
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			f.ship->GetCombatRuntime().SetRuntimeModifier("P6.double", { 2.f });
			PrimaryHit(f, *first, shot);
			f.Equal("real Primary impact applies the source-scaled hit", 800.f, first->GetHealthComponent().GetHealth());
			f.Equal("Ricochet scales once from the applied first hit", 200.f * (.20f + .05f * level), 1000.f - second->GetHealthComponent().GetHealth());
			f.Equal("Ricochet cannot chain into a third target", 1000.f, outside->GetHealthComponent().GetHealth());
			const float before = first->GetHealthComponent().GetHealth();
			PrimaryHit(f, *second, shot);
			f.Equal("all pellets or targets of one shot share one bounce", before, first->GetHealthComponent().GetHealth());
			report.Check(observer.generated == 1 && observer.allSuppressed, "bounce disables critical, statuses and module recursion");
			PrimaryWeaponShotMetadata next; next.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, next);
			report.Check(observer.generated == 2, "a new shot receives its own bounce budget");
			f.Remove(module);
			report.Check(f.ship->GetModuleApplier().GetGeneratedRuleCount() == 0, "removal releases Ricochet runtime rules");
		}
		{
			Fixture f{ report, application }; f.Install("Module.Primary.RicochetTip.Uncommon");
			const auto first = Enemy(f, 100.f); const auto outside = Enemy(f, 401.f);
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, shot);
			f.Equal("301-unit target is outside Ricochet range", 1000.f, outside->GetHealthComponent().GetHealth());
			outside->SetActorLocation(first->GetActorLocation() + sf::Vector2f{ 300.f, 0.f });
			PrimaryWeaponShotMetadata boundary; boundary.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, boundary);
			f.Equal("300-unit target is inside Ricochet range", 975.f, outside->GetHealthComponent().GetHealth());
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; f.Install("Module.Primary.RicochetTip.Uncommon", 1, level, "Module.SideFeature.RicochetTip.SecondBounce");
			const auto first = Enemy(f, 100.f); const auto second = Enemy(f, 399.f); const auto third = Enemy(f, 698.f); const auto fourth = Enemy(f, 997.f);
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, shot);
			f.Equal("private second bounce has a fixed fifteen-percent fraction", 15.f, 1000.f - third->GetHealthComponent().GetHealth());
			f.Equal("private second bounce still cannot chain", 1000.f, fourth->GetHealthComponent().GetHealth());
			report.Check(Stacks(*second, DamageStatusEffectIds::KineticEffectId) == 0, "bounce has no implicit Kinetic stack");
		}
		{
			Fixture f{ report, application };
			const auto module = f.Install("Module.Primary.RicochetTip.Uncommon", 1, 1, "Module.SideFeature.RicochetTip.StatusTransfer");
			const auto first = Enemy(f, 100.f); const auto second = Enemy(f, 200.f);
			StatusHit(*first, f.ship.get(), DamageTypeSchema::Thermal, 3);
			StatusHit(*first, f.ship.get(), DamageTypeSchema::Cryo, 2);
			DamageObserver observer{ *f.ship };
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, shot);
			report.Check(Stacks(*second, DamageStatusEffectIds::IgniteEffectId) == 1 && Stacks(*second, DamageStatusEffectIds::CryoSlowedEffectId) == 0, "Status Transfer copies exactly one status and one stack");
			second->GetAbilitySystemComponent().Tick(1.01f);
			report.Check(observer.generated >= 2 && observer.allSuppressed, "transferred Ignite periodic damage also suppresses other modules");
			f.Remove(module);
		}
		{
			Fixture f{ report, application }; f.Install("Module.Primary.RicochetTip.Uncommon", 0);
			const auto first = Enemy(f, 100.f); const auto second = Enemy(f, 200.f);
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, shot, 0.f);
			PrimaryHit(f, *first, shot, 100.f);
			f.Equal("zero damage does not consume the successful-hit budget", 975.f, second->GetHealthComponent().GetHealth());
		}
		{
			Fixture f{ report, application }; f.Install("Module.Primary.RicochetTip.Uncommon");
			const auto first = Enemy(f, 150.f); const auto second = Enemy(f, 350.f);
			first->SetCollisionMask(CollisionLayer::PlayerBullet); first->SetEnablePhysics(true);
			const auto* authored = content::WeaponContentCatalog::FindById("Weapon.Beam.ContinuousHeatLaser.Basic");
			report.Check(authored != nullptr, "P6 uses a real continuous beam weapon");
			const auto definition = *authored;
			PrimaryWeaponExecutionContext context{ *f.ship, definition, definition.attributes }; context.damageTags = definition.damageTags;
			PrimaryWeaponRuntimeState runtime;
			report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(definition, runtime).isValid, "P6 initializes continuous beam execution");
			f.ship->SetActorRotation(90.f); DamageObserver observer{ *f.ship };
			PrimaryWeaponExecutionSystem::BeginFire(context, runtime); f.level->TickInternal(0.f);
			PrimaryWeaponExecutionSystem::TickFire(context, runtime, .3f);
			report.CheckValue("continuous beam reaches its real target", 1.f, static_cast<float>(observer.normal), "shipped beam, physics target, one damage step");
			report.Check(observer.generated == 1 && observer.allSuppressed, "continuous beam delivers one suppressed bounce per real damage step");
			const float before = second->GetHealthComponent().GetHealth();
			PrimaryWeaponExecutionSystem::TickFire(context, runtime, .01f);
			PrimaryWeaponExecutionSystem::EndFire(context, runtime);
			report.Check(observer.generated == 2 && second->GetHealthComponent().GetHealth() < before, "beam EndFire debt shares one fresh bounce budget");
		}
	}
}
