#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/ModuleDescriptionValues.h"

namespace
{
	using namespace uncommon_p6_e2e;
	using ReadValue = std::function<float(const ResolvedModuleEffects&)>;

	float OwnerModifier(const ResolvedModuleEffects& effects, const sas::AttributeId& id)
	{
		for (const auto& modifier : effects.ownerModifiers) if (modifier.attributeId == id) return modifier.magnitude;
		throw std::runtime_error{ "missing resolved owner modifier" };
	}

	void CheckProgression(Report& report, Application& application, const char* id, const char* placeholder, float first, float step, ReadValue read, float ratio = 0.f, float added = 0.f, float largerFirst = 0.f)
	{
		const auto* module = content::ModuleContentCatalog::FindById(id);
		report.Check(module != nullptr, "progression uses shipped module", id);
		for (int variant = 0; variant < static_cast<int>(module->sizeVariants.size()); ++variant)
			for (int placementLevel = 1; placementLevel <= 3; ++placementLevel)
				for (const auto& zone : { module->family == GameplayTags::Ability::Primary ? GameplayTags::Module::Zone::Engine : GameplayTags::Module::Zone::Nose, GameplayTags::Module::Zone::Hull })
				{
					Fixture f{ report, application };
					ShipSchematic schematic; schematic.width = 4; schematic.height = 4; schematic.cellZones.assign(16, zone);
					ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
					ModuleLoadout loadout;
					const auto instance = loadout.AddToInventory(rules, *module, variant, module->rarity, placementLevel);
					report.Check(instance && loadout.Install(rules, instance, { 0, 0 }, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(loadout, rules), "load, place and apply shipped progression");
					const auto& effects = f.ship->GetModuleApplier().GetResolvedEffects();
					const int effective = placementLevel + ModuleResolver::FindZoneLevelBonus(effects, instance);
					float expected = variant > 0 && largerFirst > 0.f ? largerFirst : first;
					for (int l = 1; l < effective; ++l) expected = ratio > 0.f ? expected * ratio + added : expected + step;
					const std::string input = std::string{ id } + ", variant " + std::to_string(variant) + ", placement L" + std::to_string(placementLevel) + ", effective L" + std::to_string(effective);
					report.CheckValue("runtime resolves progression", expected, read(effects), input);
					bool described = false;
					VisitModuleDescriptionValues(module->sizeVariants[variant], effective, [&](const std::string& key, float value)
					{
						if (key != placeholder) return;
						described = true;
						report.CheckValue("description reads the same effective level", expected, value, input);
					});
					report.Check(described, "progression exposes its description placeholder", placeholder);
				}
	}

	void ProgressionChecks(Report& report, Application& application)
	{
		CheckProgression(report, application, "Module.Defense.HeavyArmorPlate.Basic", "Owner.Armor", 30.f, 4.f, [](const auto& e) { return OwnerModifier(e, OwnerAttributeIds::Armor); });
		CheckProgression(report, application, "Module.Offense.AssaultCore.Basic", "Owner.AttackPower", 15.f, 2.f, [](const auto& e) { return OwnerModifier(e, OwnerAttributeIds::AttackPower); });
		CheckProgression(report, application, "Module.Primary.FocusedBarrel.Basic", "damage.0.damageMultiplier", 1.3f, .04f, [](const auto& e) { return e.damageRules.at(0).rule.damageMultiplier; });
		CheckProgression(report, application, "Module.Movement.ThrustBooster.Basic", "Owner.MoveSpeedHorizontal", 2.2f, .3f, [](const auto& e) { return OwnerModifier(e, OwnerAttributeIds::MoveSpeedHorizontal); });
		CheckProgression(report, application, "Module.Offense.AttackRhythm.Basic", "Owner.AttackSpeed", 15.f, 2.f, [](const auto& e) { return e.triggerBuffs.at(0).rule.modifiers.at(0).modifier.magnitude; });
		CheckProgression(report, application, "Module.Defense.ShatterCurrent.Basic", "damage.0.bonusDamagePercent", 30.f, 4.f, [](const auto& e) { return e.damageRules.at(0).rule.bonusDamageFraction * 100.f; });
		CheckProgression(report, application, "Module.Offense.EnergyFeedback.Basic", "Owner.Energy.Power", 16.f, 2.f, [](const auto& e) { return e.triggerBuffs.at(0).rule.modifiers.at(0).modifier.magnitude; });
		CheckProgression(report, application, "Module.Movement.EscapeReflex.Basic", "trigger.0.movementSpeedMultiplier", 1.25f, .03f, [](const auto& e) { return e.triggerBuffs.at(0).rule.movementSpeedMultiplier; });
		CheckProgression(report, application, "Module.Offense.FinishingBlow.Basic", "damage.0.damageMultiplier", 1.2f, .03f, [](const auto& e) { return e.damageRules.at(0).rule.damageMultiplier; });
		CheckProgression(report, application, "Module.Primary.FullMagazine.Basic", "damage.0.damageMultiplier", 1.3f, .04f, [](const auto& e) { return e.damageRules.at(0).rule.damageMultiplier; });
		CheckProgression(report, application, "Module.Defense.BarrierRecharge.Basic", "action.0.amount", .1f, .015f, [](const auto& e) { return e.eventActions.at(0).rule.amount; });
		CheckProgression(report, application, "Module.Defense.BraceReflex.Basic", "Owner.Armor", 25.f, 3.f, [](const auto& e) { return e.triggerBuffs.at(0).rule.modifiers.at(0).modifier.magnitude; });
		CheckProgression(report, application, "Module.Defense.OvershieldLayer.Uncommon", "action.0.amount", .2f, .03f, [](const auto& e) { return e.eventActions.at(0).rule.amount; });
		CheckProgression(report, application, "Module.Primary.StatusPrimer.Uncommon", "statusPrimer.0.cooldownSeconds", 1.f, -.1f, [](const auto& e) { return e.statusRules.primaryStatusPrimers.at(0).cooldownSeconds; });
		CheckProgression(report, application, "Module.Utility.FrostLock.Uncommon", "freeze.0.durationSeconds", 1.f, .2f, [](const auto& e) { return e.statusRules.cryoFreezes.at(0).durationSeconds; });
		CheckProgression(report, application, "Module.Movement.SpeedLance.Rare", "speedLance.0.speedDamageCoefficient", .4f, .05f, [](const auto& e) { return e.generatedRules.speedLances.at(0).rule.speedDamageCoefficient; });
		CheckProgression(report, application, "Module.Movement.PhaseSlip.Epic", "phaseSlip.0.explosionRadius", 120.f, 10.f, [](const auto& e) { return e.epicRules.phaseSlips.at(0).explosionRadius; });
		CheckProgression(report, application, "Module.Movement.PhaseSlip.Epic", "phaseSlip.0.primaryDamageMultiplier", 2.f, .25f, [](const auto& e) { return e.epicRules.phaseSlips.at(0).primaryDamageMultiplier; });
		CheckProgression(report, application, "Module.Utility.ReleaseValve.Rare", "annularBurst.0.baseDamage", 50.f, 0.f, [](const auto& e) { return e.generatedRules.annularBursts.at(0).rule.baseDamage; }, 1.05f, 5.f);
		CheckProgression(report, application, "Module.Utility.ReleaseValve.Rare", "annularBurst.0.energyPowerPercent", 40.f, 0.f, [](const auto& e) { return e.generatedRules.annularBursts.at(0).rule.energyPowerCoefficient * 100.f; }, 1.05f);
		CheckProgression(report, application, "Module.Offense.GlassTopCore.Uncommon", "Owner.AttackPower", 12.f, 0.f, [](const auto& e) { return OwnerModifier(e, OwnerAttributeIds::AttackPower); }, 1.17f, 0.f, 34.f);
		CheckProgression(report, application, "Module.Offense.DyingStar.Rare", "ownerBurst.0.damage", 60.f, 0.f, [](const auto& e) { return e.ownerBurstRules.at(0).rule.damage; }, 1.15f);
		CheckProgression(report, application, "Module.Offense.ShieldRupture.Rare", "rupture.0.damagePercent", 50.f, 5.f, [](const auto& e) { return e.shieldRuptureRules.at(0).rule.damageFraction * 100.f; });
	}

	void ZoneCoolerChecks(Report& report, Application& application)
	{
		const auto* module = content::ModuleContentCatalog::FindById("Module.Utility.ZoneCooler.Basic");
		for (const auto& zone : { GameplayTags::Module::Zone::Sensor, GameplayTags::Module::Zone::Hull })
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				ShipSchematic schematic; schematic.width = 4; schematic.height = 4; schematic.cellZones.assign(16, zone);
				ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
				ModuleLoadout loadout; const auto id = loadout.AddToInventory(rules, *module, 0, module->rarity, level);
				report.Check(loadout.Install(rules, id, { 0, 0 }, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(loadout, rules), "Zone Cooler applies in its selected region");
				const auto& effects = f.ship->GetModuleApplier().GetResolvedEffects();
				const int effective = level + ModuleResolver::FindZoneLevelBonus(effects, id);
				const bool hull = zone == GameplayTags::Module::Zone::Hull;
				report.CheckValue("Zone Cooler reads effective level", (hull ? .9f : .79f) - (hull ? .01f : .02f) * (effective - 1), effects.cooldownRules.at(0).multiplier, std::to_string(effective));
			}
	}

	void LoaderChecks(Report& report, Application& application)
	{
		Json source; std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" }; input >> source;
		auto& rule = source["modules"][0];
		const Json modifier = { { "target", "Owner" }, { "attributeId", "Owner.AttackPower" }, { "operation", "Add" }, { "magnitude", {10,20,40} } };
		const Json variant = { { "shape", {{0,0}} }, { "attributeModifiers", Json::array({ modifier }) } };
		const Json baseModule = { { "id", "Module.Offense.GrowthProbe.Rare" }, { "family", "Ability.Offense" }, { "levelCount", 3 }, { "sizeVariants", Json::array({ variant }) } };
		rule = baseModule;
		const auto rejects = [&](Json growth, bool scalar = false)
		{
			Json data = source; auto& modifier = data["modules"][0]["sizeVariants"][0]["attributeModifiers"][0]; modifier["magnitudeGrowth"] = growth; if (scalar) modifier["magnitude"] = 10;
			const auto loaded = content::ModuleLoader::LoadFromJson(data);
			report.Check(!loaded.Succeeded() && loaded.definitions.empty(), "invalid growth is rejected without partial content", growth);
		};
		rejects("unknown"); rejects({ { "type", "compound" }, { "ratio", 0 } }); rejects({ { "type", "compound" }, { "ratio", "1.05" } }); rejects({ { "type", "step" }, { "every", 0 }, { "amount", 1 } }); rejects({ { "type", "step" }, { "every", 1.5 }, { "amount", 1 } }); rejects({ { "type", "step" }, { "every", 2 } }); rejects({ { "type", "linear" }, { "ratio", 2 } }); rejects("compound", true);
		for (const Json growth : { Json("linear"), Json("compound"), Json{ { "type", "compound" }, { "ratio", 1.5 }, { "amount", 5 } } })
		{
			Json data = source; data["modules"][0]["sizeVariants"][0]["attributeModifiers"][0]["magnitudeGrowth"] = growth;
			const auto loaded = content::ModuleLoader::LoadFromJson(data);
			report.Check(loaded.Succeeded(), "valid growth loads through public content API", loaded.error);
			const auto& module = loaded.definitions.front();
			Fixture f{ report, application }; ShipSchematic schematic; schematic.width = 4; schematic.height = 4; schematic.cellZones.assign(16, GameplayTags::Module::Zone::Hull);
			ModuleRulesContext rules{ schematic, [&module](const sas::ContentId& id) { return id == module.moduleId ? &module : nullptr; }, f.rules.zoneTable, {} };
			ModuleLoadout loadout; const auto id = loadout.AddToInventory(rules, module, 0, module.rarity, 3);
			report.Check(loadout.Install(rules, id, {0,0}, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(loadout, rules), "growth probe uses real placement and ship applier");
			const float expected = growth.is_string() ? (growth == "linear" ? 80.f : 160.f) : 102.5f;
			report.CheckValue("public loader growth reaches L5 runtime", expected, OwnerModifier(f.ship->GetModuleApplier().GetResolvedEffects(), OwnerAttributeIds::AttackPower), growth.dump());
		}
	}

	void CascadeChecks(Report& report, Application& application)
	{
		for (const char* id : { "Ability.Defense.Shield.Basic", "Ability.Defense.ReclaimerProtocol.Basic", "Ability.Movement.Dash.Basic", "Ability.Utility.GravityAnomaly.Basic", "Ability.Offense.HullShock.Basic", "Ability.Offense.GlacialPressure.Basic", "Ability.Defense.CrystalBarricade.Basic", "Ability.Offense.CombatSentry.Basic" })
		{
			Fixture f{ report, application }; const auto instance = f.Install("Module.Utility.CascadeReactor.Epic", 0, 1);
			auto& abilities = f.ship->GetAbilitySystemComponent();
			abilities.GetAttributes().SetBaseValue(OwnerAttributeIds::EnergyPower, 200.f);
			if (auto* previous = abilities.GetAbility(sas::AbilitySlot::Ability1)) report.Check(abilities.RemoveAbility(previous->GetHandle()), "replace fixture Q");
			auto definition = *content::AbilityContentCatalog::FindById(id); definition.slot = sas::AbilitySlot::Ability1;
			const auto handle = abilities.GrantAbility(definition); auto* ability = abilities.GetAbility(handle);
			report.Check(ability != nullptr && ability->TryActivate(), "activate shipped main effect without chain");
			ability->Cancel(sas::AbilityEndReason::Completed); ability->Tick(100.f);
			AbilityExecutionContext context{ &abilities, &ability->GetDefinition(), nullptr, ability };
			auto probe = ability->GetDefinition().attributes;
			for (const auto& spec : definition.effectSpecs) probe.insert(probe.end(), spec.attributes.begin(), spec.attributes.end());
			const std::string abilityId{ id };
			const char* actorId = abilityId == "Ability.Utility.GravityAnomaly.Basic" ? "Actor.Ability.GravityAnomaly.Field.Basic" : abilityId == "Ability.Defense.CrystalBarricade.Basic" ? "Actor.Ability.CrystalBarricade.Wall.Basic" : abilityId == "Ability.Offense.CombatSentry.Basic" ? "Actor.Ability.CombatSentry.Turret.Basic" : nullptr;
			if (actorId)
			{
				const auto* actor = content::AbilityContentCatalog::FindActorById(actorId);
				report.Check(actor != nullptr, "main effect probe uses the shipped actor contract");
				probe.insert(probe.end(), actor->attributes.begin(), actor->attributes.end());
			}
			const auto baseline = AbilityActionAttributeResolver::ResolveAttributes(context, nullptr, probe);
			f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().Tick(6.f);
			const auto& tags = definition.abilityTags;
			const bool offense = ModuleTriggerEventTagsMatch({ GameplayTags::Ability::Offense }, tags);
			f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().OnAbilityUse("probe.previous", { offense ? GameplayTags::Ability::Movement : GameplayTags::Ability::Offense });
			const int level = 1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), instance);
			const float multiplier = 1.2f + .02f * (level - 1);
			report.Check(ability->TryActivate(), "activate the powered shipped main effect");
			report.CheckValue("main multiplier is captured before behavior activation", multiplier, ability->GetDefinition().mainEffectMultiplier, id);
			const auto powered = AbilityActionAttributeResolver::ResolveAttributes(context, nullptr, probe);
			for (const auto& value : baseline)
			{
				const bool main = std::find(definition.mainEffectAttributes.begin(), definition.mainEffectAttributes.end(), value.id) != definition.mainEffectAttributes.end();
				report.CheckValue(main ? "main quantity grows" : "duration and delivery data stay fixed", std::clamp(value.currentValue * (main ? multiplier : 1.f), value.minValue, value.maxValue), sas::FindAttributeValue(powered, value.id, -1.f), std::string{ id } + " " + std::string{ value.id.GetName() });
			}
			const auto value = [](const sas::GameplayAttributeList& values, const char* key) { return sas::FindAttributeValue(values, sas::AttributeId{ key }, -1.f); };
			if (abilityId == "Ability.Offense.GlacialPressure.Basic")
			{
				const auto distance = [&](const auto& values)
				{
					return value(values, "Ability.Offense.GlacialPressure.PushDistance") * (1.f + std::max(0.f, f.Owner(OwnerAttributeIds::MaxHealth) - value(values, "Ability.Offense.GlacialPressure.MaxHealthReference")) * value(values, "Ability.Offense.GlacialPressure.MaxHealthPushScale"));
				};
				report.CheckValue("Glacial push quantity receives Cascade exactly once", distance(baseline) * multiplier, distance(powered), "base distance times health-derived factor");
			}
			if (abilityId == "Ability.Defense.CrystalBarricade.Basic")
			{
				const auto ricochet = [&](const auto& values)
				{
					return value(values, "AbilityActor.CrystalBarricade.Wall.BaseRicochetMultiplier") * (1.f + f.Owner(OwnerAttributeIds::EnergyPower) / value(values, "AbilityActor.CrystalBarricade.Wall.EnergyPowerRicochetReference") * value(values, "AbilityActor.CrystalBarricade.Wall.EnergyPowerRicochetScale"));
				};
				report.CheckValue("Crystal ricochet quantity receives Cascade exactly once", ricochet(baseline) * multiplier, ricochet(powered), "base multiplier times EP-derived factor");
			}
			const int step = f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().GetCascadeStep(instance);
			report.Check(!ability->TryActivate(), "active or cooling down ability cannot activate twice");
			report.Check(f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime().GetCascadeStep(instance) == step, "failed activation cannot advance Cascade");
		}
	}

	struct ReflectionObserver
	{
		CombatRuntime& runtime;
		DelegateHandle handle;
		float damage = 0.f;
		explicit ReflectionObserver(CombatRuntime& owner) : runtime{ owner }, handle{ owner.onDamageResolved.BindAction(this, &ReflectionObserver::OnDamage) } {}
		~ReflectionObserver() { runtime.onDamageResolved.UnbindAction(handle); }
		void OnDamage(const DamageContext& context) { damage = context.originalDamage; }
	};

	void ReflectionChecks(Report& report, Application& application)
	{
		Fixture f{ report, application }; const auto enemy = Enemy(f);
		const auto reflection = f.Install("Module.Defense.ReflectionShield.Uncommon");
		f.Install("Module.Defense.ImpactRedirector.Rare");
		f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
		report.Check(f.ship->GetShipRuntime().SetBaseMaxShieldContribution("Test.Reflection.Capacity", 500.f), "reflection test provides sufficient normal shield");
		f.ship->GetShieldComponent().ChangeShield(10000.f);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		if (auto* old = abilities.GetAbility(sas::AbilitySlot::Ability1)) report.Check(abilities.RemoveAbility(old->GetHandle()), "replace Q for a real Defense window");
		auto defense = *content::AbilityContentCatalog::FindById("Ability.Defense.DirectionalBarrier.Basic"); defense.slot = sas::AbilitySlot::Ability1;
		auto* ability = abilities.GetAbility(abilities.GrantAbility(defense));
		report.Check(ability && ability->TryActivate(), "real Defense activation opens Reflection window"); ability->Cancel(sas::AbilityEndReason::Completed);
		ReflectionObserver observer{ enemy->GetCombatRuntime() };
		const float before = f.ship->GetShieldComponent().GetShield();
		Hit(*f.ship, enemy.get(), 100.f, Plain());
		const float lost = before - f.ship->GetShieldComponent().GetShield();
		report.Check(lost > 0.f && lost < 100.f, "Impact Redirector reduces actual shield loss", lost);
		const int level = 1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), reflection);
		report.CheckValue("Reflection uses damage arriving before Impact Redirector", 100.f * (.35f + .05f * (level - 1)), observer.damage, "100 raw damage, 100 Armor");
	}

	void StepReviveChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application }; const auto enemy = Enemy(f);
			ShipSchematic schematic; schematic.width = 4; schematic.height = 4; schematic.cellZones.assign(16, GameplayTags::Module::Zone::Hull);
			ModuleRulesContext rules{ schematic, f.rules.lookup, f.rules.zoneTable, f.rules.sideFeatureLookup };
			const auto* module = rules.lookup("Module.Defense.LastStand.Epic"); ModuleLoadout loadout;
			const auto id = loadout.AddToInventory(rules, *module, 0, module->rarity, level);
			report.Check(loadout.Install(rules, id, {0,0}, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(loadout, rules), "Last Stand installs in Hull");
			const int effective = level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			const int charges = 1 + (effective - 1) / 2;
			for (int use = 0; use < charges; ++use)
			{
				Hit(*f.ship, enemy.get(), 100000.f, Plain());
				report.Check(!f.ship->GetIsPendingDestroy(), "step growth grants the expected revive", Json{ { "effectiveLevel", effective }, { "use", use + 1 }, { "charges", charges } });
			}
			Hit(*f.ship, enemy.get(), 100000.f, Plain());
			report.Check(f.ship->GetIsPendingDestroy(), "L3/L4/L5 have exactly 2/2/3 revives", effective);
		}
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext testContext; vega::EngineContext::Scope testScope{ testContext }; vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "growth content registration failed" };
		std::string title = "Module progression E2E"; Application application{ {64u,64u}, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ProgressionChecks(report, application); ZoneCoolerChecks(report, application); LoaderChecks(report, application); CascadeChecks(report, application); ReflectionChecks(report, application); StepReviveChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "ModuleLevelGrowthE2E"; WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
