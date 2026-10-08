#include "UncommonModuleCompletionE2E.h"
#include "player/Player.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
namespace uncommon_completion_e2e
{
	namespace
	{
		std::uint32_t InstallPlayer(Report& report, Player& player, const char* module, int level, const char* side)
		{
			const auto rules = player.BuildModuleRules();
			const auto id = player.AddModuleToInventory(module, 1, ModuleRarity::Uncommon, level, side);
			report.Check(rules && id != 0, "Gold module enters actual player inventory");
			auto candidate = *player.GetModuleLoadout().FindInventoryItem(id);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules->schematic.height; ++y)
					for (int x = 0; x < rules->schematic.width; ++x)
					{
						candidate.origin = { x, y }; candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(*rules, player.GetModuleLoadout(), candidate) != ModulePlacementFailure::None) continue;
						report.Check(player.TryInstallModule(id, candidate.origin, rotation) == ModulePlacementFailure::None, "Gold module installs through actual player API");
						return id;
					}
			throw std::runtime_error{ "No legal Gold placement" };
		}
	}
	void ReactiveChecks(Report& report, Application& application)
	{
		const char* modules[]{ "Module.Offense.ScatterCore.Uncommon", "Module.Offense.SharpScale.Uncommon", "Module.Movement.TrailMines.Uncommon", "Module.Offense.GlassTopCore.Uncommon", "Module.Defense.GritYourTeeth.Uncommon", "Module.Utility.HuntClock.Uncommon", "Module.Movement.MovementRhythm.Uncommon" };
		const char* sides[]{ "Module.SideFeature.Shared.O4", "Module.SideFeature.Shared.O5", "Module.SideFeature.Shared.G1", "Module.SideFeature.GlassTop.LowHealthCrit", "Module.SideFeature.GritYourTeeth.ShieldRegen", "Module.SideFeature.HuntClock.HuntBounty", "Module.SideFeature.MovementRhythm.Momentum" };
		for (int kind = 0; kind < 7; ++kind)
		{
			Fixture f{ report, application };
			const auto handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const float critical = f.Owner(OwnerAttributeIds::CriticalChance);
			const float attackSpeed = f.Owner(OwnerAttributeIds::AttackSpeed);
			const float regen = f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen);
			const auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet);
			const auto check = [&](bool active, bool mainActive)
			{
				if (kind == 0) { auto target = Enemy(f); StatusHit(*target, f.ship.get(), DamageTypeSchema::Electric); f.Equal("O4 small/reboot gate in real damage", content::DamageStatusBalanceCatalog::Get().electric.duration + (active ? 1.f : 0.f), Effect(*target, DamageStatusEffectIds::ElectricEffectId)->spec.duration); }
				else if (kind == 1) { auto target = Enemy(f); f.Equal("O5 small/reboot gate in real damage", active ? 11.5f : 10.f, Hit(*target, f.ship.get(), 10.f, Plain())); }
				else if (kind == 2 || kind == 5) { const auto before = wallet->GetAmount(); f.Kill(); report.Check(wallet->GetAmount() == before + (active ? 1u : 0u), "Gold small/reboot gate in real kill"); }
				else if (kind == 3 || kind == 4)
				{
					auto& health = f.ship->GetHealthComponent(); health.ChangeHealth(health.GetMaxHealth() * .3f - health.GetHealth());
					if (kind == 3) f.Equal("Glass small/reboot gate at low health", critical + (active ? 10.f : 0.f), f.Owner(OwnerAttributeIds::CriticalChance));
					else f.Equal("Grit small/reboot gate at low health", regen * (active ? 2.f : 1.f), f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
					health.Regenerate(health.GetMaxHealth());
				}
				else
				{
					f.ship->SetVelocity({ 500.f, 0.f }); f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(0.f);
					f.ship->SetVelocity({ 0.f, 0.f }); f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(mainActive ? .75f : 0.f);
					f.Equal("fade small/reboot gate on actual movement", attackSpeed + (active ? 8.75f : 0.f), f.Owner(OwnerAttributeIds::AttackSpeed));
				}
			};
			const auto small = f.Install(modules[kind], 0, 1, sides[kind]); check(false, true); f.Remove(small);
			const auto large = f.Install(modules[kind], 1, 1, sides[kind]); check(true, true);
			const auto placement = *f.loadout.FindPlacement(large); f.Remove(large);
			report.Check(f.loadout.Install(f.rules, large, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules, true), "remaining feature enters real combat reboot");
			check(false, false);
			f.ship->GetModuleApplier().Tick(f.rules.zoneTable.rebootDurationSeconds + .01f); check(true, true); f.Remove(large);
			for (int cycle = 0; cycle < 10; ++cycle) { const auto id = f.Install(modules[kind], 1, 1, sides[kind]); check(true, true); f.Remove(id); report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == handles, "ten remaining-feature cycles leave no owned modifier"); }
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const float critical = f.Owner(OwnerAttributeIds::CriticalChance);
			const auto glass = f.Install("Module.Offense.GlassTopCore.Uncommon", 1, level, "Module.SideFeature.GlassTop.LowHealthCrit");
			auto& health = f.ship->GetHealthComponent();
			const auto setHealth = [&](float fraction) { health.ChangeHealth(health.GetMaxHealth() * fraction - health.GetHealth()); };
			setHealth(.5f); f.Equal("Glass critical bonus includes 50 percent boundary", critical + 10.f, f.Owner(OwnerAttributeIds::CriticalChance));
			setHealth(.501f); f.Equal("Glass bonus clears above boundary", critical, f.Owner(OwnerAttributeIds::CriticalChance));
			setHealth(.4f); f.Remove(glass); f.Equal("Glass removal releases active critical modifier", critical, f.Owner(OwnerAttributeIds::CriticalChance));
			const float regeneration = f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen);
			const auto grit = f.Install("Module.Defense.GritYourTeeth.Uncommon", 1, level, "Module.SideFeature.GritYourTeeth.ShieldRegen");
			setHealth(.35f); f.Equal("Grit doubles ship regeneration at 35 percent", regeneration * 2.f, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
			setHealth(.351f); f.Equal("Grit removes regeneration above boundary", regeneration, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
			setHealth(.3f); auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(-10000.f); shield.ClearRechargeDelay();
			f.ship->Tick(.05f); f.Equal("Grit affects actual passive shield recharge", regeneration * 2.f * .05f, shield.GetShield());
			f.Remove(grit); f.Equal("Grit removal releases ship modifier", regeneration, f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen));
			setHealth(1.f);
			const float attackSpeed = f.Owner(OwnerAttributeIds::AttackSpeed);
			const auto rhythm = f.Install("Module.Movement.MovementRhythm.Uncommon", 1, level, "Module.SideFeature.MovementRhythm.Momentum");
			const auto speed = [&](float value, float delta) { f.ship->SetVelocity({ value, 0.f }); f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(delta); };
			const float bonus = 5.f * (3.5f + .5f * (level - 1));
			speed(500.f, .1f); f.Equal("fade does not add a second speed bonus", attackSpeed + bonus, f.Owner(OwnerAttributeIds::AttackSpeed));
			speed(0.f, .75f); f.Equal("stationary bonus fades to half after 0.75 seconds", attackSpeed + bonus * .5f, f.Owner(OwnerAttributeIds::AttackSpeed));
			f.ship->GetModuleApplier().Tick(.75f); f.Equal("fade ends after 1.5 seconds", attackSpeed, f.Owner(OwnerAttributeIds::AttackSpeed));
			speed(500.f, .1f); speed(0.f, .25f); f.Remove(rhythm); f.Equal("removal clears in-progress fade", attackSpeed, f.Owner(OwnerAttributeIds::AttackSpeed));
		}
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			Player player;
			f.ship = player.SpawnSpaceShip(f.level.get()).lock(); f.level->TickInternal(0.f); f.ship->SetInvulnerability(false); f.ship->SetUseScreenClamp(false); f.ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(player.GetGold() == 0, "new run starts at zero Gold");
			const auto hunt = InstallPlayer(report, player, "Module.Utility.HuntClock.Uncommon", level, "Module.SideFeature.HuntClock.HuntBounty");
			const auto trail = InstallPlayer(report, player, "Module.Movement.TrailMines.Uncommon", level, "Module.SideFeature.Shared.G1");
			f.Kill(); report.Check(player.GetGold() == 2, "G1 and Hunt Bounty independently award one Gold");
			f.Kill(nullptr, true); report.Check(player.GetGold() == 2, "generated kills award no Gold");
			auto other = Enemy(f); f.Kill(other.get()); report.Check(player.GetGold() == 2, "another actor's kill awards no owner Gold");
			report.Check(player.TryUninstallModule(trail), "G1 uninstalls through player");
			f.Kill(); report.Check(player.GetGold() == 3, "removing G1 leaves only Hunt Bounty");
			f.ship->Destroy(); f.level->TickInternal(0.f);
			f.ship = player.SpawnSpaceShip(f.level.get()).lock(); f.level->TickInternal(0.f); f.ship->SetInvulnerability(false);
			report.Check(player.GetGold() == 3, "Gold survives real ship respawn");
			f.Kill(); report.Check(player.GetGold() == 4, "respawn rebinds the same run wallet");
			report.Check(player.TryUninstallModule(hunt), "Hunt Bounty uninstalls through player");
			f.Kill(); report.Check(player.GetGold() == 4, "no equipped Gold side means no base Gold drop");
			f.ship->Destroy(); f.level->TickInternal(0.f); f.ship = player.SpawnSpaceShip(f.level.get()).lock(); f.level->TickInternal(0.f);
			f.ship->Destroy(); f.level->TickInternal(0.f);
			report.Check(player.SpawnSpaceShip(f.level.get()).expired() && player.GetGold() == 0, "life exhaustion resets run Gold through real reset flow");
		}
	}
}
