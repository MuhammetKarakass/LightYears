#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/module/ModuleRoller.h"
#include "gameplay/progression/RunGold.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"

#include <set>

namespace
{
	using namespace uncommon_p6_e2e;
	const char* kCore = "Module.Utility.TripleCore.Rare";
	const char* kDrive = "Module.Movement.MomentumDrive.Epic";
	const char* kField = "Module.Utility.FieldMastery.Epic";
	const char* kEmergency = "Module.Defense.EmergencyProtocol.Epic";
	std::string Shared(const std::string& suffix) { return "Module.SideFeature.Shared." + suffix; }
	const ModuleSideFeatureDefinition& Feature(const std::string& suffix)
	{
		const auto* feature = content::ModuleSideFeatureCatalog::FindById(Shared(suffix));
		if (!feature) throw std::runtime_error{ "missing shared feature " + suffix };
		return *feature;
	}
	// Real ship/applier, with an empty two-size host to isolate the shipped side's contribution.
	std::shared_ptr<ModuleDefinition> Host(Fixture& f, ModuleRarity rarity)
	{
		auto host = std::make_shared<ModuleDefinition>(*content::ModuleContentCatalog::FindById("Module.Defense.OvershieldLayer.Uncommon"));
		host->moduleId = sas::ContentId{ std::string{ "Module.Defense.SidePoolProbe." } + (rarity == ModuleRarity::Uncommon ? "Uncommon" : rarity == ModuleRarity::Rare ? "Rare" : "Epic") }; host->rarity = rarity; host->sharedPools.clear(); host->sideFeatures.clear();
		for (auto& variant : host->sizeVariants) { const auto shape = variant.shape; variant = ModuleSizeVariant{ shape }; }
		for (const auto& pool : content::ModuleSideFeatureCatalog::GetPools())
		{
			ModuleSharedPoolPermission permission; permission.poolId = pool.poolId;
			for (const auto& choice : pool.choices)
			{
				const auto* feature = content::ModuleSideFeatureCatalog::FindById(choice.sideFeatureId);
				if (feature && feature->requiredRarity && *feature->requiredRarity == rarity) permission.allowedFeatureIds.push_back(choice.sideFeatureId);
			}
			if (!permission.allowedFeatureIds.empty()) host->sharedPools.push_back(std::move(permission));
		}
		f.rules.lookup = [host](const sas::ContentId& id) { return id == host->moduleId ? host.get() : content::ModuleContentCatalog::FindById(id); };
		return host;
	}
	GameAbility& Use(Fixture& f, const char* id, sas::AbilitySlot slot)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		auto* ability = asc.GetAbilityById(id);
		if (!ability)
		{
			if (auto* old = asc.GetAbility(slot)) f.report.Check(asc.RemoveAbility(old->GetHandle()), "replace fixture ability through ASC");
			auto definition = *content::AbilityContentCatalog::FindById(id); definition.slot = slot;
			f.report.Check(asc.GrantAbility(definition).IsValid(), "grant shipped side-feature test ability"); ability = asc.GetAbilityById(id);
		}
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		ability->Tick(100.f);
		f.report.Check(ability->TryActivate(), "activate real ability for side feature");
		if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
		return *ability;
	}
	void NoArmor(Fixture& f)
	{
		f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
		f.ship->GetShieldComponent().ChangeShield(-10000.f);
	}
	void PoolChecks(Report& report)
	{
		const struct { const char* module; std::set<std::string> shared; } cases[] = {
			{ kCore, { "O1.Rare", "G2.Rare", "CalmBreath" } },
			{ "Module.Defense.GoldPlating.Rare", { "D1.Rare", "G1.Rare", "AttunedSocket" } },
			{ "Module.Defense.ShieldBattery.Rare", { "D4.Rare", "D5.Rare", "AttunedSocket" } },
			{ "Module.Offense.PrismCore.Epic", { "O4.Epic", "R5.Epic", "HullBond" } },
			{ "Module.Offense.BloodContract.Epic", { "D3.Epic", "D1.Epic", "DeferredDamage" } },
			{ kEmergency, { "G2.Epic", "D6.Epic", "DeferredDamage" } },
			{ "Module.Primary.TurretMount.Epic", { "O2.Epic", "O3.Epic", "HullBond" } },
			{ kField, { "G2.Epic", "G5.Epic", "HullBond" } },
			{ kDrive, { "G4.Epic", "R6.Epic", "DeferredDamage" } }
		};
		for (const auto& item : cases)
		{
			const auto* module = content::ModuleContentCatalog::FindById(item.module); report.Check(module != nullptr, "selected side-pool carrier exists", item.module);
			std::set<std::string> actual;
			for (const auto& pool : module->sharedPools)
				for (const auto& featureId : pool.allowedFeatureIds)
				{
					const auto* feature = content::ModuleSideFeatureCatalog::FindById(featureId);
					report.Check(IsModuleSideFeatureAllowed(*module, feature), "every selected pool permission resolves and matches rarity", featureId.ToString());
					actual.insert(featureId.ToString().substr(26));
				}
			report.Check(actual == item.shared, "carrier has exactly the selected three shared sides", item.module);
			report.Check(module->sideFeatures.size() == (std::string{ item.module }.find("GoldPlating") != std::string::npos ? 1u : 2u), "private sides are wired; Gold discount still awaits shop", item.module);
			for (const auto* suffix : { "CalmBreath", "AttunedSocket", "DeferredDamage", "HullBond" })
			{
				const auto& feature = Feature(suffix);
				report.Check(IsModuleSideFeatureAllowed(*module, &feature) == (item.shared.count(suffix) != 0), "rarity-only common is opt-in, never automatic", item.module);
			}
		}
		report.Check(!content::ModuleSideFeatureCatalog::FindById(Shared("R3")) && !content::ModuleSideFeatureCatalog::FindById(Shared("R4")), "retired R3/R4 content no longer exists");
		const auto* giant = content::ModuleContentCatalog::FindById("Module.Offense.GiantSlayer.Rare");
		report.Check(IsModuleSideFeatureAllowed(*giant, &Feature("O1.Rare")), "Giant Slayer replaces retired R3 with Rare O1");
		auto forged = *content::ModuleContentCatalog::FindById(kCore); forged.rarity = ModuleRarity::Epic;
		report.Check(!IsModuleSideFeatureAllowed(forged, &Feature("CalmBreath")), "a forged permission cannot put a Rare-only side on an Epic carrier");
	}
	void ScaleChecks(Report& report, Application& application)
	{
		const ModuleRarity rarities[] = { ModuleRarity::Uncommon, ModuleRarity::Rare, ModuleRarity::Epic };
		const char* suffixes[] = { ".Uncommon", "", ".Epic" };
		const float guarded[] = { 1.09f, 1.15f, 1.21f }, plating[] = { 17.f, 30.f, 43.f }, weaver[] = { 1.11f, 1.20f, 1.29f }, fleet[] = { .91f, .85f, .79f };
		for (int rarity = 0; rarity < 3; ++rarity)
			for (int level : { 1, 3 })
			{
				Fixture f{ report, application }; auto host = Host(f, rarities[rarity]); const auto enemy = Enemy(f); NoArmor(f);
				const auto install = [&](const char* code) { return f.Install(host->moduleId.ToString().c_str(), 1, level, Shared(std::string{ code } + suffixes[rarity]).c_str()); };
				const auto guard = install("R1"); Use(f, "Ability.Defense.DirectionalBarrier.Basic", sas::AbilitySlot::Ability1);
				f.Equal("Guarded Strike scales by rarity and stays fixed across levels", 10.f * guarded[rarity], Hit(*enemy, f.ship.get(), 10.f, Plain(), {}, { GameplayTags::Ability::Primary })); f.Remove(guard);
				const float armor = f.Owner(OwnerAttributeIds::Armor); const auto plate = install("R2");
				auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(shield.GetMaxShield()); Hit(*f.ship, enemy.get(), shield.GetShield(), Plain());
				f.Equal("Reactive Plating scales by rarity", armor + plating[rarity], f.Owner(OwnerAttributeIds::Armor)); f.Remove(plate);
				StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Thermal); StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric);
				const float base = Hit(*enemy, f.ship.get(), 10.f, Plain()); const auto weave = install("R5");
				f.Equal("Status Weaver scales by rarity", base * weaver[rarity], Hit(*enemy, f.ship.get(), 10.f, Plain())); f.Remove(weave);
				const auto run = install("R6"); CastMovement(f); f.ship->GetShieldComponent().ChangeShield(-10000.f);
				f.Equal("Fleet Guard scales by rarity", 10.f * fleet[rarity], Hit(*f.ship, enemy.get(), 10.f, Plain())); f.Remove(run);
			}
		const float scales[] = { 1.f, 1.75f, 2.5f };
		for (int rarity = 0; rarity < 3; ++rarity)
		{
			const std::string suffix = rarity == 0 ? "" : rarity == 1 ? ".Rare" : ".Epic";
			Fixture f{ report, application }; auto host = Host(f, rarities[rarity]);
			const struct { const char* code; const sas::AttributeId* attribute; float base; bool discrete; } stats[] = {
				{ "O1", &OwnerAttributeIds::AttackPower, 8.f, false }, { "O2", &OwnerAttributeIds::CriticalChance, 8.f, false }, { "O3", &OwnerAttributeIds::AttackSpeed, 4.f, false },
				{ "D1", &OwnerAttributeIds::MaxHealth, 25.f, false }, { "G2", &OwnerAttributeIds::AbilityHaste, 4.f, false }, { "G4", &OwnerAttributeIds::MoveSpeedHorizontal, .8f, false }, { "G5", &OwnerAttributeIds::Luck, 1.f, true }
			};
			for (const auto& stat : stats)
			{
				const float base = f.Owner(*stat.attribute); const auto instance = f.Install(host->moduleId.ToString().c_str(), 1, 3, Shared(stat.code + suffix).c_str());
				const float bonus = stat.discrete ? std::round(stat.base * scales[rarity]) : stat.base * scales[rarity];
				f.Equal("O/D/G stat obeys exact rarity scale and discrete rounding", base + bonus, f.Owner(*stat.attribute)); f.Remove(instance);
			}
			const auto& d6 = Feature("D6" + suffix); f.Equal("Desperate Patch amount uses rarity scale", .12f * scales[rarity], d6.rules.eventActions.front().amount);
		}
	}
	void ZoneChecks(Report& report, Application& application)
	{
		for (const auto* moduleId : { "Module.Defense.GoldPlating.Rare", "Module.Primary.TurretMount.Epic" })
			for (int level : { 1, 3 })
				for (const auto& zone : { GameplayTags::Module::Zone::Wings, GameplayTags::Module::Zone::Hull })
				{
					Fixture f{ report, application }; const auto* module = f.rules.lookup(moduleId); const bool rare = module->rarity == ModuleRarity::Rare;
					const auto side = Shared(rare ? "AttunedSocket" : "HullBond"); const auto instance = f.loadout.AddToInventory(f.rules, *module, 1, module->rarity, level, side);
					report.Check(instance != 0, "placement side validates through inventory");
					// Expand this real ship's schematic into a single zone to isolate the placement contract.
					auto schematic = f.rules.schematic; for (auto& cell : schematic.cellZones) if (cell.IsValid()) cell = zone;
					ModuleRulesContext zoneRules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
					ModulePlacement candidate = *f.loadout.FindInventoryItem(instance); bool installed = false;
					for (int y = 0; y < f.rules.schematic.height && !installed; ++y)
						for (int x = 0; x < f.rules.schematic.width && !installed; ++x)
							if (f.loadout.Install(zoneRules, instance, { x, y }, 0) == ModulePlacementFailure::None) installed = true;
					report.Check(installed && f.ship->GetModuleApplier().Apply(f.loadout, zoneRules), "install placement-side carrier through runtime");
					const auto* placed = f.loadout.FindPlacement(instance); const int expected = zone == GameplayTags::Module::Zone::Hull || !rare ? 2 : 1;
					f.Equal("layout/presenter zone calculation honors placement side", static_cast<float>(expected), static_cast<float>(ModuleResolver::GetZoneLevelBonus(zoneRules, *module, *placed)));
					f.Equal("resolved runtime bonus matches the layout/presenter value", static_cast<float>(expected), static_cast<float>(ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), instance)));
					auto small = *placed; small.variantIndex = 0;
					f.Equal("smaller carrier keeps its placement side locked", zone == GameplayTags::Module::Zone::Hull ? 2.f : 0.f, static_cast<float>(ModuleResolver::GetZoneLevelBonus(zoneRules, *module, small)));
				}
	}
	void OtherScaleChecks(Report& report, Application& application)
	{
		for (int rarity = 0; rarity < 3; ++rarity)
		{
			Fixture f{ report, application }; const ModuleRarity category = rarity == 0 ? ModuleRarity::Uncommon : rarity == 1 ? ModuleRarity::Rare : ModuleRarity::Epic;
			const auto host = Host(f, category); const float scale = rarity == 0 ? 1.f : rarity == 1 ? 1.75f : 2.5f; const std::string suffix = rarity == 0 ? "" : rarity == 1 ? ".Rare" : ".Epic";
			const auto install = [&](const char* code) { return f.Install(host->moduleId.ToString().c_str(), 1, 3, Shared(code + suffix).c_str()); };
			const auto enemy = Enemy(f); const float firstBase = Hit(*enemy, f.ship.get(), 10.f, Plain()); const auto opening = install("O5");
			const auto fresh = Enemy(f); f.Equal("Opening Hit scales its bonus above one by rarity", firstBase * (1.f + .15f * scale), Hit(*fresh, f.ship.get(), 10.f, Plain())); f.Remove(opening);
			StatusHit(*enemy, f.ship.get(), DamageTypeSchema::Electric); const float duration = Effect(*enemy, DamageStatusEffectIds::ElectricEffectId)->spec.duration;
			const auto lasting = install("O4"); const auto statusTarget = Enemy(f); StatusHit(*statusTarget, f.ship.get(), DamageTypeSchema::Electric);
			f.Equal("Lasting Status uses exact rarity seconds", duration + scale, Effect(*statusTarget, DamageStatusEffectIds::ElectricEffectId)->spec.duration); f.Remove(lasting);
			const float regen = f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen); const auto recharge = install("D4");
			f.Equal("Shield Recharge scales its bonus above one", regen * (1.f + .15f * scale), f.ship->GetShipRuntime().GetAttributes().GetCurrentValue(ShipAttributeIds::ShieldRegen)); f.Remove(recharge);
			const float shieldMax = f.ship->GetShieldComponent().GetMaxShield(); const auto shield = install("D5"); f.Equal("Max Shield scales its percent bonus", shieldMax * (1.f + .08f * scale), f.ship->GetShieldComponent().GetMaxShield()); f.Remove(shield);
			const float capacity = f.ship->GetShipRuntime().GetAfterburnerCapacity(); const auto fuel = install("G3"); f.Equal("Afterburner capacity uses exact rarity scale", capacity * (1.f + .1f * scale), f.ship->GetShipRuntime().GetAfterburnerCapacity()); f.Remove(fuel);
			NoArmor(f); const float armor = f.Owner(OwnerAttributeIds::Armor); const auto guard = install("D2"); Hit(*f.ship, enemy.get(), 1.f, Plain());
			f.Equal("Damage Guard Armor uses exact rarity scale", armor + 12.f * scale, f.Owner(OwnerAttributeIds::Armor)); f.Remove(guard);
			const auto recovery = install("D3"); auto& health = f.ship->GetHealthComponent(); health.ChangeHealth(-50.f); const float before = health.GetHealth(); f.Kill();
			f.Equal("Hunt Recovery heals its rarity fraction", before + .01f * scale * health.GetMaxHealth(), health.GetHealth()); f.Remove(recovery);
			const auto wallet = std::make_shared<RunGold>(); f.ship->GetModuleApplier().SetGoldWallet(wallet); const auto gold = install("G1"); f.Kill();
			f.Equal("Hunt Gold uses nearest integer with halves upward", std::round(scale), static_cast<float>(wallet->GetAmount())); f.Remove(gold);
		}
	}
	void RecoveryChecks(Report& report, Application& application)
	{
		{
			Fixture f{ report, application }; f.Install(kCore, 1, 3, Shared("CalmBreath").c_str()); auto& ability = CastOffense(f); const float before = ability.GetCooldownRemaining();
			f.ship->Tick(2.9f); f.Equal("Calm Breath waits three damage-free seconds", before - 2.9f, ability.GetCooldownRemaining());
			f.ship->Tick(.3f); f.Equal("Calm Breath only recovers the part after threshold", before - 3.23f, ability.GetCooldownRemaining());
			const auto enemy = Enemy(f); Hit(*f.ship, enemy.get(), 1.f, Plain()); const float reset = ability.GetCooldownRemaining();
			f.ship->Tick(2.9f); f.Equal("shield damage immediately resets Calm Breath", reset - 2.9f, ability.GetCooldownRemaining());
		}
		{
			Fixture f{ report, application }; f.Install(kCore, 1, 1, "Module.SideFeature.TripleCore.SpareTimer");
			const auto placements = f.loadout.GetPlacements(); int empty = 0;
			for (const auto& placement : placements) if (AbilitySlotForModuleSystem(f.rules.lookup(placement.moduleId)->systemCategory) != sas::AbilitySlot::None && empty < 2) { f.Remove(placement.instanceId); ++empty; }
			auto& ability = CastOffense(f); const float before = ability.GetCooldownRemaining(); f.ship->Tick(.5f);
			f.Equal("two empty ability shells grant thirty percent extra recovery", before - .65f, ability.GetCooldownRemaining());
		}
		{
			Fixture f{ report, application }; f.Install(kCore, 1, 3, "Module.SideFeature.TripleCore.FocusFire"); const auto enemy = Enemy(f);
			f.Equal("Focus Fire is gated while all shells are installed", 100.f, Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }));
			const auto placements = f.loadout.GetPlacements(); for (const auto& p : placements) if (AbilitySlotForModuleSystem(f.rules.lookup(p.moduleId)->systemCategory) == sas::AbilitySlot::Ability1) f.Remove(p.instanceId);
			float abilityMultiplier = 0.f; for (const auto& rule : f.ship->GetModuleApplier().GetResolvedEffects().damageRules) if (rule.rule.condition == ModuleDamageCondition::AbilityDirectHit) abilityMultiplier = rule.rule.damageMultiplier;
			f.Equal("Focus Fire shares half the effective Triple Core bonus", 100.f * (1.f + .5f * (abilityMultiplier - 1.f)), Hit(*enemy, f.ship.get(), 100.f, Plain(), {}, { GameplayTags::Ability::Primary }));
		}
	}
	void DeferredChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; const auto instance = f.Install(kDrive, 1, 1, Shared("DeferredDamage").c_str()); NoArmor(f); const auto enemy = Enemy(f);
		auto& health = f.ship->GetHealthComponent(); const float initial = health.GetHealth(); Hit(*f.ship, enemy.get(), 100.f, Plain()); f.Equal("Deferred Damage takes seventy percent immediately", initial - 70.f, health.GetHealth());
		f.ship->GetDeferredDamageRuntime().Tick(1.f); f.Equal("first hit pays ten percent per second", initial - 80.f, health.GetHealth());
		Hit(*f.ship, enemy.get(), 50.f, Plain()); f.Equal("overlapping hit has its own immediate share", initial - 115.f, health.GetHealth());
		f.Remove(instance); enemy->Destroy(); f.ship->GetDeferredDamageRuntime().Tick(2.f);
		f.Equal("removal and source destruction preserve earned overlapping debt", initial - 145.f, health.GetHealth());
		f.ship->GetDeferredDamageRuntime().Tick(2.f); f.Equal("large tick pays only remaining debt without recursion", initial - 150.f, health.GetHealth());
		f.Equal("debt is empty after all original damage is paid", 0.f, f.ship->GetDeferredDamageRuntime().GetPendingDamage());
		const auto reinstalled = f.Install(kDrive, 1, 1, Shared("DeferredDamage").c_str());
		auto& shield = f.ship->GetShieldComponent(); shield.ChangeShield(shield.GetMaxShield()); const float shieldBefore = shield.GetShield();
		Hit(*f.ship, nullptr, 20.f, Plain()); f.Equal("shield receives only the immediate seventy percent", shieldBefore - 14.f, shield.GetShield());
		f.ship->GetShipRuntime().GetAttributes().SetBaseValue(ShipAttributeIds::ShieldRegen, 0.f); f.ship->SetInvulnerability(true); f.ship->Tick(3.f); f.Equal("the same deferred share later reaches shield through the real ship tick", shieldBefore - 20.f, shield.GetShield());
		f.ship->SetInvulnerability(true); Hit(*f.ship, nullptr, 20.f, Plain()); f.Equal("blocked hits create no deferred debt", 0.f, f.ship->GetDeferredDamageRuntime().GetPendingDamage()); f.ship->SetInvulnerability(false); f.Remove(reinstalled);
	}
	void PatchChecks(Report& report, Application& application)
	{
		for (const auto rarity : { ModuleRarity::Uncommon, ModuleRarity::Rare, ModuleRarity::Epic })
		{
			Fixture f{ report, application }; const auto host = Host(f, rarity); const std::string suffix = rarity == ModuleRarity::Uncommon ? "" : rarity == ModuleRarity::Rare ? ".Rare" : ".Epic";
			f.Install(host->moduleId.ToString().c_str(), 1, 3, Shared("D6" + suffix).c_str()); NoArmor(f); auto& health = f.ship->GetHealthComponent();
			health.ChangeHealth(.4f * health.GetMaxHealth() - health.GetHealth()); const float before = health.GetHealth();
			Use(f, "Ability.Defense.DirectionalBarrier.Basic", sas::AbilitySlot::Ability1); const float fraction = rarity == ModuleRarity::Uncommon ? .12f : rarity == ModuleRarity::Rare ? .21f : .3f;
			f.Equal("Desperate Patch heals at the inclusive forty-percent threshold", before + health.GetMaxHealth() * fraction, health.GetHealth());
			const float healed = health.GetHealth(); Use(f, "Ability.Defense.DirectionalBarrier.Basic", sas::AbilitySlot::Ability1); f.Equal("Desperate Patch does not heal above threshold", healed, health.GetHealth());
			health.ChangeHealth(.3f * health.GetMaxHealth() - health.GetHealth()); const float low = health.GetHealth(); CastMovement(f); f.Equal("Desperate Patch excludes Movement uses", low, health.GetHealth());
		}
	}
	void MomentumChecks(Report& report, Application& application)
	{
		{
			Fixture f{ report, application }; f.Install(kDrive, 1, 3, "Module.SideFeature.MomentumDrive.SpeedEcho"); CastMovement(f); const float doubled = f.Owner(OwnerAttributeIds::MoveSpeedHorizontal);
			f.ship->GetModuleTriggerRuntime().Tick(4.f); CastMovement(f); f.ship->GetModuleTriggerRuntime().Tick(2.9f); f.Equal("Speed Echo adds two seconds to one remaining second", doubled, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			f.ship->GetModuleTriggerRuntime().Tick(.2f); report.Check(f.Owner(OwnerAttributeIds::MoveSpeedHorizontal) < doubled, "extension expires after three seconds, not a refreshed five");
			CastMovement(f); for (int i = 0; i < 8; ++i) CastMovement(f); f.ship->GetModuleTriggerRuntime().Tick(9.9f); f.Equal("Speed Echo caps remaining duration at ten seconds", doubled, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal));
			f.ship->GetModuleTriggerRuntime().Tick(.2f); report.Check(f.Owner(OwnerAttributeIds::MoveSpeedHorizontal) < doubled, "ten-second cap expires");
		}
		{
			Fixture f{ report, application }; NoArmor(f); const auto enemy = Enemy(f); f.Install(kDrive, 1, 1, "Module.SideFeature.MomentumDrive.FlowShield");
			f.Equal("Flow Shield is closed before Movement use", 10.f, Hit(*f.ship, enemy.get(), 10.f, Plain())); CastMovement(f);
			f.Equal("Flow Shield reduces damage during doubling", 8.5f, Hit(*f.ship, enemy.get(), 10.f, Plain())); f.ship->Tick(5.01f);
			f.Equal("Flow Shield ends with the doubling window", 10.f, Hit(*f.ship, enemy.get(), 10.f, Plain()));
		}
	}
	void OverflowChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; const auto instance = f.Install(kField, 1, 3, "Module.SideFeature.FieldMastery.MasteryOverflow");
		const auto run = std::make_shared<FieldMasteryRunState>(); f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().SetFieldMasteryRunState(run);
		auto& ability = Use(f, "Ability.Utility.CryoBola.Basic", sas::AbilitySlot::Ability1); auto& asc = f.ship->GetAbilitySystemComponent();
		report.Check(ability.GetMaxLevel() == GameAbilityDefinition::kUnboundedAbilityLevel, "shipped Utility progression retains its existing unbounded technical ceiling");
		ability.ConfigureInvocationLevel(1, 3); report.Check(asc.SetAbilityLevel(ability.GetHandle(), ability.GetMaxLevel()), "bound only the test invocation and reach its maximum");
		for (int i = 0; i < 14; ++i) Use(f, "Ability.Utility.CryoBola.Basic", sas::AbilitySlot::Ability1);
		const List<GameplayTag> utility{ GameplayTags::Ability::Utility };
		f.Equal("fourteen maximum-level uses do not grant overflow", 1.f, f.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
		Use(f, "Ability.Utility.CryoBola.Basic", sas::AbilitySlot::Ability1); f.Equal("fifteenth use grants five percent to Utility main effects", 1.05f, f.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
		f.Equal("overflow excludes Offense", 1.f, f.ship->GetModuleApplier().GetAbilityMainEffectMultiplier({ GameplayTags::Ability::Offense }));
		Use(f, "Ability.Utility.CryoBola.Basic", sas::AbilitySlot::Ability1); f.Equal("a real Utility activation captures the earned effect multiplier", 1.05f, ability.GetDefinition().mainEffectMultiplier);
		auto& other = Use(f, "Ability.Utility.GravityAnomaly.Basic", sas::AbilitySlot::Ability4); other.ConfigureInvocationLevel(1, 3); report.Check(asc.SetAbilityLevel(other.GetHandle(), other.GetMaxLevel()), "another bounded Utility invocation reaches its own maximum");
		for (int i = 0; i < 14; ++i) Use(f, "Ability.Utility.GravityAnomaly.Basic", sas::AbilitySlot::Ability4);
		f.Equal("overflow counters belong to each ability, not a pooled sum", 1.05f, f.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
		Use(f, "Ability.Utility.GravityAnomaly.Basic", sas::AbilitySlot::Ability4); f.Equal("another fifteen maximum-level uses add a second five percent", 1.1f, f.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
		f.Remove(instance); for (int i = 0; i < 15; ++i) Use(f, "Ability.Utility.CryoBola.Basic", sas::AbilitySlot::Ability1);
		f.Equal("earned Utility bonus persists after removal; counters freeze", 1.1f, f.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
		Fixture respawn{ report, application }; respawn.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().SetFieldMasteryRunState(run);
		f.Equal("a replacement ship reads the same run-owned overflow bonus", 1.1f, respawn.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
		Fixture fresh{ report, application }; f.Equal("a new run state starts without overflow", 1.f, fresh.ship->GetModuleApplier().GetAbilityMainEffectMultiplier(utility));
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets(); if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "side-pool content registration failed" };
		std::string title = "Module side pools E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		PoolChecks(report); ScaleChecks(report, application); OtherScaleChecks(report, application); ZoneChecks(report, application); RecoveryChecks(report, application); DeferredChecks(report, application); PatchChecks(report, application); MomentumChecks(report, application); OverflowChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "ModuleSidePoolsE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
