#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"
#include "gameplay/control/ControlEffects.h"
#include "gameplay/damage/StatusReactions.h"
#include "gameplay/tags/ability/NullPulseTags.h"
#include "gameplay/weapon/ShotDeviation.h"
#include "gameConfigs/combat/EffectConfig.h"

#include <cmath>

namespace
{
	using namespace uncommon_p4_e2e;

	constexpr char kReactionCore[] = "Module.Offense.ReactionCore.Legendary";

	struct ReactionFixture : Fixture
	{
		std::uint32_t instance = 0;
		int effectiveLevel = 1;
		ReactionFixture(Report& result, Application& application, int levelValue) : Fixture{ result, application }
		{
			// No default modules: their status rules (durations, freezes, consumes) would blur the reaction numbers.
			const auto defaults = loadout.GetPlacements();
			for (const auto& placement : defaults) report.Check(loadout.Uninstall(placement.instanceId), "remove default module");
			report.Check(ship->GetModuleApplier().Apply(loadout, rules), "apply empty loadout");
			instance = Install(kReactionCore, 0, levelValue);
			effectiveLevel = levelValue + ModuleResolver::FindZoneLevelBonus(ship->GetModuleApplier().GetResolvedEffects(), instance);
		}
		const LegendaryReactionCoreRuntime& Runtime() const { return ship->GetModuleApplier().GetLegendaryReactionCoreRuntime(); }
		const ResolvedModuleLegendaryReactionCoreRule& Rule() const { return Runtime().GetRules().front(); }
		int Count(StatusReactionKind kind) const { return Runtime().GetReactionCount(kind); }
		void Process() { ship->GetModuleApplier().Tick(0.f); }
		void Advance(float seconds) { ship->GetModuleApplier().Tick(seconds); }
		float Stat(const sas::AttributeId& id) const { return Owner(id); }
		shared_ptr<SpaceShip> Target(const sf::Vector2f& location)
		{
			auto enemy = Enemy(*this);
			enemy->SetActorLocation(location);
			return enemy;
		}
	};

	DamagePayload StatusPayload(const GameplayTag& damageType, int stacks)
	{
		DamagePayload payload = Plain();
		if (damageType == DamageTypeSchema::Thermal) payload.igniteStacks = stacks;
		else if (damageType == DamageTypeSchema::Cryo) payload.cryoBuildupPerHit = stacks;
		else if (damageType == DamageTypeSchema::Electric) payload.electricStacks = stacks;
		else if (damageType == DamageTypeSchema::Kinetic) payload.kineticStacks = stacks;
		return payload;
	}
	// Setup statuses come from no source, so they never start a reaction themselves.
	void Give(Actor& target, const GameplayTag& damageType, int stacks = 1) { ApplyCombatDamage(target, 1.f, nullptr, { damageType }, StatusPayload(damageType, stacks)); }
	// A real owner hit adding one stack.
	void Strike(ReactionFixture& f, Actor& target, const GameplayTag& damageType, DamageDeliveryType delivery = DamageDeliveryType::Direct) { ApplyCombatDamage(target, 1.f, f.ship.get(), { damageType }, StatusPayload(damageType, 1), {}, { GameplayTags::Ability::Offense }, delivery); }

	sas::ActiveGameplayEffect* Effect(SpaceShip& target, const char* id) { return target.GetAbilitySystemComponent().FindGameplayEffectById(id); }
	bool Has(SpaceShip& target, const char* id) { return Effect(target, id) != nullptr; }
	float Health(SpaceShip& target) { return target.GetHealthComponent().GetHealth(); }
	float Armor(SpaceShip& target) { return target.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor); }
	float Distance(const Actor& a, const sf::Vector2f& b) { const auto d = a.GetActorLocation() - b; return std::sqrt(d.x * d.x + d.y * d.y); }

	const char* const kThermal = DamageStatusEffectIds::IgniteEffectId;
	const char* const kCryo = DamageStatusEffectIds::CryoSlowedEffectId;
	const char* const kElectric = DamageStatusEffectIds::ElectricEffectId;
	const char* const kKinetic = DamageStatusEffectIds::KineticEffectId;

	void ValueChecks(Report& report, ReactionFixture& f)
	{
		const float n = static_cast<float>(f.effectiveLevel - 1);
		const auto& rule = f.Rule();
		const std::string input = "effective L" + std::to_string(f.effectiveLevel);
		report.CheckValue("Steam seconds 2 + 0.2n", 2.f + .2f * n, rule.steamSeconds, input);
		report.CheckValue("Steam deviation 25 degrees", 25.f, rule.steamDeviationDegrees, input);
		report.CheckValue("Magnet speed 300 + 40n", 300.f + 40.f * n, rule.magnetSpeed, input);
		report.CheckValue("Plasma base 60 + 12n", 60.f + 12.f * n, rule.plasmaDamage, input);
		report.CheckValue("Plasma EP coefficient 0.60 + 0.06n", .6f + .06f * n, rule.plasmaEnergyPowerCoefficient, input);
		report.CheckValue("Fracture seconds 2 + 0.15n", 2.f + .15f * n, rule.fractureSeconds, input);
		report.CheckValue("Superconduct stun 1 + 0.1n", 1.f + .1f * n, rule.stunSeconds, input);
		report.CheckValue("Shrapnel base 10 + 2n", 10.f + 2.f * n, rule.shrapnelDamage, input);
		report.CheckValue("Shrapnel AP coefficient 0.10 + 0.01n", .1f + .01f * n, rule.shrapnelAttackPowerCoefficient, input);
		report.CheckValue("Shrapnel EP coefficient 0.10 + 0.01n", .1f + .01f * n, rule.shrapnelEnergyPowerCoefficient, input);
		report.Check(rule.shrapnelCount == 6 && rule.shrapnelMaxHitsPerTarget == 2, "Shrapnel 6 pieces, at most 2 per target", input);
	}

	void ReactionEffects(Report& report, Application& application, int levelValue)
	{
		ReactionFixture f{ report, application, levelValue };
		ValueChecks(report, f);
		const auto& rule = f.Rule();
		const std::string input = "effective L" + std::to_string(f.effectiveLevel);
		const sf::Vector2f base = f.ship->GetActorLocation() + sf::Vector2f{ 3000.f, 0.f };

		// Steam: Thermal on the target, a new Cryo stack from the owner.
		{
			const auto target = f.Target(base);
			Give(*target, DamageTypeSchema::Thermal);
			Strike(f, *target, DamageTypeSchema::Cryo);
			report.Check(Has(*target, kThermal) && Has(*target, kCryo), "the triggering hit finishes its own status before the reaction", input);
			f.Process();
			report.Check(f.Count(StatusReactionKind::Steam) == 1, "Thermal + Cryo fires Steam", input);
			report.Check(!Has(*target, kThermal) && !Has(*target, kCryo), "Steam consumes both statuses completely", input);
			report.CheckValue("Steam deviation bound on the target", rule.steamDeviationDegrees, GetShotDeviationDegrees(*target), input);
			const auto* steam = Effect(*target, ShotDeviationIds::EffectId);
			report.CheckValue("Steam lasts its level seconds", rule.steamSeconds, steam ? steam->remainingDuration : 0.f, input);
			bool bounded = true, varied = false;
			for (int sample = 0; sample < 64; ++sample)
			{
				const float angle = SampleShotDeviationDegrees(*target);
				bounded = bounded && std::abs(angle) <= rule.steamDeviationDegrees + 1e-4f;
				varied = varied || std::abs(angle) > 1.f;
			}
			report.Check(bounded && varied, "newly fired shots deviate randomly within +/-25 degrees", input);
			target->Destroy();
		}

		// Magnet: Electric + Kinetic pulls enemies within the radius toward the target; bosses and far enemies stay.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 1000.f });
			const auto near = f.Target(base + sf::Vector2f{ 200.f, 1000.f });
			const auto far = f.Target(base + sf::Vector2f{ rule.magnetRadius + 150.f, 1000.f });
			const auto boss = f.Target(base + sf::Vector2f{ -200.f, 1000.f });
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			Give(*target, DamageTypeSchema::Electric);
			Strike(f, *target, DamageTypeSchema::Kinetic);
			f.Process();
			report.Check(f.Count(StatusReactionKind::Magnet) == 1 && !Has(*target, kElectric) && !Has(*target, kKinetic), "Electric + Kinetic fires Magnet and consumes both", input);
			const float nearBefore = Distance(*near, target->GetActorLocation()), farBefore = Distance(*far, target->GetActorLocation()), bossBefore = Distance(*boss, target->GetActorLocation());
			for (int step = 0; step < 10; ++step) f.level->TickInternal(.05f);
			const float pulled = nearBefore - Distance(*near, target->GetActorLocation());
			report.Check(std::abs(rule.magnetSpeed * .5f - pulled) <= rule.magnetSpeed * .5f * .05f, "Magnet pulls a nearby enemy at its level speed for 0.5 s", Json{ { "expected", rule.magnetSpeed * .5f }, { "actual", pulled }, { "input", input } });
			report.Check(std::abs(farBefore - Distance(*far, target->GetActorLocation())) < 1.f, "Magnet leaves enemies outside its radius", input);
			report.Check(std::abs(bossBefore - Distance(*boss, target->GetActorLocation())) < 1.f, "Magnet respects boss crowd-control immunity", input);
			const sf::Vector2f settled = near->GetActorLocation();
			for (int step = 0; step < 6; ++step) f.level->TickInternal(.05f);
			report.Check(Distance(*near, settled) < 1.f, "Magnet releases its targets after 0.5 s", input);
			for (const auto& actor : { target, near, far, boss }) actor->Destroy();
		}

		// Plasma: Thermal + Electric, area Thermal damage (60 + 12n) + (0.60 + 0.06n) x EP; no new status.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 2000.f });
			const auto inside = f.Target(base + sf::Vector2f{ 200.f, 2000.f });
			const auto outside = f.Target(base + sf::Vector2f{ rule.plasmaRadius + 150.f, 2000.f });
			Give(*target, DamageTypeSchema::Thermal);
			Strike(f, *target, DamageTypeSchema::Electric);
			const float targetBefore = Health(*target), insideBefore = Health(*inside), outsideBefore = Health(*outside);
			f.Process();
			const float expected = rule.plasmaDamage + rule.plasmaEnergyPowerCoefficient * f.Stat(OwnerAttributeIds::EnergyPower);
			report.Check(f.Count(StatusReactionKind::Plasma) == 1 && !Has(*target, kThermal) && !Has(*target, kElectric), "Thermal + Electric fires Plasma and consumes both", input);
			report.CheckValue("Plasma damages the target with its level formula", expected, targetBefore - Health(*target), input);
			report.CheckValue("Plasma damages every enemy within its radius", expected, insideBefore - Health(*inside), input);
			report.CheckValue("Plasma misses enemies outside its radius", 0.f, outsideBefore - Health(*outside), input);
			report.Check(!Has(*inside, kThermal) && !Has(*target, kThermal), "Plasma adds no new Ignite or status", input);
			for (const auto& actor : { target, inside, outside }) actor->Destroy();
		}

		// Fracture: Cryo + Kinetic, Armor 0 for its seconds; boss Armor loses at most half; other Armor returns afterwards.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 3000.f });
			const auto boss = f.Target(base + sf::Vector2f{ 1500.f, 3000.f });
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			for (const auto& actor : { target, boss }) actor->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
			const auto handle = target->GetAbilitySystemComponent().GetAttributes().AddModifier({ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Add, 20.f });
			for (const auto& actor : { target, boss })
			{
				Give(*actor, DamageTypeSchema::Cryo);
				Strike(f, *actor, DamageTypeSchema::Kinetic);
			}
			f.Process();
			report.Check(f.Count(StatusReactionKind::Fracture) == 2 && !Has(*target, kCryo) && !Has(*target, kKinetic), "Cryo + Kinetic fires Fracture and consumes both", input);
			report.CheckValue("Fracture sets Armor to 0", 0.f, Armor(*target), input);
			report.CheckValue("Fracture lowers boss Armor by at most 50%", 50.f, Armor(*boss), input);
			const auto* fracture = Effect(*target, "Effect.Debuff.ArmorBreak");
			report.CheckValue("Fracture lasts its level seconds", rule.fractureSeconds, fracture ? fracture->remainingDuration : 0.f, input);
			for (float elapsed = 0.f; elapsed < rule.fractureSeconds + .1f; elapsed += .05f) f.level->TickInternal(.05f);
			report.CheckValue("other Armor modifiers are back after Fracture", 120.f, Armor(*target), input);
			target->GetAbilitySystemComponent().GetAttributes().RemoveModifier(handle);
			for (const auto& actor : { target, boss }) actor->Destroy();
		}

		// Superconduct: Cryo + Electric stuns through the control response.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 4000.f });
			const auto elite = f.Target(base + sf::Vector2f{ 1500.f, 4000.f });
			const auto boss = f.Target(base + sf::Vector2f{ 3000.f, 4000.f });
			elite->SetControlTargetClass(ControlTargetClass::Elite);
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			for (const auto& actor : { target, elite, boss })
			{
				Give(*actor, DamageTypeSchema::Cryo);
				Strike(f, *actor, DamageTypeSchema::Electric);
			}
			f.Process();
			report.Check(f.Count(StatusReactionKind::Superconduct) == 3 && !Has(*target, kCryo) && !Has(*target, kElectric), "Cryo + Electric fires Superconduct and consumes both", input);
			const auto stun = [](SpaceShip& actor) { const auto* effect = Effect(actor, "Effect.Control.Stun.Basic"); return effect ? effect->remainingDuration : 0.f; };
			report.Check(target->GetAbilitySystemComponent().HasOwnedTag(GameplayTags::State::Effect::Control::Stunned), "Superconduct stuns the target", input);
			report.CheckValue("Superconduct stun lasts its level seconds", rule.stunSeconds, stun(*target), input);
			report.CheckValue("Superconduct respects elite stun resistance", rule.stunSeconds * .6f, stun(*elite), input);
			report.CheckValue("Superconduct respects boss stun immunity", 0.f, stun(*boss), input);
			for (const auto& actor : { target, elite, boss }) actor->Destroy();
		}

		// Shrapnel: Thermal + Kinetic, 6 Kinetic pieces, at most 2 per target, no new Kinetic stack.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 5000.f });
			const auto other = f.Target(base + sf::Vector2f{ 150.f, 5000.f });
			const auto outside = f.Target(base + sf::Vector2f{ rule.shrapnelRadius + 150.f, 5000.f });
			Give(*target, DamageTypeSchema::Thermal);
			Strike(f, *target, DamageTypeSchema::Kinetic);
			const float targetBefore = Health(*target), otherBefore = Health(*other), outsideBefore = Health(*outside);
			f.Process();
			const float piece = rule.shrapnelDamage + rule.shrapnelAttackPowerCoefficient * f.Stat(OwnerAttributeIds::AttackPower) + rule.shrapnelEnergyPowerCoefficient * f.Stat(OwnerAttributeIds::EnergyPower);
			report.Check(f.Count(StatusReactionKind::Shrapnel) == 1 && !Has(*target, kThermal) && !Has(*target, kKinetic), "Thermal + Kinetic fires Shrapnel and consumes both", input);
			report.CheckValue("Shrapnel hits the target with at most 2 pieces", piece * 2.f, targetBefore - Health(*target), input);
			report.CheckValue("Shrapnel hits a nearby enemy with at most 2 pieces", piece * 2.f, otherBefore - Health(*other), input);
			report.CheckValue("Shrapnel misses enemies outside its radius", 0.f, outsideBefore - Health(*outside), input);
			report.Check(!Has(*other, kKinetic) && !Has(*target, kKinetic), "Shrapnel adds no new Kinetic stack", input);
			for (const auto& actor : { target, other, outside }) actor->Destroy();
		}
	}

	void TriggerRules(Report& report, Application& application)
	{
		ReactionFixture f{ report, application, 1 };
		const sf::Vector2f base = f.ship->GetActorLocation() + sf::Vector2f{ 3000.f, 0.f };
		int total = 0;
		const auto sum = [&f]() { int count = 0; for (const auto kind : { StatusReactionKind::Steam, StatusReactionKind::Magnet, StatusReactionKind::Plasma, StatusReactionKind::Fracture, StatusReactionKind::Superconduct, StatusReactionKind::Shrapnel }) count += f.Count(kind); return count; };

		report.Check(ResolveStatusReaction(DamageStatusSchema::Ignite, DamageStatusSchema::Ignite) == StatusReactionKind::None, "a status never reacts with itself");

		// Pairing: the new type takes the eligible old status with the longest remaining time; a third status stays.
		{
			const auto target = f.Target(base);
			Give(*target, DamageTypeSchema::Thermal); Give(*target, DamageTypeSchema::Electric); Give(*target, DamageTypeSchema::Kinetic);
			Effect(*target, kThermal)->remainingDuration = 1.f;
			Effect(*target, kElectric)->remainingDuration = 3.f;
			Effect(*target, kKinetic)->remainingDuration = 2.f;
			Strike(f, *target, DamageTypeSchema::Cryo);
			f.Process();
			report.Check(f.Count(StatusReactionKind::Superconduct) == 1 && sum() == 1, "new Cryo pairs with the longest-lived Electric");
			report.Check(!Has(*target, kCryo) && !Has(*target, kElectric) && Has(*target, kThermal) && Has(*target, kKinetic), "the third and fourth statuses are preserved");
			target->Destroy();
		}
		// Tie: fixed order Thermal / Cryo / Electric / Kinetic.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 1000.f });
			Give(*target, DamageTypeSchema::Thermal); Give(*target, DamageTypeSchema::Electric);
			Effect(*target, kThermal)->remainingDuration = 2.f;
			Effect(*target, kElectric)->remainingDuration = 2.f;
			Strike(f, *target, DamageTypeSchema::Kinetic);
			f.Process();
			report.Check(f.Count(StatusReactionKind::Shrapnel) == 1 && f.Count(StatusReactionKind::Magnet) == 0 && Has(*target, kElectric), "equal remaining time pairs with Thermal before Electric");
			target->Destroy();
		}
		total = sum();
		// A refresh at the stack cap and a DoT tick never trigger.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 2000.f });
			Give(*target, DamageTypeSchema::Cryo); Give(*target, DamageTypeSchema::Thermal, 99);
			Strike(f, *target, DamageTypeSchema::Thermal);
			f.Process();
			report.Check(sum() == total && Has(*target, kThermal) && Has(*target, kCryo), "a duration refresh at the stack cap does not react");
			ApplyCombatDamage(*target, 1.f, f.ship.get(), { DamageTypeSchema::Electric }, StatusPayload(DamageTypeSchema::Electric, 1), {}, {}, DamageDeliveryType::Periodic);
			f.Process();
			report.Check(sum() == total && Has(*target, kThermal), "a periodic tick adding a stack does not react");
			// Photonic and Energy carry no stacks of their own and never pair.
			for (const auto& damageType : { DamageTypeSchema::Energy, DamageTypeSchema::Photonic }) ApplyCombatDamage(*target, 1.f, f.ship.get(), { damageType }, Plain());
			f.Process();
			report.Check(sum() == total, "Photonic and Energy hits never pair");
			target->Destroy();
		}
		// Shared 1 s lock per target; statuses behave normally meanwhile.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 3000.f });
			Give(*target, DamageTypeSchema::Thermal);
			Strike(f, *target, DamageTypeSchema::Cryo);
			f.Process();
			report.Check(sum() == total + 1, "first pair reacts");
			report.CheckValue("per-target lock is 1 s", 1.f, f.Runtime().GetTargetLockSeconds(target->GetUniqueID()), "lock right after the reaction");
			Give(*target, DamageTypeSchema::Electric);
			Strike(f, *target, DamageTypeSchema::Kinetic);
			f.Process();
			report.Check(sum() == total + 1 && Has(*target, kElectric) && Has(*target, kKinetic), "a second pair inside the lock does not react; statuses stay");
			f.Advance(.6f);
			Strike(f, *target, DamageTypeSchema::Kinetic);
			f.Process();
			report.Check(sum() == total + 1, "still locked at 0.6 s");
			f.Advance(.5f);
			Strike(f, *target, DamageTypeSchema::Kinetic);
			f.Process();
			report.Check(f.Count(StatusReactionKind::Magnet) == 1 && sum() == total + 2, "after the lock a new stack event reacts again");
			target->Destroy();
		}
		total = sum();
		// Reaction damage never starts another reaction, even on a target that carries a status.
		{
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 4000.f });
			const auto neighbour = f.Target(base + sf::Vector2f{ 100.f, 4000.f });
			Give(*neighbour, DamageTypeSchema::Electric);
			Give(*target, DamageTypeSchema::Electric);
			Strike(f, *target, DamageTypeSchema::Thermal);
			f.Process(); f.Process(); f.Advance(.1f);
			report.Check(f.Count(StatusReactionKind::Plasma) == 1 && sum() == total + 1 && Has(*neighbour, kElectric), "Plasma damage on a carrier chains nothing");
			for (const auto& actor : { target, neighbour }) actor->Destroy();
		}
		total = sum();
		// Uninstall stops reactions.
		{
			f.Remove(f.instance);
			report.Check(f.Runtime().GetRules().empty(), "uninstall clears the runtime");
			const auto target = f.Target(base + sf::Vector2f{ 0.f, 5000.f });
			Give(*target, DamageTypeSchema::Thermal);
			Strike(f, *target, DamageTypeSchema::Cryo);
			f.Process();
			report.Check(Has(*target, kThermal) && Has(*target, kCryo) && f.Count(StatusReactionKind::Steam) == 0, "no reaction after uninstall");
			target->Destroy();
		}
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
		{
			Json source; std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" }; input >> source;
			const auto loaded = content::ModuleLoader::LoadFromJson(source);
			report.Check(loaded.Succeeded(), "shipped modules load with the Reaction Core rule", loaded.error);
			std::string effectError;
			const bool effects = content::EffectContentCatalog::LoadFromFile(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/effects.json", EffectData::GetBuiltinGameplayEffectDefinitions(), &effectError);
			report.Check(effects, "shipped effects load with the shot deviation and armor break debuffs", effectError);
		}
		report.Check(GameContentBootstrap::Register(), "register shipped game content");
		std::string title = "Legendary Reaction Core E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ReactionEffects(report, application, 1);
		ReactionEffects(report, application, 3);
		TriggerRules(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryReactionCoreE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
