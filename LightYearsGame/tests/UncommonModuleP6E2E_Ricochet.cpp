#include "gameConfigs/CollisionLayers.h"
#include "UncommonModuleP6E2E.h"
#include "framework/MathUtility.h"

namespace uncommon_p6_e2e
{
	void BalanceChecks(Report& report, Application& application)
	{
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			const auto module = f.Install("Module.Primary.FocusedBarrel.Basic");
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *enemy, shot);
			f.Equal("Focused Barrel multiplies the complete Primary hit", 130.f, 1000.f - enemy->GetHealthComponent().GetHealth());
			f.Equal("Focused Barrel excludes ability damage", 100.f, Hit(*enemy, f.ship.get(), 100.f, Plain()));
			f.Remove(module);
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			f.Install("Module.Primary.PhotonicLens.Uncommon", 0, level);
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *enemy, shot);
			f.Equal("Lens boosts a real Primary projectile on a clean target", 100.f * (1.5f + .03f * (level - 1)), 1000.f - enemy->GetHealthComponent().GetHealth());
			f.Equal("Lens excludes non-primary ability damage", 100.f, Hit(*enemy, f.ship.get(), 100.f, Plain()));
			f.Equal("Lens excludes periodic damage", 100.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, {}, DamageDeliveryType::Periodic));
			f.Equal("Lens excludes periodic damage even with Primary source tags", 100.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Periodic));
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			const float before = enemy->GetHealthComponent().GetHealth();
			PrimaryWeaponShotMetadata next; next.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *enemy, next);
			f.Equal("Lens excludes a Primary target with status", 100.f, before - enemy->GetHealthComponent().GetHealth());
		}
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			f.Install("Module.Offense.FinishingBlow.Basic");
			for (float health : { 251.f, 250.f, 200.f })
			{
				enemy->GetHealthComponent().ChangeHealth(health - enemy->GetHealthComponent().GetHealth());
				PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
				PrimaryHit(f, *enemy, shot);
				f.Equal("Finishing Blow checks pre-hit health at the inclusive quarter threshold", health <= 250.f ? 120.f : 100.f, health - enemy->GetHealthComponent().GetHealth());
			}
			enemy->GetHealthComponent().Regenerate(250.f - enemy->GetHealthComponent().GetHealth());
			f.Equal("Finishing Blow also applies to ability damage", 120.f, Hit(*enemy, f.ship.get(), 100.f, Plain()));
		}
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			f.Install("Module.Movement.EscapeReflex.Basic");
			const float base = f.ship->GetMovementSpeedMultiplier();
			auto& ability = CastOffense(f); OffenseHit(f, *enemy, ability, 1.f);
			f.Equal("outgoing ability damage cannot trigger Escape Reflex", base, f.ship->GetMovementSpeedMultiplier());
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *enemy, shot, 1.f);
			f.Equal("outgoing primary damage cannot trigger Escape Reflex", base, f.ship->GetMovementSpeedMultiplier());
			DamageContext foreign; foreign.source = f.ship.get(); foreign.target = enemy.get(); foreign.appliedDamage = 1.f;
			sas::AbilityEvent event; event.eventTag = GameplayTags::Event::Owner::DamageTaken; event.magnitude = 1.f; event.SetContext(&foreign);
			f.ship->GetAbilitySystemComponent().HandleGameplayEvent(event);
			f.Equal("forwarded foreign DamageTaken cannot trigger Escape Reflex", base, f.ship->GetMovementSpeedMultiplier());
			const auto* weapon = content::WeaponContentCatalog::FindById("Weapon.Projectile.FighterRapidLaser.Basic");
			const auto projectile = f.level->SpawnActor<PrimaryWeaponProjectileActor>(enemy.get(), weapon->presentationDefinition, weapon->attributes).lock();
			projectile->SetDamage(1.f); projectile->SetDamagePayload(Plain()); projectile->SetShotMetadata(shot);
			projectile->OnActorBeginOverlap(f.ship.get());
			f.Equal("incoming enemy Primary projectile triggers Escape Reflex", base * 1.25f, f.ship->GetMovementSpeedMultiplier());
			f.ship->GetModuleTriggerRuntime().Tick(2.01f);
			f.Equal("Escape Reflex expires after two seconds", base, f.ship->GetMovementSpeedMultiplier());
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; f.Install("Module.Movement.MovementRhythm.Uncommon", 0, level);
			const float base = f.Owner(OwnerAttributeIds::AttackSpeed);
			for (float speed : { 0.f, 100.f, 199.f, 200.f, 250.f, 299.f, 300.f, 350.f, 400.f, 500.f, 199.f })
			{
				Move(f, speed * .1f, .1f);
				const float steps = speed < 200.f ? 0.f : std::floor(speed / 100.f) - 1.f;
				f.Equal("Movement Rhythm changes only at complete speed steps", base + steps * (3.5f + .5f * (level - 1)), f.Owner(OwnerAttributeIds::AttackSpeed));
			}
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			f.Install("Module.Offense.ThermalShockCoil.Uncommon", 0, level);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal); StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			f.Equal("Thermal Shock uses the new thirty-percent base and existing progression", 100.f * (1.3f + .05f * (level - 1)), Hit(*enemy, f.ship.get(), 100.f, Plain()));
		}
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			f.Install("Module.Defense.ColdRetort.Uncommon", 1, 1, "Module.SideFeature.ColdRetort.JarringEffect");
			f.ship->GetShieldComponent().ChangeShield(-10000.f);
			f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Cryo);
			f.Equal("Jarring Effect reduces incoming status-source damage eighteen percent", 82.f, Hit(*f.ship, enemy.get(), 100.f, Plain()));
		}
		{
			Fixture f{ report, application };
			const float regen = f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen);
			const auto defaults = f.loadout.GetPlacements();
			for (const auto& placement : defaults) if (f.rules.lookup(placement.moduleId)->systemCategory == GameplayTags::Module::System::Shield) f.Remove(placement.instanceId);
			const auto heavy = f.Install("Module.Defense.ShieldGenerator.Heavy");
			f.Equal("Heavy Generator boosts regen exactly thirty percent", regen * 1.3f, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
			f.Equal("Heavy Generator reduces Fighter delay from six to four", 4.f, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRechargeDelay));
			f.Remove(heavy);
		}
	}
	void RicochetChecks(Report& report, Application& application)
	{
		{
			Fixture f{ report, application }; const auto module = f.Install("Module.Primary.RicochetTip.Uncommon");
			const auto enemy = Enemy(f, 100.f);
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *enemy, shot);
			const auto projectiles = Actors<ModuleRicochetProjectileActor>(f);
			report.Check(projectiles.size() == 1, "Ricochet with no second enemy emits one random projectile");
			const auto projectile = projectiles.front();
			f.Equal("random bounce snapshots the applied hit", 25.f, projectile->GetDamage());
			projectile->OnActorBeginOverlap(enemy.get());
			report.Check(!projectile->GetIsPendingDestroy(), "random bounce ignores the original victim");
			const auto start = projectile->GetActorLocation(); projectile->Tick(.1f);
			f.Equal("random bounce keeps the Primary projectile speed", projectile->GetProjectileSpeed() * .1f, GetVectorLength(projectile->GetActorLocation() - start));
			f.Remove(module);
			report.Check(projectile->GetIsPendingDestroy(), "module removal cancels random bounce through its lease");
		}
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
			const auto first = Enemy(f, 100.f); const auto outside = Enemy(f, 551.f);
			PrimaryWeaponShotMetadata shot; shot.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, shot);
			f.Equal("451-unit target is outside Ricochet range", 1000.f, outside->GetHealthComponent().GetHealth());
			outside->SetActorLocation(first->GetActorLocation() + sf::Vector2f{ 450.f, 0.f });
			PrimaryWeaponShotMetadata boundary; boundary.criticalPolicy = DamageCriticalPolicy::Disabled;
			PrimaryHit(f, *first, boundary);
			f.Equal("450-unit target is inside Ricochet range", 975.f, outside->GetHealthComponent().GetHealth());
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
			first->SetCollisionMask(vega::GameCollision::PlayerBullet); first->SetEnablePhysics(true);
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
