#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/damage/DamageHitGroup.h"
#include "gameplay/module/generated/ModuleDelayedExplosionActor.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "player/Player.h"
#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace rare_wave4_polish_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		// Shipped content only. The fixture registers the real modules.json; nothing is copied or re-parsed here.
		constexpr const char* kCore = "Module.Utility.TripleCore.Rare";
		constexpr const char* kHull = "Module.Utility.StrippedHull.Rare";
		constexpr const char* kOdometer = "Module.Movement.Odometer.Rare";
		// Shipped one-cell Common module without a damage rule; installing it changes the empty-cell count only.
		constexpr const char* kFiller = "Module.Utility.TargetingComputer.Basic";
		// Shipped Uncommon Primary module: its bounce is generated damage derived from the applied first hit.
		constexpr const char* kRicochet = "Module.Primary.RicochetTip.Uncommon";

		// Level 1 values from modules.json.
		constexpr float kCoreMultiplier = 1.25f;
		constexpr float kHullPerCell = 0.025f;
		constexpr float kOdometerPerStack = 0.01f;
		constexpr float kProbeDamage = 100.f;
		// Long enough to finish any module reboot in one tick.
		constexpr float kRebootTick = 100.f;

		bool Near(float left, float right, float tolerance = 1e-4f)
		{
			return std::abs(left - right) <= tolerance;
		}

		// Instance id of the installed Module.System.AbilitySlot(index + 1) shell, or 0 when it is not installed.
		std::uint32_t FindShell(const Fixture& f, int index)
		{
			const GameplayTag systems[4] = { GameplayTags::Module::System::AbilitySlot1, GameplayTags::Module::System::AbilitySlot2,
				GameplayTags::Module::System::AbilitySlot3, GameplayTags::Module::System::AbilitySlot4 };
			for (const ModulePlacement& placement : f.loadout.GetPlacements())
			{
				const ModuleDefinition* definition = f.rules.lookup(placement.moduleId);
				if (definition && definition->systemCategory == systems[index]) return placement.instanceId;
			}
			return 0;
		}

		// Q/E/F/R abilities bound by the default loadout, captured before any shell is parked.
		std::array<sas::ContentId, 4> CaptureAbilities(Fixture& f)
		{
			const sas::AbilitySlot slots[4] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };
			std::array<sas::ContentId, 4> ids;
			for (std::size_t index = 0; index < 4; ++index)
			{
				const GameAbility* ability = f.ship->GetAbilitySystemComponent().GetAbility(slots[index]);
				f.report.Check(ability != nullptr, "Default loadout binds a Q/E/F/R ability", Json{ { "slotIndex", index + 1 } });
				ids[index] = sas::ContentId{ ability->GetDefinition().abilityId };
			}
			return ids;
		}

		// One real hit on a target whose health is restored first. Returns the health it removed.
		float Dealt(Fixture& f, Actor& target, const sas::ContentId& ability, const List<GameplayTag>& tags)
		{
			HealthComponent* health = target.FindComponent<HealthComponent>();
			health->Regenerate(health->GetMaxHealth());
			return Hit(target, f.ship.get(), kProbeDamage, Plain(), ability, tags);
		}

		// Uninstalls through the real loadout and reapplies the layout out of combat. The instance keeps its cell for Unpark.
		struct Parked
		{
			std::uint32_t instance = 0;
			sf::Vector2i origin{};
			int rotation = 0;
		};

		Parked Park(Fixture& f, std::uint32_t instance)
		{
			const ModulePlacement placed = FindInstalled(f.loadout, instance);
			const Parked parked{ instance, placed.origin, placed.rotationQuarterTurns };
			f.Remove(instance);
			return parked;
		}

		void Unpark(Fixture& f, const Parked& parked)
		{
			f.report.Check(f.loadout.Install(f.rules, parked.instance, parked.origin, parked.rotation) == ModulePlacementFailure::None &&
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "Reinstalls the parked module on its own cell through the real applier");
		}

		// Reinstalls a parked module in combat. The applier must start a reboot for it and keep it installed.
		bool RebootInstall(Fixture& f, const Parked& parked)
		{
			return f.loadout.Install(f.rules, parked.instance, parked.origin, parked.rotation) == ModulePlacementFailure::None &&
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules, true) && f.ship->GetModuleApplier().IsRebooting(parked.instance);
		}

		// Damage multiplier the applier resolved for this instance's rule (0 when the rule is not in the output).
		float ResolvedMultiplier(const Fixture& f, std::uint32_t instance)
		{
			for (const ResolvedModuleDamageRule& damage : f.ship->GetModuleApplier().GetResolvedEffects().damageRules)
				if (damage.instanceId == instance) return damage.rule.damageMultiplier;
			return 0.f;
		}

		int ResolvedRuleCount(const Fixture& f, std::uint32_t instance)
		{
			int count = 0;
			for (const ResolvedModuleDamageRule& damage : f.ship->GetModuleApplier().GetResolvedEffects().damageRules)
				if (damage.instanceId == instance) ++count;
			return count;
		}

		void Accrue(Fixture& f, double units)
		{
			f.loadout.AccrueRunCounters(f.rules, units);
			f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
		}

		float Multiplier(const SpaceShip& ship)
		{
			return ship.GetCombatRuntime().GetOutgoingDamageMultiplier();
		}

		// Counts the generated (module-suppressed) hits the owner deals while it is alive.
		struct GeneratedObserver
		{
			SpaceShip& ship;
			DelegateHandle handle;
			int generated = 0;
			bool allSuppressed = true;
			explicit GeneratedObserver(SpaceShip& owner) : ship{ owner }, handle{ ship.GetCombatRuntime().onDamageDealt.BindAction(this, &GeneratedObserver::OnDamage) } {}
			~GeneratedObserver() { ship.GetCombatRuntime().onDamageDealt.UnbindAction(handle); }
			void OnDamage(const DamageContext& context)
			{
				if (context.payload.moduleTriggerPolicy != DamageModuleTriggerPolicy::Suppressed) return;
				++generated;
				allSuppressed = allSuppressed && context.payload.skipModuleDamageRules && !context.wasCritical;
			}
		};

		struct Shot
		{
			float first = 0.f;
			float bounce = 0.f;
		};

		// One Primary shot as a real hit with a PrimaryShot hit group. The Ricochet bounce lands on the other enemy.
		Shot PrimaryShot(Fixture& f, SpaceShip& first, SpaceShip& second)
		{
			HealthComponent& firstHealth = first.GetHealthComponent();
			HealthComponent& secondHealth = second.GetHealthComponent();
			firstHealth.Regenerate(firstHealth.GetMaxHealth());
			secondHealth.Regenerate(secondHealth.GetMaxHealth());
			auto payload = Plain();
			payload.hitGroup = std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot);
			Shot shot;
			shot.first = Hit(first, f.ship.get(), kProbeDamage, payload);
			shot.bounce = secondHealth.GetMaxHealth() - secondHealth.GetHealth();
			return shot;
		}

		void CatalogChecks(Report& report)
		{
			const auto loaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(loaded.Succeeded(), "shipped modules.json loads through the real ModuleLoader", loaded.error);
			for (const char* id : { kCore, kHull, kOdometer })
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(sas::ContentId{ std::string{ id } });
				report.Check(definition != nullptr && definition->rarity == ModuleRarity::Rare, std::string{ "shipped Rare module is in the catalog: " } + id);
			}
		}

		// Reboot must not change Triple Core: the rebooting shell still occupies its ability slot.
		void TripleCoreRebootChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::array<sas::ContentId, 4> ids = CaptureAbilities(f);
			const List<GameplayTag> none{};
			const auto enemy = Enemy(f);

			std::array<std::uint32_t, 4> shells{};
			for (int index = 0; index < 4; ++index)
			{
				shells[static_cast<std::size_t>(index)] = FindShell(f, index);
				report.Check(shells[static_cast<std::size_t>(index)] != 0, "Default loadout installs every ability-slot shell", Json{ { "shellIndex", index + 1 } });
			}

			const std::uint32_t core = f.Install(kCore, 0, 1);
			const Parked shell1 = Park(f, shells[0]);
			const float active = Dealt(f, *enemy, ids[2], none);
			report.Check(ResolvedRuleCount(f, core) == 1 && Near(ResolvedMultiplier(f, core), kCoreMultiplier),
				"one empty shell: Triple Core is active", Json{ { "rules", ResolvedRuleCount(f, core) } });
			report.CheckValue("one empty shell: a Q/E/F/R hit is multiplied by 1.25", kProbeDamage * kCoreMultiplier, active, "shipped Triple Core, level 1");

			const Parked shell2 = Park(f, shells[1]);
			report.Check(ResolvedRuleCount(f, core) == 1, "two empty shells: Triple Core remains active");

			report.Check(RebootInstall(f, shell2), "reinstalling a shell in combat starts its reboot");
			report.Check(ResolvedRuleCount(f, core) == 1 && Near(ResolvedMultiplier(f, core), kCoreMultiplier),
				"rebooting shell counts as filled: Triple Core stays active during the reboot", Json{ { "rules", ResolvedRuleCount(f, core) } });
			report.CheckValue("rebooting shell: the Q/E/F/R hit is unchanged", active, Dealt(f, *enemy, ids[2], none), "reboot in progress");

			f.ship->GetModuleApplier().Tick(kRebootTick);
			report.Check(!f.ship->GetModuleApplier().IsRebooting(shell2.instance) && ResolvedRuleCount(f, core) == 1,
				"reboot completion keeps Triple Core active");

			const Parked uninstalled = Park(f, shell2.instance);
			report.Check(ResolvedRuleCount(f, core) == 1, "uninstalling the shell leaves Triple Core active with two empty slots");

			report.Check(RebootInstall(f, uninstalled), "reinstalling the shell in combat starts a second reboot");
			const Parked rebooting = Park(f, shell2.instance);
			report.Check(ResolvedRuleCount(f, core) == 1 && !f.ship->GetModuleApplier().IsRebooting(shell2.instance),
				"uninstalling a rebooting shell leaves it empty and clears its reboot");
			Unpark(f, rebooting);
			report.Check(ResolvedRuleCount(f, core) == 1 && Near(ResolvedMultiplier(f, core), kCoreMultiplier),
				"the reinstalled shell fills its slot again: Triple Core is active");

			Unpark(f, shell1);
			report.Check(ResolvedRuleCount(f, core) == 0, "all four shells installed: Triple Core is inert again");
			f.Remove(core);
		}

		// Reboot must not change Stripped Hull: the rebooting module's cell is still occupied.
		void StrippedHullRebootChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::array<sas::ContentId, 4> ids = CaptureAbilities(f);
			const List<GameplayTag> none{};
			const auto enemy = Enemy(f);

			const std::uint32_t hull = f.Install(kHull, 0, 1);
			const std::uint32_t filler = f.Install(kFiller, 0, 1);
			const int empty = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			const float expected = 1.f + kHullPerCell * static_cast<float>(empty);
			report.CheckValue("Stripped Hull factor counts the empty cells with the filler installed", expected, ResolvedMultiplier(f, hull),
				"empty cells " + std::to_string(empty));
			const float before = Dealt(f, *enemy, ids[0], none);

			const Parked parked = Park(f, filler);
			report.Check(RebootInstall(f, parked), "reinstalling the filler in combat starts its reboot");
			report.CheckValue("rebooting filler's cell is not counted empty: Stripped Hull factor unchanged", expected, ResolvedMultiplier(f, hull),
				"reboot in progress");
			report.CheckValue("rebooting filler: a real Q/E/F/R hit is unchanged", before, Dealt(f, *enemy, ids[0], none), "reboot in progress");

			f.ship->GetModuleApplier().Tick(kRebootTick);
			report.CheckValue("reboot completion keeps the Stripped Hull factor", expected, ResolvedMultiplier(f, hull), "filler installed again");

			const Parked uninstalled = Park(f, filler);
			report.CheckValue("uninstalled filler frees its cell: the Stripped Hull factor grows by one cell", 1.f + kHullPerCell * static_cast<float>(empty + 1),
				ResolvedMultiplier(f, hull), "filler removed");
			Unpark(f, uninstalled);
			f.Remove(hull);
		}

		// Odometer scales all owner damage. A generated bounce derives from the already scaled hit, so the factor applies once.
		void OdometerGeneratedChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::uint32_t ricochet = f.Install(kRicochet, 0, 1);
			const std::uint32_t odometer = f.Install(kOdometer, 0, 1);
			const auto first = Enemy(f, 100.f);
			const auto second = Enemy(f, 200.f);
			const float base = Multiplier(*f.ship);

			const Shot stack0 = PrimaryShot(f, *first, *second);
			report.Check(stack0.bounce > 0.f, "Ricochet bounces a real Primary hit at stack 0", Json{ { "bounce", stack0.bounce } });

			Accrue(f, 5000.0);
			report.CheckValue("odometer stack 1 adds 1% outgoing damage", 1.f + kOdometerPerStack, Multiplier(*f.ship) / base, "shipped Odometer, level 1");
			GeneratedObserver observer{ *f.ship };
			const Shot stack1 = PrimaryShot(f, *first, *second);
			report.CheckValue("stack 1 Primary hit is 1.01 times the stack 0 hit", 1.f + kOdometerPerStack, stack1.first / stack0.first, "direct hit");
			report.CheckValue("stack 1 generated bounce is 1.01 times the stack 0 bounce, applied once", 1.f + kOdometerPerStack,
				stack1.bounce / stack0.bounce, "Ricochet from the scaled hit, no second multiplication");
			report.Check(observer.generated == 1 && observer.allSuppressed,
				"the generated bounce stays suppressed and triggers no other module", Json{ { "generated", observer.generated } });

			f.Remove(odometer);
			f.Remove(ricochet);
		}

		template<typename T> List<shared_ptr<T>> LiveActors(Fixture& f)
		{
			List<shared_ptr<T>> actors;
			for (const auto& weak : f.level->GetActorsByTypeIncludingPending<T>()) if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) actors.push_back(actor);
			return actors;
		}

		// Same inline status helper as UncommonModuleP5E2E.h: a one-stack Thermal or Cryo hit.
		void StatusHit(SpaceShip& target, Actor* source, const GameplayTag& type)
		{
			auto payload = Plain();
			if (type == DamageTypeSchema::Cryo) payload.cryoBuildupPerHit = 1;
			if (type == DamageTypeSchema::Thermal) payload.igniteStacks = 1;
			Hit(target, source, 0.1f, payload, {}, {}, DamageDeliveryType::Direct, { type });
		}

		// Thermal and Cryo on the first target make its death create the shipped ThermalShockCoil burst. Returns the damage
		// the burst deals to a target inside its radius. sourceMultiplier is an extra runtime source (1 = none).
		float DeathBurstDamage(Report& report, Application& application, bool withOdometer, float sourceMultiplier)
		{
			Fixture f{ report, application };
			f.Install("Module.Offense.ThermalShockCoil.Uncommon", 1, 1, "Module.SideFeature.ThermalShockCoil.DeathBurst");
			if (withOdometer)
			{
				f.Install(kOdometer, 0, 1);
				Accrue(f, 5000.0);
			}
			const auto first = Enemy(f, 200.f);
			const auto inside = Enemy(f, 299.f);
			StatusHit(*first, f.ship.get(), DamageTypeSchema::Thermal);
			StatusHit(*first, f.ship.get(), DamageTypeSchema::Cryo);
			if (sourceMultiplier != 1.f) f.ship->GetCombatRuntime().SetRuntimeModifier("E2E.sourceScale", { sourceMultiplier });
			Hit(*first, f.ship.get(), 10000.f, Plain());
			const auto bursts = LiveActors<ModuleDelayedExplosionActor>(f);
			report.Check(bursts.size() == 1, "dead ThermalShockCoil target creates one real death burst", Json{ { "bursts", bursts.size() } });
			if (bursts.empty()) return 0.f;
			bursts.front()->Tick(0.f);
			return 1000.f - inside->GetHealthComponent().GetHealth();
		}

		// The death burst is a kill explosion: the Odometer stack applies to it once, and a second runtime source does not.
		void OdometerDeathBurstChecks(Report& report, Application& application)
		{
			const float plain = DeathBurstDamage(report, application, false, 1.f);
			const float stack1 = DeathBurstDamage(report, application, true, 1.f);
			const float stack1Doubled = DeathBurstDamage(report, application, true, 2.f);
			report.CheckValue("death burst is 3% of the target's max health without run counters", 30.f, plain, "ThermalShockCoil level 1");
			report.CheckValue("odometer stack 1 raises the death burst by 1%", 1.f + kOdometerPerStack, stack1 / plain, "run counter applied to the kill explosion");
			report.CheckValue("a second runtime source multiplier does not reach the death burst", stack1, stack1Doubled, "no second source scale");
		}

		void RegressionChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const ModuleDefinition* common = f.rules.lookup(kFiller);
			report.Check(common != nullptr, "regression: the shipped Common module is available");
			const float before = f.Owner(common->sizeVariants[0].attributeModifiers[0].modifier.attributeId);
			const std::uint32_t instance = f.Install(kFiller, 0, 1);
			const ModulePlacement placement = *f.loadout.FindPlacement(instance);
			const float zoneBonus = ModuleResolver::GetZoneBonus(f.rules, *common, placement);
			const ModuleAttributeModifier& entry = common->sizeVariants[0].attributeModifiers[0];
			const float base = entry.magnitudeByLevel.ValueAtLevel(entry.modifier.magnitude, 1);
			report.CheckValue("regression: the shipped Common module still scales by its own zone bonus only", base * (1.f + zoneBonus),
				f.Owner(entry.modifier.attributeId) - before, "Targeting Computer with the Rare set installed");
			f.Remove(instance);
		}
	}
}

void ReportedLiveLoadoutChecks(uncommon_p4_e2e::Report& report, vega::Application& application)
{
	using namespace uncommon_p4_e2e;
	Fixture f{ report, application };
	auto player = std::make_shared<Player>();
	auto ship = player->SpawnSpaceShip(f.level.get()).lock();
	report.Check(ship != nullptr, "reported bugs: spawn through the real Player loadout path");
	f.level->TickInternal(0.f);
	ship->SetInvulnerability(false);
	ship->SetActorLocation({ 20000.f, 20000.f });
	const auto rules = player->BuildModuleRules();
	const auto install = [&](const char* moduleId, const char* feature = "", int variant = 0)
	{
		const auto instance = player->AddModuleToInventory(sas::ContentId{ moduleId }, variant, rules->lookup(moduleId)->rarity, 1, sas::ContentId{ feature });
		report.Check(instance != 0, "real Player inventory accepts the reported module");
		for (int rotation = 0; rotation < 4; ++rotation)
			for (int y = 0; y < rules->schematic.height; ++y)
				for (int x = 0; x < rules->schematic.width; ++x)
					if (player->TryInstallModule(instance, { x, y }, rotation) == ModulePlacementFailure::None)
					{
						ship->GetModuleApplier().Tick(100.f);
						return instance;
					}
		throw std::runtime_error{ "No legal placement for reported live-loadout check" };
	};
	const auto target = Enemy(f);
	target->GetShieldComponent().SetMaxShield(0.f);
	target->SetCollisionMask(vega::GameCollision::PlayerBullet);
	report.Check(ship->GetAbilityLoadout().EquipAbility(AbilityData::InfernoSpray::AbilityId::Basic, sas::AbilitySlot::Ability2), "live loadout equips Inferno Spray for reported ability-hit bugs");
	ship->GetAbilitySystemComponent().AddOwnedTag(GameplayTags::State::Ability::InfernoSpray::Active);
	const auto sprayTick = [&](SpaceShip& victim, int thermalStacks = 0)
	{
		f.level->TickInternal(0.f);
		ship->SetActorRotation(0.f);
		victim.SetActorLocation(ship->GetActorLocation() + ship->GetActorForwardDirection() * 200.f);
		const float before = victim.GetHealthComponent().GetHealth();
		auto actor = f.level->SpawnActor<InfernoSprayActor>(ship.get(), InfernoSprayPresentationProfile{}).lock();
		sas::GameplayAttributeList values{ sas::GameplayAttribute{ AbilityData::InfernoSpray::Actor::FlameCone::Range, 400.f, 0.f }, sas::GameplayAttribute{ AbilityData::InfernoSpray::Actor::FlameCone::ConeAngle, 36.f, 0.f }, sas::GameplayAttribute{ AbilityData::InfernoSpray::Actor::FlameCone::BaseDPS, 40.f, 0.f } };
		actor->ConfigureFromAttributes(values);
		actor->SetSourceAbility(sas::ContentId{ AbilityData::InfernoSpray::AbilityId::Basic }, { GameplayTags::Ability::Offense });
		DamagePayload payload = Plain(); payload.igniteStacks = thermalStacks;
		actor->SetDamagePayload(payload);
		actor->Tick(0.25f);
		actor->Destroy();
		return before - victim.GetHealthComponent().GetHealth();
	};
	const float sprayBaseline = sprayTick(*target);
	report.Check(sprayBaseline > 0.f, "real Inferno Spray actor reaches the target", Json{ { "damage", sprayBaseline }, { "shipX", ship->GetActorLocation().x }, { "shipY", ship->GetActorLocation().y }, { "targetX", target->GetActorLocation().x }, { "targetY", target->GetActorLocation().y }, { "active", ship->GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::InfernoSpray::Active) } });
	const auto shot = [&](DamagePayload payload = Plain(), List<GameplayTag> tags = {})
	{
		target->GetHealthComponent().Regenerate(1000.f);
		const float before = target->GetHealthComponent().GetHealth();
		sas::GameplayAttributeList values{ sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f } };
		auto projectile = f.level->SpawnActor<PrimaryWeaponProjectileActor>(ship.get(), WeaponPresentationDefinition{}, values).lock();
		projectile->ConfigureCollisionFromOwner();
		projectile->SetDamageTags(tags);
		projectile->SetDamagePayload(payload);
		projectile->OnActorBeginOverlap(target.get());
		return before - target->GetHealthComponent().GetHealth();
	};
	const float baseline = shot();
	report.Check(baseline > 0.f, "a real Primary projectile hits the test target");
	const auto hull = install("Module.Utility.StrippedHull.Rare");
	const int empty = ModuleResolver::CountEmptyCells(*rules, player->GetModuleLoadout().GetPlacements());
	report.CheckValue("Stripped Hull affects a real Primary projectile", baseline * (1.f + 0.025f * empty), shot(), "real Player install, projectile impact");
	report.CheckValue("Stripped Hull affects a real Inferno Spray tick", sprayBaseline * (1.f + 0.025f * empty), sprayTick(*target), "shared outgoing damage boundary");
	report.Check(player->TryUninstallModule(hull), "real Player removes Stripped Hull");
	const auto core = install("Module.Utility.TripleCore.Rare");
	std::uint32_t shell = 0;
	for (const auto& placement : player->GetModuleLoadout().GetPlacements())
		if (rules->lookup(placement.moduleId)->systemCategory == GameplayTags::Module::System::AbilitySlot1) shell = placement.instanceId;
	report.Check(shell != 0 && player->TryUninstallModule(shell), "real Player removes exactly one ability shell");
	const auto* ability = ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability3);
	report.Check(ability != nullptr, "remaining Q/E/F/R ability retains its source identity");
	const sas::ContentId abilityId{ ability->GetDefinition().abilityId };
	const float abilityHit = Hit(*target, ship.get(), 10.f, Plain(), abilityId, ability->GetDefinition().abilityTags, DamageDeliveryType::Projectile);
	report.CheckValue("Triple Core affects a remaining ability projectile", baseline * 1.25f, abilityHit, "one shell removed through Player API");
	report.CheckValue("Triple Core affects a real Inferno Spray tick", sprayBaseline * 1.25f, sprayTick(*target), "Inferno source ability ID is preserved");
	report.Check(player->TryUninstallModule(core), "real Player removes Triple Core");
	const auto opening = install("Module.Offense.EmberAccumulation.Rare", "Module.SideFeature.Shared.O5.Rare", 1);
	const auto fresh = Enemy(f);
	fresh->GetShieldComponent().SetMaxShield(0.f);
	const float first = Hit(*fresh, ship.get(), 10.f, Plain(), {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Projectile);
	const float second = Hit(*fresh, ship.get(), 10.f, Plain(), {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Projectile);
	report.CheckValue("Opening Hit multiplies only the first projectile hit", second * 1.25f, first, "new target, shipped Rare O5 is 25 percent");
	const auto sprayFresh = Enemy(f);
	sprayFresh->GetShieldComponent().SetMaxShield(0.f);
	sprayFresh->SetCollisionMask(vega::GameCollision::PlayerBullet);
	const float firstSpray = sprayTick(*sprayFresh);
	const float secondSpray = sprayTick(*sprayFresh);
	report.CheckValue("Opening Hit multiplies only the first Inferno Spray tick", secondSpray * 1.25f, firstSpray, "shared first-hit claim and ability source");
	report.Check(player->TryUninstallModule(opening), "real Player removes Opening Hit host");
	const auto kinetic = install("Module.Offense.KineticBreaker.Rare", "Module.SideFeature.KineticBreaker.ArmorRend", 1);
	target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 40.f);
	DamagePayload stacks = Plain(); stacks.kineticStacks = 4;
	shot(stacks, { DamageTypeSchema::Kinetic });
	const auto* kineticEffect = target->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::KineticEffectId);
	report.Check(kineticEffect && kineticEffect->stackCount == 4, "kinetic projectile applies four stacks", Json{ { "stacks", kineticEffect ? kineticEffect->stackCount : 0 } });
	shot();
	report.Check(target->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::KineticEffectId) == nullptr, "next real projectile consumes Kinetic Breaker stacks");
	report.Check(target->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::KineticArmorRendEffectId) != nullptr, "Armor Rend applies after a real projectile consumes stacks");
	report.Check(player->TryUninstallModule(kinetic), "real Player removes Kinetic Breaker");
	const auto areaHost = install("Module.Offense.EmberAccumulation.Rare", "Module.SideFeature.EmberAccumulation.AreaBlast", 1);
	const auto areaTarget = Enemy(f);
	areaTarget->GetShieldComponent().SetMaxShield(0.f);
	areaTarget->SetCollisionMask(vega::GameCollision::PlayerBullet);
	const auto neighbour = Enemy(f);
	neighbour->GetShieldComponent().SetMaxShield(0.f);
	neighbour->SetActorLocation(ship->GetActorLocation() + ship->GetActorForwardDirection() * 200.f + ship->GetActorRightDirection() * 120.f);
	f.level->TickInternal(0.f);
	const float neighbourBefore = neighbour->GetHealthComponent().GetHealth();
	for (int tick = 0; tick < 4; ++tick) sprayTick(*areaTarget, 1);
	report.Check(neighbour->GetHealthComponent().GetHealth() < neighbourBefore, "Ember Area Blast reaches a neighbour after real Inferno Spray stacks");
	report.Check(player->TryUninstallModule(areaHost), "real Player removes Ember Area Blast");
	const auto smolderHost = install("Module.Offense.EmberAccumulation.Rare", "Module.SideFeature.EmberAccumulation.SmolderingRemains", 1);
	const auto smolderTarget = Enemy(f);
	smolderTarget->GetShieldComponent().SetMaxShield(0.f);
	smolderTarget->SetCollisionMask(vega::GameCollision::PlayerBullet);
	for (int tick = 0; tick < 4; ++tick) sprayTick(*smolderTarget, 1);
	report.Check(ship->GetModuleApplier().GetSmolderFieldCount() > 0, "Ember Smoldering Remains spawns from real Inferno Spray stacks", Json{ { "fields", ship->GetModuleApplier().GetSmolderFieldCount() } });
	report.Check(player->TryUninstallModule(smolderHost), "real Player removes Ember Smoldering Remains");
	ship->SetMovementMode(ShipMovementMode::ThrustDrift);
	auto& movement = ship->GetMovementComponent();
	const float normalCap = movement.ResolveCurrentSpeedCap(1.f);
	const float normalThrust = movement.ResolveForwardThrust();
	const auto omni = install("Module.Movement.OmniThrusters.Uncommon");
	report.CheckValue("Omni lowers the live top-speed cap by 30 percent", normalCap * 0.70f, movement.ResolveCurrentSpeedCap(1.f), "real Player install");
	report.CheckValue("Omni lowers forward thrust by 30 percent", normalThrust * 0.70f, movement.ResolveForwardThrust(), "real Player install");
	report.CheckValue("Omni reverse thrust matches the reduced forward thrust", movement.ResolveForwardThrust(), movement.ResolveReverseThrust(), "direction equalization");
	report.CheckValue("Omni sideways thrust matches the reduced forward thrust", movement.ResolveForwardThrust(), movement.ResolveStrafeThrust(), "direction equalization");
	for (const auto direction : { sf::Vector2f{ 1.f, 0.f }, sf::Vector2f{ -1.f, 0.f }, sf::Vector2f{ 0.f, 1.f } })
	{
		ship->SetVelocity(direction * 5000.f);
		f.level->TickInternal(0.1f);
		report.Check(GetVectorLength(ship->GetVelocity()) <= normalCap * 0.70f + 0.01f, "Omni clamps actual velocity in every direction", Json{ { "speed", GetVectorLength(ship->GetVelocity()) } });
	}
	report.Check(player->TryUninstallModule(omni), "real Player removes Omni Thrusters");
	report.CheckValue("removing Omni restores top speed", normalCap, movement.ResolveCurrentSpeedCap(1.f), "removal cleanup");
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace rare_wave4_polish_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare wave 4 content registration failed" };
		std::string title = "Rare Wave 4 Polish E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		ReportedLiveLoadoutChecks(report, application);
		CatalogChecks(report);
		TripleCoreRebootChecks(report, application);
		StrippedHullRebootChecks(report, application);
		OdometerGeneratedChecks(report, application);
		OdometerDeathBurstChecks(report, application);
		RegressionChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareWave4PolishE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
