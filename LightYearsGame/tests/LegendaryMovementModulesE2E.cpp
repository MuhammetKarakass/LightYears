#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP5E2E.h"
#include "gameplay/movement/MovementInfluenceService.h"
#include "presentation/ability/ionComet/IonCometVisualActor.h"
#include "presentation/hud/vitals/VitalsPresenter.h"
#include "presentation/hud/vitals/VitalsView.h"
#include "player/PlayerManager.h"
#include "player/Player.h"

namespace
{
	using namespace uncommon_p5_e2e;
	constexpr auto kBlink = "Module.Movement.BlinkDrive.Legendary";
	constexpr auto kIon = "Module.Movement.IonComet.Legendary";
	constexpr auto kElectric = "Effect.Status.Damage.Electric";

	struct MovementFixture : Fixture
	{
		ShipSchematic schematic;
		ModuleRulesContext movementRules;
		MovementFixture(Report& report, Application& application, int effective = 1) : Fixture{ report, application }, movementRules{ schematic, rules.lookup, rules.zoneTable, rules.sideFeatureLookup }
		{
			schematic.width = schematic.height = 6;
			schematic.cellZones.assign(36, effective > 3 ? GameplayTags::Module::Zone::Hull : GameplayTags::Module::Zone::Nose);
			const auto defaults = loadout.GetPlacements();
			for (const auto& item : defaults) report.Check(loadout.Remove(item.instanceId), "remove fixture default systems");
			ship->GetModuleApplier().Apply(loadout, movementRules);
			auto& movement = ship->GetMovementComponent();
			movement.SetMovementMode(ShipMovementMode::ThrustDrift);
			movement.GetAttributes().maxSpeed.currentValue = 10000.f;
			movement.GetAttributes().linearDamping.currentValue = 1.f;
			movement.GetAttributes().inputResponsiveness.currentValue = 0.f;
			ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::EnergyPower, 100.f);
		}
		std::uint32_t InstallMovement(const char* id, int effective = 1)
		{
			const auto* module = movementRules.lookup(id);
			const int naturalLevel = effective > 3 ? effective - 2 : effective;
			const auto instance = loadout.AddToInventory(movementRules, *module, 0, module->rarity, naturalLevel);
			report.Check(instance != 0, "shipped Movement Legendary enters inventory");
			const auto placement = FindPlacement(movementRules, loadout, *module, instance);
			report.Check(placement.has_value(), "Movement Legendary has a legal fixed-shape placement");
			report.Check(loadout.Install(movementRules, instance, placement->first, placement->second) == ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, movementRules), "install through placement, resolver and live applier");
			ship->GetMovementComponent().GetAttributes().maxSpeed.currentValue = 10000.f;
			ship->GetMovementComponent().GetAttributes().linearDamping.currentValue = 1.f;
			ship->GetMovementComponent().GetAttributes().inputResponsiveness.currentValue = 0.f;
			return instance;
		}
		void RemoveMovement(std::uint32_t instance) { report.Check(loadout.Uninstall(instance) && ship->GetModuleApplier().Apply(loadout, movementRules), "uninstall through the live applier"); }
		LegendaryBlinkDriveRuntime& Blink() { return ship->GetModuleApplier().GetLegendaryBlinkDriveRuntime(); }
		LegendaryIonCometRuntime& Ion() { return ship->GetModuleApplier().GetLegendaryIonCometRuntime(); }
		void Flight(float speed, float seconds)
		{
			ship->SetVelocity({ speed, 0.f });
			ship->GetMovementComponent().Tick(seconds);
			ship->GetModuleApplier().Tick(seconds);
		}
		void Control(const sf::Vector2f& input, bool shift, float seconds)
		{
			ship->FindComponent<PlayerMovementComponent>()->ApplyInput(input, shift, seconds);
			ship->SpaceShip::Tick(seconds);
			ship->GetModuleApplier().Tick(seconds);
		}
	};

	class Wall final : public Actor
	{
	public:
		explicit Wall(World* world) : Actor{ world }
		{
			SetEnablePhysics(true);
			SetPhysicsBodyType(PhysicsBodyType::Static);
			SetCollisionLayer(GameCollision::Environment);
			SetCollisionMask(GameCollision::Player);
		}
		sf::Vector2f GetPhysicsCollisionBoxHalfExtents() const override { return { 10.f, 90.f }; }
	};

	struct DistanceProbe
	{
		PlayerSpaceShip& ship;
		DelegateHandle handle;
		float distance = 0.f;
		explicit DistanceProbe(PlayerSpaceShip& owner) : ship{ owner }, handle{ ship.onRunDistanceTraveled.BindAction(this, &DistanceProbe::OnDistance) } {}
		~DistanceProbe() { ship.onRunDistanceTraveled.UnbindAction(handle); }
		void OnDistance(float value) { distance += value; }
	};

	void BlinkLevels(Report& report, Application& application)
	{
		for (int level = 1; level <= 5; ++level)
		{
			MovementFixture f{ report, application, level };
			const auto instance = f.InstallMovement(kBlink, level);
			const float recharge = 6.f - .3f * (level - 1);
			f.Equal("three charges at every effective level", 3.f, static_cast<float>(f.Blink().GetCharges()));
			f.Equal("resolved refill follows effective level including Hull bonuses", recharge, f.Blink().GetRules().front().rechargeSeconds);
			const auto origin = f.ship->GetActorLocation();
			report.Check(f.Blink().TryBlink({ 1.f, 0.f }), "first Blink succeeds");
			f.Equal("distance never scales with level", 350.f, f.ship->GetActorLocation().x - origin.x);
			f.Equal("level-dependent speed boost", 1.2f + .03f * (level - 1), f.ship->GetMovementSpeedMultiplier());
			report.Check(!f.Blink().TryBlink({ 1.f, 0.f }), "immediate second use is rejected");
			f.Blink().Tick(.999f);
			report.Check(!f.Blink().TryBlink({ 1.f, 0.f }), "the fixed one-second gap is enforced");
			f.Blink().Tick(.0011f);
			report.Check(f.Blink().TryBlink({ 1.f, 0.f }), "second use after one second succeeds");
			f.Equal("refresh does not stack speed bonuses", 1.2f + .03f * (level - 1), f.ship->GetMovementSpeedMultiplier());
			f.Equal("second use refreshes full boost duration", 2.f, f.Blink().GetBoostRemaining());
			f.Blink().Tick(1.f);
			report.Check(f.Blink().TryBlink({ 1.f, 0.f }), "third use succeeds");
			f.Equal("all three charges can be spent", 0.f, static_cast<float>(f.Blink().GetCharges()));
			f.Blink().Tick(recharge - 2.0012f);
			f.Equal("no charge before its own refill boundary", 0.f, static_cast<float>(f.Blink().GetCharges()));
			f.Blink().Tick(.002f);
			f.Equal("only one charge refills at a time", 1.f, static_cast<float>(f.Blink().GetCharges()));
			f.Blink().Tick(recharge);
			f.Equal("second missing charge uses a separate interval", 2.f, static_cast<float>(f.Blink().GetCharges()));
			f.Blink().Tick(recharge);
			f.Equal("third missing charge uses a separate interval", 3.f, static_cast<float>(f.Blink().GetCharges()));
			f.Equal("expired boost releases its modifier", 1.f, f.ship->GetMovementSpeedMultiplier());
			f.RemoveMovement(instance);
			report.Check(!f.Blink().IsInstalled() && !f.Blink().TryBlink({ 1.f, 0.f }), "uninstalled runtime cannot teleport");
		}
	}

	void BlinkDirectionsAndObstacles(Report& report, Application& application)
	{
		MovementFixture f{ report, application };
		f.InstallMovement(kBlink);
		auto origin = f.ship->GetActorLocation();
		f.ship->SetVelocity({ -100.f, 0.f });
		report.Check(f.Blink().TryBlink({ 0.f, 2.f }), "movement input takes priority over velocity");
		f.Equal("input direction is normalized", 350.f, f.ship->GetActorLocation().y - origin.y);
		f.Equal("teleport preserves flight velocity", -100.f, f.ship->GetVelocity().x);
		f.Blink().Tick(1.f); origin = f.ship->GetActorLocation();
		report.Check(f.Blink().TryBlink({}), "no input uses flight velocity");
		f.Equal("velocity fallback direction", -350.f, f.ship->GetActorLocation().x - origin.x);
		f.Blink().Tick(1.f); f.ship->SetVelocity({}); f.ship->SetActorRotation(90.f); origin = f.ship->GetActorLocation();
		report.Check(f.Blink().TryBlink({}), "stationary fallback uses the nose");
		f.Equal("nose fallback direction", 350.f, f.ship->GetActorLocation().x - origin.x);
		f.Blink().Tick(20.f);
		const auto wall = f.level->SpawnActor<Wall>().lock();
		origin = f.ship->GetActorLocation(); wall->SetActorLocation(origin + sf::Vector2f{ 175.f, 0.f });
		f.level->TickInternal(0.f);
		report.Check(f.Blink().TryBlink({ 1.f, 0.f }), "an obstacle along the path does not block Blink");
		f.Equal("Blink does not sweep the path", 350.f, f.ship->GetActorLocation().x - origin.x);
		f.Blink().Tick(1.f); origin = f.ship->GetActorLocation();
		wall->SetActorLocation(origin + sf::Vector2f{ 350.f, 0.f });
		f.level->TickInternal(0.f);
		report.Check(f.Blink().TryBlink({ 1.f, 0.f }), "an obstructed destination stops before the obstacle");
		const float distance = f.ship->GetActorLocation().x - origin.x;
		report.Check(distance > 200.f && distance < 340.f, "the complete ship hull remains before the destination wall", distance);
		f.ship->GetMovementComponent().Tick(.25f);
		f.Equal("teleport is absent from movement distance", 0.f, f.ship->GetMovementComponent().GetCurrentMovementDistance());
		f.Equal("teleport is absent from actual speed", 0.f, f.ship->GetMovementComponent().GetCurrentMovementSpeed());
		report.Check(!f.ship->IsInvulnerable(), "Blink grants no invulnerability");
		f.Blink().Tick(20.f); wall->Destroy();
		DistanceProbe probe{ *f.ship };
		StatusHit(*f.ship, f.ship.get(), DamageTypeSchema::Thermal);
		const int ignite = Stacks(*f.ship, "Effect.Status.Damage.Ignite");
		report.Check(ignite > 0 && f.Blink().TryBlink({ 1.f, 0.f }), "a status-afflicted ship can Blink");
		f.Equal("Blink does not cleanse existing statuses", static_cast<float>(ignite), static_cast<float>(Stacks(*f.ship, "Effect.Status.Damage.Ignite")));
		f.ship->Tick(.1f);
		f.Equal("full player tick emits no run distance for a stationary teleport", 0.f, probe.distance);
	}

	void BlinkReplacement(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto* controls = f.ship->FindComponent<PlayerMovementComponent>();
		controls->ApplyInput({ 0.f, -1.f }, true, .05f);
		report.Check(f.ship->IsAfterburning(), "baseline Shift runs the existing Afterburner");
		controls->ApplyInput({}, false, .01f);
		const auto blinkId = f.Install(kBlink);
		auto& blink = f.ship->GetModuleApplier().GetLegendaryBlinkDriveRuntime();
		const auto standard = std::find_if(f.loadout.GetPlacements().begin(), f.loadout.GetPlacements().end(), [](const auto& item) { return item.moduleId.ToString() == "Module.Movement.Afterburner.Standard"; });
		report.Check(standard != f.loadout.GetPlacements().end(), "canonical loadout owns the Standard Afterburner");
		const auto standardId = standard->instanceId;
		f.Remove(standardId);
		const auto extended = f.Install("Module.Movement.Afterburner.Extended");
		const auto energy = f.ship->GetEnergyComponent().GetEnergy();
		controls->ApplyInput({ 1.f, 0.f }, true, .05f);
		report.Check(!f.ship->IsAfterburning() && blink.GetCharges() == 2, "Shift consumes one Blink charge while Afterburner is replaced");
		f.Equal("Blink never drains Afterburner energy", energy, f.ship->GetEnergyComponent().GetEnergy());
		f.Equal("capacity investment does not add Blink charges", 3.f, static_cast<float>(blink.GetMaximumCharges()));
		const float recharge = blink.GetRechargeRemaining();
		f.Remove(extended);
		f.Equal("unrelated rebuild does not reset recharge", recharge, blink.GetRechargeRemaining());
		f.Equal("unrelated rebuild does not refill charges", 2.f, static_cast<float>(blink.GetCharges()));
		f.Install("Module.Movement.Afterburner.Standard");
		f.Remove(blinkId);
		f.Equal("uninstall clears active boost", 1.f, f.ship->GetMovementSpeedMultiplier());
		controls->ApplyInput({}, false, .01f);
		controls->ApplyInput({ 0.f, -1.f }, true, .05f);
		report.Check(f.ship->IsAfterburning(), "uninstall restores the existing Afterburner");
	}

	void IonFields(Report& report, Application& application)
	{
		for (const int level : { 1, 2, 3, 4, 5 })
			for (const float speed : { 299.f, 300.f, 500.f, 700.f, 900.f, 1300.f })
			{
				MovementFixture f{ report, application, level };
				const auto instance = f.InstallMovement(kIon, level);
				const auto enemy = Enemy(f);
				const auto outside = Enemy(f);
				f.ship->SetVelocity({ speed, 0.f }); f.ship->GetMovementComponent().Tick(.25f);
				const float effective = speed <= 700.f ? speed : speed == 900.f ? 800.f : 875.f;
				const float radius = speed < 300.f ? 0.f : 300.f + (effective - 300.f) * .5f;
				enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ radius > 0.f ? radius - 5.f : 50.f, 0.f });
				outside->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ radius + 100.f, 0.f });
				f.ship->GetModuleApplier().Tick(.25f);
				f.Equal("Ion field radius follows real speed and high-speed falloff", radius, f.Ion().GetFieldRadius());
				const float tick = (6.f + 1.5f * (level - 1) + (.08f + .01f * (level - 1)) * 100.f) * effective / 300.f;
				f.Equal("Ion field damage follows EP, level and effective speed", speed < 300.f ? 0.f : tick, 1000.f - enemy->GetHealthComponent().GetHealth());
				f.Equal("one Electric stack each global tick", speed < 300.f ? 0.f : 1.f, static_cast<float>(Stacks(*enemy, kElectric)));
				f.Equal("targets beyond the radius are untouched", 1000.f, outside->GetHealthComponent().GetHealth());
				f.ship->GetModuleApplier().Tick(.25f);
				f.Equal("continued contact gains another stack on the next tick", speed < 300.f ? 0.f : 2.f, static_cast<float>(Stacks(*enemy, kElectric)));
				f.RemoveMovement(instance);
				f.Equal("uninstall closes the field", 0.f, f.Ion().GetFieldRadius());
				for (const auto& weak : f.level->GetActorsByTypeIncludingPending<IonCometVisualActor>()) if (const auto visual = weak.lock()) report.Check(visual->GetIsPendingDestroy(), "uninstall releases the owned visual");
			}
	}

	void IonChargeAndBrake(Report& report, Application& application)
	{
		for (int level = 1; level <= 5; ++level)
		{
			MovementFixture f{ report, application, level };
			f.InstallMovement(kIon, level);
			f.Flight(300.f, 1.f); f.Equal("300 speed does not fill charge", 0.f, f.Ion().GetCharge());
			f.Flight(450.f, 1.f); f.Equal("450 speed fills ten charge per second", 10.f, f.Ion().GetCharge());
			f.Flight(0.f, 1.f); f.Equal("low speed retains charge", 10.f, f.Ion().GetCharge());
			f.Flight(600.f, 4.5f); f.Equal("600 speed fills twenty per second up to 100", 100.f, f.Ion().GetCharge());
			const auto enemy = Enemy(f);
			f.ship->SetActorRotation(90.f);
			f.ship->GetMovementComponent().GetAttributes().reverseThrust.currentValue = 2500.f;
			f.ship->SetVelocity({ 500.f, 0.f });
			enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 400.f, 0.f });
			f.Control({ 0.f, 1.f }, false, .1f);
			report.Check(f.Ion().GetBurstCount() == 1, "real reverse thrust losing half speed triggers one charged burst");
			f.Equal("burst uses the pre-brake radius", 400.f, f.Ion().GetLastBurstRadius());
			const float expected = (65.f + 10.f * (level - 1) + (.50f + .05f * (level - 1)) * 100.f) * 500.f / 300.f;
			f.Equal("burst uses pre-brake effective speed and level/EP", expected, f.Ion().GetLastBurstDamage());
			f.Equal("burst actually damages a target outside the closed field", expected, 1000.f - enemy->GetHealthComponent().GetHealth());
			f.Equal("burst applies four Electric stacks", 4.f, static_cast<float>(Stacks(*enemy, kElectric)));
			f.Equal("burst resets charge", 0.f, f.Ion().GetCharge());
			f.Equal("burst is processed before closing the slow field", 0.f, f.Ion().GetFieldRadius());
			f.Control({ 0.f, 1.f }, false, .01f);
			report.Check(f.Ion().GetBurstCount() == 1, "held brake cannot repeat a spent burst");
		}
	}

	void IonRejections(Report& report, Application& application)
	{
		for (const int cause : { 0, 1, 2, 3, 4, 5, 6, 7, 8 })
		{
			MovementFixture f{ report, application };
			f.InstallMovement(kIon);
			f.InstallMovement(kBlink);
			f.Flight(600.f, cause == 6 ? 2.5f : 5.f);
			f.ship->SetActorRotation(90.f);
			f.ship->GetMovementComponent().GetAttributes().reverseThrust.currentValue = 2500.f;
			f.ship->SetVelocity({ 500.f, 0.f });
			if (cause == 0)
			{
				f.ship->SetVelocity({}); f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(.1f);
			}
			if (cause == 1)
			{
				report.Check(f.Blink().TryBlink({ 1.f, 0.f }), "charged Ion can coexist with Blink");
				f.Control({ 0.f, 1.f }, false, .1f);
			}
			if (cause == 2)
			{
				report.Check(movement::MovementInfluenceService::SetForcedMovement(*f.ship, { f.ship->GetUniqueID(), { 50.f, 0.f }, 1 }), "forced movement enters canonical movement service");
				f.Control({ 0.f, 1.f }, false, .1f);
			}
			if (cause == 3)
			{
				f.ship->GetAbilitySystemComponent().AddOwnedTag(GameplayTags::State::Effect::Control::Stunned);
				f.Control({ 0.f, 1.f }, false, .1f);
			}
			if (cause == 4)
			{
				f.ship->GetMovementComponent().GetAttributes().reverseThrust.currentValue = 700.f;
				for (int step = 0; step < 4; ++step) f.Control({ 0.f, 1.f }, false, .1f);
			}
			if (cause == 5)
			{
				const auto wall = f.level->SpawnActor<Wall>().lock();
				wall->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 45.f, 0.f });
				f.ship->SetCollisionMask(f.ship->GetCollisionMask() | GameCollision::Environment);
				f.level->TickInternal(0.f);
				f.ship->SetVelocity({ 500.f, 0.f });
				f.Control({ 0.f, 1.f }, false, .1f);
			}
			if (cause == 6) f.Control({ 0.f, 1.f }, false, .1f);
			if (cause == 7)
			{
				f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::MovementSlow, .5f);
				f.Control({ 0.f, 1.f }, false, .1f);
			}
			if (cause == 8)
			{
				report.Check(movement::MovementInfluenceService::ApplyImpulse(*f.ship, { { 1.f, 0.f }, .5f }), "knockback impulse enters canonical movement service");
				f.Control({ 0.f, 1.f }, false, .1f);
			}
			report.Check(f.Ion().GetBurstCount() == 0, "no burst from uncommanded loss, teleport, knockback, stun or braking outside the time window", cause);
		}
	}

	void IonBrakeBoundaries(Report& report, Application& application)
	{
		for (const float speed : { 300.f, 500.f, 900.f, 1300.f })
		{
			MovementFixture f{ report, application };
			f.InstallMovement(kIon);
			for (int step = 0; step < 20; ++step) f.Flight(600.f, .25f);
			const auto enemy = Enemy(f);
			f.ship->SetActorRotation(90.f); f.ship->SetVelocity({ speed, 0.f });
			const float finalSpeed = speed == 300.f ? 150.f : speed == 500.f ? 300.f : 200.f;
			f.ship->GetMovementComponent().GetAttributes().reverseThrust.currentValue = (speed - finalSpeed) / .1f;
			enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 250.f, 0.f });
			f.Control({ 0.f, 1.f }, false, .1f);
			report.Check(f.Ion().GetBurstCount() == 1, "exact 40 percent brake and high-speed brakes qualify");
			const float effective = speed <= 700.f ? speed : speed == 900.f ? 800.f : 875.f;
			f.Equal("burst snapshot applies the high-speed falloff", 115.f * effective / 300.f, f.Ion().GetLastBurstDamage());
			f.Equal("burst radius snapshots the high-speed falloff", 300.f + (effective - 300.f) * .5f, f.Ion().GetLastBurstRadius());
			f.Equal("burst tick starts again from zero charge", 0.f, f.Ion().GetCharge());
			f.Equal("exact-boundary burst has four stacks before the next combat tick", 4.f, static_cast<float>(Stacks(*enemy, kElectric)));
		}
		MovementFixture f{ report, application };
		f.InstallMovement(kIon); f.Flight(600.f, 5.f);
		f.ship->SetVelocity({ 500.f, 0.f });
		f.ship->GetMovementComponent().GetAttributes().reverseThrust.currentValue = 800.f;
		for (int step = 0; step < 3; ++step)
		{
			f.ship->SetActorRotation(90.f); f.Control({ 0.f, 1.f }, false, .1f);
			report.Check(f.Ion().GetBurstCount() == (step == 2 ? 1u : 0u), "brake loss accumulates across exactly the 0.30 second window");
		}
		f.Equal("multi-frame brake retains the initial pre-brake radius", 400.f, f.Ion().GetLastBurstRadius());
	}

	void MovementIntegrations(Report& report, Application& application)
	{
		MovementFixture f{ report, application };
		f.InstallMovement("Module.Movement.AssaultVector.Epic");
		f.InstallMovement("Module.Movement.PhaseSlip.Epic");
		f.InstallMovement(kBlink);
		f.InstallMovement(kIon);
		const auto enemy = Enemy(f, 700.f);
		f.Flight(0.f, .1f);
		f.Control({ 1.f, 0.f }, true, 0.f);
		f.Flight(0.f, .25f);
		f.Equal("Blink cannot activate Assault Vector as travelled distance", 0.f, f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().GetAssaultDamageBonus());
		report.Check(!f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().IsPhaseActive(), "Blink cannot start the Afterburner-only Phase Slip");
		f.Equal("stationary Blink cannot open Ion field", 0.f, f.Ion().GetFieldRadius());
		f.Equal("stationary Blink cannot fill Ion charge", 0.f, f.Ion().GetCharge());
		f.Flight(200.f, .1f);
		report.Check(f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().GetAssaultDamageBonus() > 0.f, "real flight after Blink still activates Assault Vector");
		const auto refill = f.Blink().GetRechargeRemaining();
		f.ship->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::AfterburnerRegen, 10000.f);
		f.ship->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::AfterburnerCapacity, 10000.f);
		f.Blink().Tick(.5f);
		f.Equal("Afterburner regeneration investments cannot advance the Blink clock", refill - .5f, f.Blink().GetRechargeRemaining());
		f.Equal("Afterburner capacity investments cannot add charges", 3.f, static_cast<float>(f.Blink().GetMaximumCharges()));
	}

	void HUDIntegration(Report& report, Application& application)
	{
		auto& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		struct Reset { PlayerManager& manager; ~Reset() { manager.Reset(); } } reset{ manager };
		auto level = std::make_shared<HeadlessModuleLevel>(&application);
		level->BeginPlayInternal();
		auto& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr, "HUD fixture spawns the current Player ship");
		level->TickInternal(0.f);
		const auto rules = player.BuildModuleRules();
		report.Check(rules.has_value(), "HUD fixture uses real player schematic");
		const auto instance = player.AddModuleToInventory(kBlink, 0, ModuleRarity::Legendary);
		const auto placement = FindPlacement(*rules, player.GetModuleLoadout(), *rules->lookup(kBlink), instance);
		report.Check(placement && player.TryInstallModule(instance, placement->first, placement->second) == ModulePlacementFailure::None, "HUD fixture installs Blink through Player");
		VitalsPresenter presenter;
		presenter.Tick();
		const auto model = presenter.GetViewModel();
		report.Check(model->blinkDrive && model->blinkCharges == 3 && model->blinkMaximumCharges == 3, "Afterburner readout exposes three Blink charges");
		ship->GetModuleApplier().GetLegendaryBlinkDriveRuntime().TryBlink({ 1.f, 0.f });
		presenter.Tick();
		report.Check(model->blinkCharges == 2 && model->blinkRechargeRemaining > 0.f, "HUD follows spent charges and sequential refill");
		VitalsView view{ application.GetEngineContext(), model };
		view.Tick(0.f);
		report.Check(view.GetRefreshCount() == 1, "Blink charge gauge and disabled-investment notice render through VitalsView");
		report.Check(player.TryUninstallModule(instance), "HUD fixture uninstalls Blink");
		presenter.Tick(); view.Tick(0.f);
		report.Check(!model->blinkDrive && model->blinkCharges == 0, "HUD returns to the ordinary Afterburner gauge on uninstall");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "register shipped content");
		std::string title = "Legendary Movement E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		BlinkLevels(report, application); BlinkDirectionsAndObstacles(report, application); BlinkReplacement(report, application); IonFields(report, application); IonChargeAndBrake(report, application); IonRejections(report, application); IonBrakeBoundaries(report, application); MovementIntegrations(report, application); HUDIntegration(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryMovementModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
