#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP5E2E.h"
#include "presentation/hud/ability/AbilityBarPresenter.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include <SFML/Window/Event.hpp>
#include <iostream>

namespace
{
	using namespace uncommon_p5_e2e;
	constexpr const char* HardpointId = "Module.Primary.SecondHardpoint.Legendary";
	constexpr const char* MeltdownId = "Module.Primary.MeltdownBarrel.Legendary";
	constexpr const char* Rapid = "Weapon.Projectile.FighterRapidLaser.Basic";
	constexpr const char* Cone = "Weapon.Cone.RapidShotgun.Basic";
	constexpr const char* Beam = "Weapon.Beam.ContinuousHeatLaser.Basic";
	constexpr const char* Wave = "Weapon.Wave.CryoProjector.Basic";
	constexpr const char* Twin = "Weapon.Projectile.DualKineticBlaster.Basic";

	bool Near(float a, float b, float tolerance = .01f) { return std::abs(a - b) <= tolerance; }
	void Step(Fixture& f, float dt)
	{
		f.ship->GetAbilitySystemComponent().Tick(dt);
		f.ship->GetModuleApplier().Tick(dt);
	}

	struct FireRig
	{
		Fixture& f;
		PrimaryWeaponDefinition weapon;
		sas::GameplayAttributeList attributes;
		PrimaryWeaponRuntimeState state;
		FireRig(Fixture& fixture, const char* id = Rapid) : f{ fixture }, weapon{ *content::WeaponContentCatalog::FindById(id) }
		{
			const auto* primary = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
			AbilityExecutionContext resolve{ &f.ship->GetAbilitySystemComponent(), &primary->GetDefinition() };
			attributes = AbilityActionAttributeResolver::ResolveAttributes(resolve, &weapon, weapon.attributes);
			f.report.Check(PrimaryWeaponExecutionSystem::InitializeRuntime(weapon, state).isValid, "initialize shipped weapon runtime");
		}
		PrimaryWeaponExecutionContext Context()
		{
			PrimaryWeaponExecutionContext context{ *f.ship, weapon, attributes, weapon.damageTags };
			context.runtime = &state;
			return context;
		}
		bool Fire()
		{
			auto context = Context();
			PrimaryWeaponExecutionSystem::BeginFire(context, state);
			return PrimaryWeaponExecutionSystem::FireOnce(context, state);
		}
		void Release() { PrimaryWeaponExecutionSystem::EndFire(Context(), state); }
		void Cool(float dt) { Release(); PrimaryWeaponExecutionSystem::TickInactive(Context(), state, dt); }
		~FireRig() { Release(); }
	};

	float WeaponHit(Fixture& f, SpaceShip& target, const char* weaponId, DamageDeliveryType delivery = DamageDeliveryType::Projectile, bool generated = false)
	{
		auto payload = Plain();
		payload.primaryWeaponId = sas::ContentId{ weaponId };
		payload.hitGroup = std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot);
		if (generated) { payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed; payload.skipModuleDamageRules = true; }
		return Hit(target, f.ship.get(), 10.f, payload, {}, {}, delivery);
	}

	void HardpointChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		report.Check(!f.ship->AssignSecondPrimaryWeapon(Cone), "reserve assignment requires the installed module");
		const auto installed = f.Install(HardpointId);
		report.Check(!f.ship->AssignSecondPrimaryWeapon("Weapon.Missing") && !f.ship->AssignSecondPrimaryWeapon(Rapid), "assignment validates catalog id and keeps the hull weapon first");
		report.Check(f.ship->AssignSecondPrimaryWeapon(Cone), "debug/run API assigns a catalog reserve");
		auto& abilities = f.ship->GetAbilitySystemComponent();
		auto* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		Step(f, .01f);
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		const int rounds = primary->GetPrimaryWeaponRuntime().magazineState.roundsRemaining;
		report.Check(rounds == 47, "hull Primary fires before the switch");
		const sf::Event wheel{ sf::Event::MouseWheelScrolled{ sf::Mouse::Wheel::Vertical, 1.f, { 0, 0 } } };
		report.Check(f.ship->HandleWeaponInput(wheel), "real mouse-wheel event enters PlayerSpaceShip switch path");
		auto* hardpoint = abilities.GetSecondHardpointRuntime();
		report.Check(hardpoint->IsSecondWeaponActive() && Near(hardpoint->GetSwitchRemaining(), 1.f), "wheel selects reserve with fixed one-second lock");
		report.Check(!f.ship->HandleWeaponInput(wheel), "wheel cannot restart an in-progress switch");
		report.Check(primary->GetPrimaryWeaponRuntime().magazineState.roundsRemaining == rounds, "switch does not refill a partial magazine");
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		Step(f, .5f);
		report.Check(hardpoint->GetSecondWeaponState().runtime.successfulFireCount == 0, "no firing halfway through switch");
		Step(f, .49f);
		report.Check(hardpoint->GetSecondWeaponState().runtime.successfulFireCount == 0 && hardpoint->IsSwitching(), "no firing before one second");
		Step(f, .02f);
		Step(f, .01f);
		report.Check(hardpoint->GetSecondWeaponState().runtime.successfulFireCount > 0, "held input fires only the reserve after lock");
		report.Check(primary->GetPrimaryWeaponRuntime().successfulFireCount == 1, "only one weapon fires at a time");
		for (int i = 0; i < 12; ++i) Step(f, .1f);
		report.Check(primary->GetPrimaryWeaponRuntime().magazineState.roundsRemaining == 48, "reserve partial magazine reloads at natural speed while other weapon fires");
		const auto& reserve = hardpoint->GetSecondWeaponState().runtime;
		report.Check(reserve.magazineState.roundsRemaining < hardpoint->GetSecondWeapon()->magazine->capacity && reserve.magazineState.reloadRemaining == 0.0, "partial active magazine does not reload while firing");
		report.Check(BuildPrimaryWeaponReadout(*f.ship).find("[2/2]") != std::string::npos, "HUD identifies selected reserve");
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		f.Remove(installed);
		report.Check(!abilities.GetSecondHardpointRuntime() && !f.ship->HandleWeaponInput(wheel), "uninstall detaches reserve and wheel hook");
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		Step(f, .3f);
		report.Check(primary->GetPrimaryWeaponRuntime().successfulFireCount > 1, "uninstall restores hull Primary firing");
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
	}

	void MarkChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto installed = f.Install(HardpointId, 0, level);
			report.Check(f.ship->AssignSecondPrimaryWeapon(Cone), "mark test assigns real second weapon");
			const int effective = level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), installed);
			const float expected = 10.f * (1.20f + .05f * (effective - 1));
			const auto target = Enemy(f), other = Enemy(f, 100.f);
			f.Equal("unmarked first hit is normal", 10.f, WeaponHit(f, *target, Rapid));
			f.Equal("cross-weapon mark scales with effective level", expected, WeaponHit(f, *target, Cone));
			f.Equal("mark is not consumed", expected, WeaponHit(f, *target, Cone));
			f.Equal("bonus hit renews the opposite mark", expected, WeaponHit(f, *target, Rapid));
			f.Equal("marks are per target", 10.f, WeaponHit(f, *other, Cone));
			f.Equal("DoT does not use mark bonus", 10.f, WeaponHit(f, *target, Cone, DamageDeliveryType::Periodic));
			f.Equal("generated module damage does not use mark bonus", 10.f, WeaponHit(f, *target, Cone, DamageDeliveryType::Area, true));
			f.ship->GetModuleApplier().Tick(4.99f);
			f.Equal("mark lasts the fixed five seconds", expected, WeaponHit(f, *target, Cone));
			f.ship->GetModuleApplier().Tick(.02f);
			f.Equal("a hit does not renew its own mark", 10.f, WeaponHit(f, *target, Cone));
			f.ship->GetModuleApplier().Tick(5.01f);
			WeaponHit(f, *target, Rapid, DamageDeliveryType::Periodic);
			WeaponHit(f, *target, Rapid, DamageDeliveryType::Area, true);
			f.Equal("DoT and generated hits do not create marks", 10.f, WeaponHit(f, *target, Cone));
		}
	}

	void ReserveHeatChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(HardpointId);
		report.Check(f.ship->AssignSecondPrimaryWeapon(Beam), "hardpoint can carry a natural heat weapon");
		auto& abilities = f.ship->GetAbilitySystemComponent();
		auto* hardpoint = abilities.GetSecondHardpointRuntime();
		report.Check(hardpoint->SwitchWeapon(), "select heat reserve");
		Step(f, 1.01f);
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		for (int i = 0; i < 12; ++i) Step(f, .05f);
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		const auto heatKey = PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue;
		const float heat = hardpoint->GetSecondWeaponState().runtime.GetFeatureValue(heatKey);
		report.Check(heat > 0.f && hardpoint->SwitchWeapon(), "switch away from a partly heated laser");
		report.Check(Near(hardpoint->GetSecondWeaponState().runtime.GetFeatureValue(heatKey), heat), "switch never instantly clears natural heat");
		Step(f, .5f);
		report.Check(Near(hardpoint->GetSecondWeaponState().runtime.GetFeatureValue(heatKey), std::max(0.f, heat - 12.5f)), "reserve laser keeps natural 25 heat per second dissipation");
	}

	void NovaLevelChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto installed = f.Install(MeltdownId, 0, level);
			const int effective = level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), installed);
			const auto rule = f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.meltdownBarrels.front();
			report.Check(Near(rule.novaBudgetFraction, .30f + .04f * (effective - 1)) && Near(rule.novaRadius, 500.f) && rule.igniteStacks == 4 && Near(rule.focusSeconds, 2.f), "only nova budget fraction scales with effective level");
			FireRig gun{ f };
			for (int i = 0; i < 48; ++i) report.Check(gun.Fire(), "leveled magazine cycle fires");
			const float normal = sas::FindAttributeValue(gun.attributes, CommonAttributeIds::Damage);
			report.Check(Near(gun.state.meltdownCycle->pendingDamage, normal * 48.f * (.30f + .04f * (effective - 1)), .05f), "nova snapshot uses effective module level fraction");
		}
	}

	void MagazineMeltdownChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto installed = f.Install(MeltdownId);
		const auto rule = f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.meltdownBarrels.front();
		FireRig gun{ f };
		const auto near = Enemy(f, 490.f), outside = Enemy(f, 510.f);
		for (int shot = 0; shot < 48; ++shot)
		{
			report.Check(gun.Fire(), "magazine trigger produces a shot");
			report.Check(Near(gun.state.meltdownCycle->heat, (shot + 1) * 100.f / 48.f), "one heat increment per trigger");
		}
		const auto cycle = gun.state.meltdownCycle;
		const float expected = 48.f * sas::FindAttributeValue(gun.attributes, CommonAttributeIds::Damage) * rule.novaBudgetFraction;
		report.Check(Near(cycle->pendingDamage, expected), "Q counts potential normal damage even when every projectile misses");
		report.Check(cycle->budget == 0.f && Near(cycle->focusRemaining, 2.f) && !gun.Fire(), "full heat snapshots Q and locks overheated weapon");
		report.Check(BuildPrimaryWeaponReadout(*f.ship).find("Heat") != std::string::npos, "HUD uses heat while Meltdown is installed");
		gun.Cool(1.99f);
		f.ship->GetModuleApplier().Tick(1.99f);
		report.Check(near->GetHealthComponent().GetHealth() == 1000.f, "nova waits until end of focus");
		report.Check(cycle->heat < 1.f && Near(cycle->pendingDamage, expected), "normal cooling runs in parallel without reducing pending snapshot");
		// Position at the end of focus, not the initial overheat location.
		f.ship->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 1000.f, 0.f });
		near->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 499.f, 0.f });
		outside->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 501.f, 0.f });
		gun.Cool(.02f);
		f.ship->GetModuleApplier().Tick(.02f);
		report.Check(Near(1000.f - near->GetHealthComponent().GetHealth(), expected), "Thermal nova uses snapshot damage at final ship position");
		report.Check(outside->GetHealthComponent().GetHealth() == 1000.f, "nova radius is fixed at 500");
		report.Check(Stacks(*near, DamageStatusEffectIds::IgniteEffectId) == 4, "nova applies exactly four Ignite stacks");
		report.Check(cycle->pendingDamage == 0.f && cycle->budget == 0.f && cycle->heat == 0.f, "nova consumes its snapshot once and cooling clears cycle budget");
		report.Check(gun.Fire(), "focus and reload run concurrently rather than additively");
		f.Remove(installed);
		report.Check(!f.ship->GetAbilitySystemComponent().GetMeltdownBarrelRuntime() && !cycle->active, "uninstall detaches heat/nova hooks");
		report.Check(BuildPrimaryWeaponReadout(*f.ship).empty(), "uninstall clears module weapon readout");
		const int before = gun.state.magazineState.roundsRemaining;
		report.Check(gun.Fire() && gun.state.magazineState.roundsRemaining == before - 1, "normal magazine behaviour restored after uninstall");
	}

	void CoolingAndTriggerChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(MeltdownId);
		FireRig gun{ f };
		for (int i = 0; i < 12; ++i) report.Check(gun.Fire(), "partial cycle fires");
		const auto cycle = gun.state.meltdownCycle;
		const float before = cycle->budget;
		gun.Cool(.25f);
		report.Check(Near(cycle->heat, 12.5f) && Near(cycle->budget, before / 2.f), "Q decays proportionally with partial-cycle cooling");
		gun.Cool(.25f);
		report.Check(cycle->heat == 0.f && cycle->budget == 0.f, "fully cooled cycle cannot bank old damage");
		FireRig twin{ f, Twin };
		report.Check(twin.Fire(), "multi-projectile magazine weapon fires");
		const int muzzles = twin.weapon.alternateMuzzles ? 1 : static_cast<int>(twin.weapon.muzzleDefinitions.size());
		const int triggers = (twin.weapon.magazine->capacity + muzzles - 1) / muzzles;
		report.Check(Near(twin.state.meltdownCycle->heat, 100.f / triggers), "muzzles and pellets do not multiply heat increments");
		const float normal = sas::FindAttributeValue(twin.attributes, CommonAttributeIds::Damage);
		report.Check(Near(twin.state.meltdownCycle->budget, normal * muzzles), "Q counts a natural twin volley once per emitted projectile");
		// A fractional recovery must leave heat authoritative, even close to full.
		gun.state.meltdownCycle->heat = 99.f;
		gun.state.magazineState.roundsRemaining = 1;
		report.Check(gun.Fire() && Near(gun.state.meltdownCycle->heat, 100.f), "fractional heat reaches overheat without a separate empty-ammo gate");
	}

	void BudgetAndEmpoweredChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(MeltdownId);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		FireRig baseline{ f };
		const float unboosted = sas::FindAttributeValue(baseline.attributes, CommonAttributeIds::Damage);
		const auto modifier = abilities.AddPrimaryWeaponModifier({ CommonAttributeIds::Damage, sas::AttributeModifierOperation::Multiply, 3.f });
		FireRig gun{ f };
		const float boosted = sas::FindAttributeValue(gun.attributes, CommonAttributeIds::Damage);
		report.Check(boosted > unboosted, "module modifier increases actual weapon damage");
		for (int i = 0; i < 42; ++i) report.Check(gun.Fire(), "boosted cycle produces normal triggers");
		report.Check(Near(gun.state.meltdownCycle->heat, 87.5f), "final six-round empowered window starts at equivalent heat");
		report.Check(Near(gun.state.meltdownCycle->budget, unboosted * 42.f, .05f), "Q excludes Primary module damage modifiers");
		report.Check(gun.Fire(), "first final-window shot fires");
		f.level->TickInternal(0.f);
		bool empowered = false, identity = false;
		for (const auto& weak : f.level->GetActorsByType<PrimaryWeaponProjectileActor>())
			if (const auto shot = weak.lock())
			{
				empowered = empowered || shot->IsEmpowered();
				identity = identity || shot->GetDamagePayload().primaryWeaponId.ToString() == Rapid;
			}
		report.Check(empowered && identity, "real projectile preserves empowered heat window and delayed weapon identity");
		const auto cycle = gun.state.meltdownCycle;
		for (int i = 43; i < 48; ++i) report.Check(gun.Fire(), "empowered cycle completes");
		const auto fraction = f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.meltdownBarrels.front().novaBudgetFraction;
		report.Check(Near(cycle->pendingDamage, unboosted * 48.f * fraction, .05f), "Q excludes empowered and critical multipliers from final shots");
		abilities.RemovePrimaryWeaponModifier(modifier);
	}

	void InputFocusChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(HardpointId);
		f.Install(MeltdownId);
		report.Check(f.ship->AssignSecondPrimaryWeapon(Cone), "input focus test assigns reserve");
		auto& abilities = f.ship->GetAbilitySystemComponent();
		auto* primary = abilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		// Spawning a target ticks the real ship's input poll; do this before synthetic held input.
		const auto target = Enemy(f, 300.f);
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		for (int i = 0; i < 400; ++i)
		{
			Step(f, .05f);
			if (primary->GetPrimaryWeaponRuntime().meltdownCycle && primary->GetPrimaryWeaponRuntime().meltdownCycle->focusRemaining > 0.f) break;
		}
		const auto cycle = primary->GetPrimaryWeaponRuntime().meltdownCycle;
		report.Check(cycle && Near(cycle->focusRemaining, 2.f) && primary->GetPrimaryWeaponRuntime().successfulFireCount == 48, "held input overheats exactly one natural magazine and starts full focus");
		const sf::Event wheel{ sf::Event::MouseWheelScrolled{ sf::Mouse::Wheel::Vertical, 1.f, { 0, 0 } } };
		report.Check(f.ship->HandleWeaponInput(wheel), "real active weapon switches during its focus");
		for (int i = 0; i < 30; ++i) Step(f, .05f);
		report.Check(abilities.GetSecondHardpointRuntime()->GetSecondWeaponState().runtime.successfulFireCount > 0 && cycle->focusRemaining > 0.f, "input fires reserve while first weapon focus and cooling continue");
		for (int i = 0; i < 11; ++i) Step(f, .05f);
		report.Check(target->GetHealthComponent().GetHealth() < 1000.f && Stacks(*target, DamageStatusEffectIds::IgniteEffectId) == 4, "actual switched weapon detonates on schedule through ability and module ticks");
		report.Check(cycle->heat == 0.f && cycle->pendingDamage == 0.f, "reserve weapon cools fully while its focus completes");
		abilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
	}

	void BeamAndUnsupportedChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(MeltdownId);
		FireRig beam{ f, Beam };
		auto context = beam.Context();
		PrimaryWeaponExecutionSystem::BeginFire(context, beam.state);
		float expectedBudget = 0.f;
		const float dps = sas::FindAttributeValue(beam.attributes, CommonAttributeIds::Damage);
		for (int i = 0; i < 200 && beam.state.requestedCooldown == 0.f; ++i)
		{
			const float heat = beam.state.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue);
			float band = 1.f;
			for (const auto& region : beam.weapon.heatDamageRegions) { band = region.damageMultiplier; if (heat < region.upToHeatPercentage) break; }
			expectedBudget += dps * band * .05f;
			PrimaryWeaponExecutionSystem::TickFire(context, beam.state, .05f);
		}
		report.Check(beam.state.requestedCooldown > 0.f && beam.state.meltdownCycle->focusRemaining > 0.f, "Heat Laser natural overheat arms nova");
		const auto rule = f.ship->GetModuleApplier().GetResolvedEffects().legendaryRules.meltdownBarrels.front();
		report.Check(Near(beam.state.meltdownCycle->pendingDamage, expectedBudget * rule.novaBudgetFraction, .03f), "beam Q includes natural heat bands without target or module multipliers");
		report.Check(beam.state.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue) == 0.f && Near(beam.state.requestedCooldown, 2.5f), "Heat Laser keeps its own heat reset and natural overheat lock");
		const auto target = Enemy(f, 300.f);
		beam.Cool(2.f);
		f.ship->GetModuleApplier().Tick(2.f);
		report.Check(target->GetHealthComponent().GetHealth() < 1000.f && Stacks(*target, DamageStatusEffectIds::IgniteEffectId) == 4, "Heat Laser nova damages and applies four Ignite");
		report.Check(!f.ship->GetAbilitySystemComponent().GetMeltdownBarrelRuntime()->CanFire(beam.state), "long natural cooling keeps beam locked after two-second focus");
		f.ship->GetModuleApplier().Tick(.5f);
		report.Check(f.ship->GetAbilitySystemComponent().GetMeltdownBarrelRuntime()->CanFire(beam.state), "beam fire lock ends at max(focus, natural cooling)");
		FireRig unsupported{ f, Wave };
		report.Check(!unsupported.weapon.magazine && unsupported.weapon.featureTypes.end() == std::find(unsupported.weapon.featureTypes.begin(), unsupported.weapon.featureTypes.end(), PrimaryWeaponFeatureType::Heat), "unsupported fixture has neither magazine nor heat");
		report.Check(unsupported.Fire() && !unsupported.state.meltdownCycle, "Meltdown leaves unsupported weapons unchanged");
	}

	void SwitchFocusChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(HardpointId);
		const auto meltdown = f.Install(MeltdownId);
		report.Check(f.ship->AssignSecondPrimaryWeapon(Cone), "combined modules assign reserve");
		FireRig gun{ f };
		for (int i = 0; i < 48; ++i) report.Check(gun.Fire(), "combined modules overheat first weapon");
		const auto target = Enemy(f, 300.f);
		const sf::Event wheel{ sf::Event::MouseWheelScrolled{ sf::Mouse::Wheel::Vertical, -1.f, { 0, 0 } } };
		report.Check(f.ship->HandleWeaponInput(wheel), "player may switch during focus");
		gun.Cool(1.01f);
		f.ship->GetModuleApplier().Tick(1.01f);
		f.ship->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		Step(f, .01f);
		report.Check(f.ship->GetAbilitySystemComponent().GetSecondHardpointRuntime()->GetSecondWeaponState().runtime.successfulFireCount > 0, "reserve fires while first weapon still focuses");
		gun.Cool(.99f);
		f.ship->GetModuleApplier().Tick(.99f);
		report.Check(target->GetHealthComponent().GetHealth() < 1000.f && Stacks(*target, DamageStatusEffectIds::IgniteEffectId) == 4, "switching during focus preserves detonation");
		f.ship->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		gun.Cool(2.f);
		for (int i = 0; i < 48; ++i) report.Check(gun.Fire(), "second cycle arms pending nova");
		f.Remove(meltdown);
		const float health = target->GetHealthComponent().GetHealth();
		f.ship->GetModuleApplier().Tick(3.f);
		report.Check(target->GetHealthComponent().GetHealth() == health, "uninstall cancels pending nova safely");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context;
	vega::EngineContext::Scope scope{ context };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	report.artifact["test"] = "LegendaryWeaponModulesE2E";
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Legendary weapon content registration failed" };
		std::string title = "Legendary Weapon E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		HardpointChecks(report, application);
		MarkChecks(report, application);
		ReserveHeatChecks(report, application);
		NovaLevelChecks(report, application);
		MagazineMeltdownChecks(report, application);
		CoolingAndTriggerChecks(report, application);
		BudgetAndEmpoweredChecks(report, application);
		BeamAndUnsupportedChecks(report, application);
		SwitchFocusChecks(report, application);
		InputFocusChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
