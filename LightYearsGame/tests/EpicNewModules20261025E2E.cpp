#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/module/ModuleDescriptionValues.h"
#include "gameConfigs/ability/AbilityCatalog.h"

namespace
{
	using namespace vega;
	using namespace common_module_e2e;
	using uncommon_p4_e2e::Fixture;
	using uncommon_p4_e2e::Enemy;
	using uncommon_p4_e2e::Hit;
	using uncommon_p4_e2e::Plain;
	constexpr const char* kBarrel = "Module.Utility.ResonantBarrel.Epic";
	constexpr const char* kCore = "Module.Offense.MonochromeCore.Epic";
	const std::array<GameplayTag, 7> kZones = { GameplayTags::Module::Zone::Wings, GameplayTags::Module::Zone::Core, GameplayTags::Module::Zone::Engine, GameplayTags::Module::Zone::Sensor, GameplayTags::Module::Zone::Hull, GameplayTags::Module::Zone::Nose, GameplayTags::Module::Zone::AbilityBay };
	const std::array<sas::AbilitySlot, 4> kSlots = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };

	struct EpicFixture : Fixture
	{
		ShipSchematic schematic;
		ModuleRulesContext zoneRules;
		int firstRow = 0;
		EpicFixture(Report& result, Application& application) : Fixture{ result, application }, schematic{ rules.schematic }, zoneRules{ schematic, rules.lookup, rules.zoneTable, rules.sideFeatureLookup }
		{
			firstRow = schematic.height;
			schematic.height += static_cast<int>(kZones.size());
			schematic.cellZones.resize(static_cast<std::size_t>(schematic.width * schematic.height));
			schematic.cellLocked.resize(schematic.cellZones.size(), 0);
			for (std::size_t row = 0; row < kZones.size(); ++row)
				for (int x = 0; x < schematic.width; ++x) schematic.cellZones[static_cast<std::size_t>((firstRow + static_cast<int>(row)) * schematic.width + x)] = kZones[row];
		}
		std::uint32_t InstallIn(const char* moduleId, int zoneIndex, int levelValue = 1, int x = 0)
		{
			const auto* module = zoneRules.lookup(moduleId);
			report.Check(module != nullptr, "new Epic fixture finds shipped module", moduleId);
			const auto id = loadout.AddToInventory(zoneRules, *module, 0, module->rarity, levelValue);
			const auto failure = loadout.Install(zoneRules, id, { x, firstRow + zoneIndex }, 0);
			report.Check(id && failure == ModulePlacementFailure::None && ship->GetModuleApplier().Apply(loadout, zoneRules), "new Epic installs through loadout and real ship applier", Json{ { "module", moduleId }, { "zone", kZones[zoneIndex].name }, { "level", levelValue }, { "failure", static_cast<int>(failure) } });
			return id;
		}
		void RemoveFromGrid(std::uint32_t id)
		{
			report.Check(loadout.Uninstall(id) && ship->GetModuleApplier().Apply(loadout, zoneRules), "new Epic uninstall applies immediately");
		}
	};

	List<std::pair<GameAbility*, float>> StartCooldowns(EpicFixture& f)
	{
		// Keep the real Utility/Offense defaults, and equip real Movement/Defense abilities to cover all four family boundaries.
		auto& asc = f.ship->GetAbilitySystemComponent();
		for (const auto& [slot, id] : { std::pair{ kSlots[1], "Ability.Movement.Dash.Basic" }, std::pair{ kSlots[3], "Ability.Defense.Shield.Basic" } })
		{
			asc.ClearAbilitySlot(slot);
			auto definition = *content::AbilityContentCatalog::FindById(id);
			definition.slot = slot;
			f.report.Check(asc.GrantAbility(definition).IsValid(), "cooldown fixture equips a real family ability", id);
		}
		List<std::pair<GameAbility*, float>> result;
		for (const auto slot : kSlots)
		{
			auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(slot);
			f.report.Check(ability != nullptr, "all four shipped loadout abilities exist");
			if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
			ability->Tick(1000.f);
			f.report.Check(ability->TryActivate(), "shipped loadout ability starts a real cooldown", ability->GetDefinition().abilityId);
			if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
			f.report.Check(ability->GetCooldownRemaining() > 0.f, "cooldown is running");
			result.push_back({ ability, ability->GetCooldownRemaining() });
		}
		return result;
	}

	DamagePayload Shot(bool critical = false)
	{
		auto payload = Plain();
		payload.criticalPolicy = critical ? DamageCriticalPolicy::Guaranteed : DamageCriticalPolicy::Disabled;
		payload.hitGroup = std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot);
		return payload;
	}

	float DescriptionValue(const ModuleRulePackage& rules, int level, const char* wanted)
	{
		float result = -1.f;
		VisitModuleDescriptionValues(rules, level, [&](const std::string& key, float value) { if (key == wanted) result = value; });
		return result;
	}

	void CatalogAndLoaderChecks(Report& report)
	{
		report.artifact["abilityDamageTypes"] = Json::array();
		report.artifact["multiTypeShippedAbilities"] = Json::array();
		for (const auto* ability : content::AbilityContentCatalog::GetDefinitions())
		{
			Json tags = Json::array();
			for (const auto& tag : ability->damageTags) tags.push_back(tag.name);
			report.artifact["abilityDamageTypes"].push_back(Json{ { "id", ability->abilityId }, { "damageTags", tags } });
			if (tags.size() > 1) report.artifact["multiTypeShippedAbilities"].push_back(ability->abilityId);
		}
		for (const auto* id : { kBarrel, kCore })
		{
			const auto* module = content::ModuleContentCatalog::FindById(id);
			report.Check(module && module->rarity == ModuleRarity::Epic && module->levelCount == 3 && module->sizeLadderCellCounts == List<int>{ 2 } && module->sizeVariants.size() == 1 && module->sizeVariants.front().shape.cells == List<sf::Vector2i>{ { 0, 0 }, { 1, 0 } } && module->sideFeatures.empty() && module->sharedPools.empty(), "shipped Epic is fixed two-cell with no sides", id);
			report.Check(module->displayName == (id == kBarrel ? "Resonant Barrel" : "Monochrome Core") && module->description.find('{') != std::string::npos, "English names and placeholder descriptions ship", module->displayName);
			for (int levelValue = 1; levelValue <= 5; ++levelValue)
				report.Check(FormatModuleDescription(*module, module->sizeVariants.front(), levelValue).find('{') == std::string::npos, "description formatter resolves all new placeholders through L5");
		}
		report.Check(content::ModuleContentCatalog::FindById(kCore)->family == GameplayTags::Ability::Offense, "Monochrome Core is Offense");
		report.Check(!content::ModuleContentCatalog::FindById("Module.Utility.UltimateProtocol.Epic"), "Ultimate Protocol is not added");
		const Json zone = { { "zone", "Module.Zone.Wings" }, { "scope", "FamilyOfZone" }, { "kind", "PrimaryHitCooldownSeconds" }, { "seconds", { .25f, .30f, .35f } } };
		const Json core = { { "pairMultiplier", { 1.25f, 1.30f, 1.35f } }, { "additionalAbilityBonus", .25f } };
		const auto parse = [](const char* key, const Json& rule, bool side = false) { return content::ModuleLoader::ParseRulePackage(Json{ { key, Json::array({ rule }) } }, "Epic new module fixture", 3, true, side); };
		const auto reject = [&](const char* key, const Json& rule, const char* label, bool side = false)
		{
			bool rejected = false;
			try { parse(key, rule, side); } catch (const std::exception&) { rejected = true; }
			report.Check(rejected, label, rule);
		};
		report.Check(parse("zoneEffects", zone).zoneEffects.front().kind == ModuleZoneEffectKind::PrimaryHitCooldownSeconds && parse("monochromeCoreRules", core).epicOffense.monochromeCores.size() == 1, "loader accepts both new typed rules");
		Json bad = zone; bad["seconds"] = { .25, 0., .35 }; reject("zoneEffects", bad, "rejects zero seconds at a later level");
		bad = zone; bad["seconds"] = -1; reject("zoneEffects", bad, "rejects negative seconds");
		bad = zone; bad["seconds"] = { .25, .30 }; reject("zoneEffects", bad, "rejects incomplete seconds table");
		bad = zone; bad["kind"] = "Typo"; reject("zoneEffects", bad, "rejects unknown zone effect kind");
		bad = zone; bad["multiplier"] = .8; reject("zoneEffects", bad, "rejects mixed multiplier and refund fields");
		reject("zoneEffects", zone, "hit refunds are main-only", true);
		bad = core; bad["pairMultiplier"] = { 1.25, 1., 1.35 }; reject("monochromeCoreRules", bad, "rejects a later pair multiplier of one");
		bad = core; bad["pairMultiplier"] = { 1.25, 1.30 }; reject("monochromeCoreRules", bad, "rejects incomplete pair multiplier table");
		bad = core; bad["additionalAbilityBonus"] = 0; reject("monochromeCoreRules", bad, "rejects zero additional ability bonus");
		bad = core; bad["pairMultplier"] = 1.25; reject("monochromeCoreRules", bad, "rejects unknown Monochrome field");
		bad = core; bad.erase("additionalAbilityBonus"); reject("monochromeCoreRules", bad, "requires additional ability bonus");
		reject("monochromeCoreRules", core, "Monochrome is main-only", true);
		for (const char* name : { "Module.Zone.Nose", "Module.Zone.Hull", "Module.Zone.Missing" })
		{
			bad = zone; bad["zone"] = name;
			bool rejected = false;
			try { const auto package = parse("zoneEffects", bad); rejected = !content::ModuleLoader::ValidateRulePackageZoneEffects(package, content::ModuleContentCatalog::GetZoneTable(), "zone validation"); }
			catch (const std::exception&) { rejected = true; }
			report.Check(rejected, "refund validates zone existence and non-Primary family scope", name);
		}
		bad = zone; bad["scope"] = "AllActiveNonPrimary";
		report.Check(!content::ModuleLoader::ValidateRulePackageZoneEffects(parse("zoneEffects", bad), content::ModuleContentCatalog::GetZoneTable(), "zone validation"), "all-family refund rejects a single-family zone");
		bad["zone"] = "Module.Zone.Hull";
		report.Check(content::ModuleLoader::ValidateRulePackageZoneEffects(parse("zoneEffects", bad), content::ModuleContentCatalog::GetZoneTable(), "zone validation"), "all-family refund accepts Hull");
	}

	void BarrelZoneChecks(Report& report, Application& application)
	{
		for (int zoneIndex = 0; zoneIndex < static_cast<int>(kZones.size()); ++zoneIndex)
			for (int level = 1; level <= 3; ++level)
			{
				EpicFixture f{ report, application };
				if (kZones[zoneIndex] == GameplayTags::Module::Zone::AbilityBay)
				{
					for (const auto* moduleId : { kBarrel, kCore })
					{
						const auto* module = f.rules.lookup(moduleId);
						const auto id = f.loadout.AddToInventory(f.zoneRules, *module, 0, module->rarity, level);
						report.Check(id && f.loadout.Install(f.zoneRules, id, { 0, f.firstRow + zoneIndex }, 0) == ModulePlacementFailure::SystemZoneMismatch, "new Epic respects AbilityBay's system-only placement contract", moduleId);
					}
					continue;
				}
				const auto id = f.InstallIn(kBarrel, zoneIndex, level);
				const auto* module = f.rules.lookup(kBarrel);
				const auto& effects = f.ship->GetModuleApplier().GetResolvedEffects();
				const auto* zone = f.rules.zoneTable.FindZone(kZones[zoneIndex]);
				const int bonus = ModuleResolver::FindZoneLevelBonus(effects, id);
				report.Check(bonus == zone->levelBonus, "zone-driven family gets the region's effective level", Json{ { "zone", zone->zone.name }, { "level", level }, { "bonus", bonus } });
				report.Check(ModuleResolver::GetPlacementFamily(f.zoneRules, *module, *f.loadout.FindPlacement(id)) == (zone->anyFamily ? GameplayTag{} : zone->family), "placement family follows its single-family zone");
				const int effective = level + bonus;
				const float refund = zoneIndex < 4 ? .25f + .05f * (effective - 1) : zoneIndex == 4 ? .10f + .02f * (effective - 1) : 0.f;
				report.CheckValue("family refund placeholder scales past L3", .25f + .05f * (effective - 1), DescriptionValue(module->sizeVariants.front(), effective, "cooldown.Module.Zone.Wings.seconds"), zone->zone.name);
				report.CheckValue("Hull refund placeholder scales past L3", .10f + .02f * (effective - 1), DescriptionValue(module->sizeVariants.front(), effective, "cooldown.Module.Zone.Hull.seconds"), zone->zone.name);
				const auto started = StartCooldowns(f);
				const auto enemy = Enemy(f);
				const auto group = Shot();
				Hit(*enemy, f.ship.get(), 1.f, group);
				Hit(*enemy, f.ship.get(), 1.f, group);
				const auto secondEnemy = Enemy(f, 150.f);
				Hit(*secondEnemy, f.ship.get(), 1.f, group);
				for (const auto& [ability, before] : started)
				{
					const auto& tags = ability->GetDefinition().abilityTags;
					const bool matching = zoneIndex == 4 || (zoneIndex < 4 && std::any_of(tags.begin(), tags.end(), [&](const auto& tag) { return tag.MatchesTag(zone->family); }));
					report.CheckValue("one successful shot refunds only its intended Q/E/F/R family once across pellets/bounces/targets", std::max(0.f, before - (matching ? refund : 0.f)), ability->GetCooldownRemaining(), zone->zone.name + ", " + ability->GetDefinition().abilityId);
				}
				List<float> after;
				for (const auto& [ability, before] : started) after.push_back(ability->GetCooldownRemaining());
				auto invalid = Shot(); invalid.hitGroup = std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::AbilityUse);
				Hit(*enemy, f.ship.get(), 1.f, invalid);
				invalid = Shot(); invalid.skipModuleDamageRules = true; Hit(*enemy, f.ship.get(), 1.f, invalid);
				invalid = Shot(); invalid.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed; Hit(*enemy, f.ship.get(), 1.f, invalid);
				Hit(*enemy, f.ship.get(), 0.f, Shot());
				for (std::size_t i = 0; i < started.size(); ++i) report.CheckValue("ability/generated/suppressed/zero hits do not refund", after[i], started[i].first->GetCooldownRemaining(), zone->zone.name);
				f.RemoveFromGrid(id);
				Hit(*enemy, f.ship.get(), 1.f, Shot());
				for (std::size_t i = 0; i < started.size(); ++i) report.CheckValue("uninstalled barrel stops refunding", after[i], started[i].first->GetCooldownRemaining(), zone->zone.name);
			}
	}

	void CriticalCoexistenceChecks(Report& report, Application& application)
	{
		EpicFixture f{ report, application };
		const auto barrel = f.InstallIn(kBarrel, 4);
		const auto sync = f.InstallIn("Module.Primary.CriticalSync.Uncommon", 5);
		const auto& effects = f.ship->GetModuleApplier().GetResolvedEffects();
		const int barrelLevel = 1 + ModuleResolver::FindZoneLevelBonus(effects, barrel);
		const int syncLevel = 1 + ModuleResolver::FindZoneLevelBonus(effects, sync);
		const float refund = .10f + .02f * (barrelLevel - 1);
		const float criticalRefund = 1.f + .2f * (syncLevel - 1);
		const auto started = StartCooldowns(f);
		const auto enemy = Enemy(f);
		for (int shot = 0; shot < 3; ++shot)
		{
			const auto payload = Shot(true);
			Hit(*enemy, f.ship.get(), 1.f, payload);
			Hit(*enemy, f.ship.get(), 1.f, payload);
		}
		float before = 0.f, after = 0.f;
		for (const auto& [ability, remaining] : started) { before += remaining; after += ability->GetCooldownRemaining(); }
		report.CheckValue("Critical Sync and Resonant Barrel count the same critical shot independently", before - 4.f * 3.f * refund - criticalRefund, after, Json{ { "barrelLevel", barrelLevel }, { "syncLevel", syncLevel }, { "shots", 3 } }.dump());
	}

	void FamilyCountChecks(Report& report, Application& application)
	{
		EpicFixture f{ report, application };
		const auto confluence = f.InstallIn("Module.Utility.ConfluenceCore.Epic", 4, 1, 4);
		const int effective = 1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), confluence);
		const float attack = f.Owner(OwnerAttributeIds::AttackPower);
		const float movement = f.Owner(OwnerAttributeIds::MoveSpeedHorizontal);
		const auto barrel = f.InstallIn(kBarrel, 0);
		report.CheckValue("Confluence counts the Barrel as Offense in Wings", attack + 2.f + (effective - 1), f.Owner(OwnerAttributeIds::AttackPower), "one zone-driven Offense module");
		report.Check(f.loadout.Move(f.zoneRules, barrel, { 0, f.firstRow + 2 }, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.zoneRules), "installed Barrel moves into Engine through real loadout");
		report.CheckValue("moving Barrel out of Wings removes its Offense count", attack, f.Owner(OwnerAttributeIds::AttackPower), "Barrel now Movement");
		report.CheckValue("Confluence counts the moved Barrel as Movement", movement + .25f + .1f * (effective - 1), f.Owner(OwnerAttributeIds::MoveSpeedHorizontal), "Engine placement");
		report.Check(f.loadout.Move(f.zoneRules, barrel, { 0, f.firstRow + 4 }, 0) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.zoneRules), "Barrel moves into all-family Hull");
		report.CheckValue("all-family Hull gives Barrel no single-family Confluence contribution", movement, f.Owner(OwnerAttributeIds::MoveSpeedHorizontal), "Hull has no unique family");
	}

	void Equip(EpicFixture& f, sas::AbilitySlot slot, const char* id)
	{
		auto& asc = f.ship->GetAbilitySystemComponent();
		asc.ClearAbilitySlot(slot);
		if (!id) return;
		const auto* source = content::AbilityContentCatalog::FindById(id);
		f.report.Check(source != nullptr, "loadout uses a shipped ability definition", id);
		auto definition = *source;
		definition.slot = slot;
		std::string failureReason;
		const auto handle = asc.GrantAbility(definition, &failureReason);
		f.report.Check(handle.IsValid(), "loadout grant succeeds", Json{ { "ability", id }, { "failureReason", failureReason } });
	}

	void BeamChecks(Report& report, Application& application)
	{
		EpicFixture f{ report, application };
		const auto barrel = f.InstallIn(kBarrel, 4);
		const int effective = 1 + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), barrel);
		const float refund = .10f + .02f * (effective - 1);
		const auto started = StartCooldowns(f);
		const auto enemy = Enemy(f);
		enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 0.f, -200.f });
		const auto other = Enemy(f);
		other->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 0.f, -350.f });
		for (const auto& target : { enemy, other })
		{
			target->SetEnablePhysics(true);
			target->SetCollisionMask(vega::GameCollision::PlayerBullet);
		}
		f.level->TickInternal(0.f);
		const auto* weapon = content::WeaponContentCatalog::FindById("Weapon.Beam.ContinuousHeatLaser.Basic");
		report.Check(weapon != nullptr, "beam cadence uses a shipped continuous beam weapon");
		auto& asc = f.ship->GetAbilitySystemComponent();
		asc.ClearAbilitySlot(sas::AbilitySlot::PrimaryFire);
		report.Check(asc.GrantAbility(AbilityData::MakePrimaryFireAbilityDefinition(*weapon), sas::AbilitySlot::PrimaryFire).IsValid(), "beam grants real Primary ability");
		asc.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		asc.Tick(0.f);
		report.Check(asc.GetAbility(sas::AbilitySlot::PrimaryFire)->IsActive(), "beam is active before cadence measurement begins");
		float elapsed = 0.f;
		for (int frame = 1; frame <= 50; ++frame)
		{
			asc.Tick(.01f); elapsed += .01f;
			const int hits = frame / 25;
			for (const auto& [ability, before] : started)
				report.CheckValue("beam refunds at most once per 0.25 sec across every target", before - elapsed - refund * hits, ability->GetCooldownRemaining(), Json{ { "frame", frame }, { "frontHealth", enemy->GetHealthComponent().GetHealth() }, { "rearHealth", other->GetHealthComponent().GetHealth() } }.dump());
			if (frame == 25)
			{
				report.Check(enemy->GetHealthComponent().GetHealth() < 1000.f && other->GetHealthComponent().GetHealth() == 1000.f, "non-piercing beam hits the front target first");
				enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 250.f, -200.f });
				report.Check(asc.GetAbility(sas::AbilitySlot::PrimaryFire)->IsActive(), "beam stays active while its front target moves aside");
			}
		}
		asc.Tick(.01f); elapsed += .01f;
		asc.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
		asc.Tick(0.f);
		for (const auto& [ability, before] : started)
			report.CheckValue("beam release flush shares the last delivery group without an extra refund", before - elapsed - 2.f * refund, ability->GetCooldownRemaining(), "release at 0.51 sec after two regular deliveries");
		report.Check(enemy->GetHealthComponent().GetHealth() < 1000.f && other->GetHealthComponent().GetHealth() < 1000.f, "real continuous beam damaged both targets");
	}

	void ClearLoadout(EpicFixture& f) { for (const auto slot : kSlots) Equip(f, slot, nullptr); }
	float TypedHit(EpicFixture& f, SpaceShip& enemy, const List<GameplayTag>& tags, DamageDeliveryType delivery = DamageDeliveryType::Direct)
	{
		return Hit(enemy, f.ship.get(), 10.f, Plain(), {}, {}, delivery, tags);
	}

	void MonochromeChecks(Report& report, Application& application)
	{
		const char* energy[] = { "Ability.Movement.EnergySpear.Basic", "Ability.Offense.MineLayer.Basic", "Ability.Offense.RailBurst.Basic", "Ability.Offense.AstralSurge.Basic" };
		for (int level = 1; level <= 3; ++level)
			for (const int zoneIndex : { 0, 4, 5 })
			{
				EpicFixture f{ report, application };
				ClearLoadout(f);
				const auto id = f.InstallIn(kCore, zoneIndex, level);
				const auto enemy = Enemy(f);
				const int effective = level + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
				const float pair = 1.25f + .05f * (effective - 1);
				report.CheckValue("Monochrome pair description uses effective level including L4/L5", pair, DescriptionValue(f.rules.lookup(kCore)->sizeVariants.front(), effective, "monochrome.0.pairMultiplier"), std::to_string(effective));
				for (int count = 0; count <= 4; ++count)
				{
					if (count > 0) Equip(f, kSlots[count - 1], energy[count - 1]);
					const float multiplier = count < 2 ? 1.f : pair + .25f * (count - 2);
					report.CheckValue("0/1/2/3/4 matching equipped abilities recalculate damage immediately", 10.f * multiplier, TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "count=" + std::to_string(count) + ", effective=" + std::to_string(effective));
					report.CheckValue("other damage types stay unchanged", 10.f, TypedHit(f, *enemy, { DamageTypeSchema::Photonic }), "Energy loadout, Photonic hit");
				}
				Equip(f, kSlots[3], nullptr);
				report.CheckValue("removing one matching ability immediately removes its increment", 10.f * (pair + .25f), TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "4 to 3");
				Equip(f, kSlots[2], "Ability.Movement.Dash.Basic");
				report.CheckValue("untyped ability never counts", 10.f * pair, TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "2 typed plus untyped Dash");
				f.RemoveFromGrid(id);
				report.CheckValue("uninstall removes typed bonus", 10.f, TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "core removed");
			}
		EpicFixture f{ report, application };
		ClearLoadout(f);
		f.InstallIn(kCore, 5);
		Equip(f, kSlots[0], energy[0]); Equip(f, kSlots[1], energy[1]);
		Equip(f, kSlots[2], "Ability.Offense.HullShock.Basic"); Equip(f, kSlots[3], "Ability.Offense.IonStorm.Basic");
		const auto enemy = Enemy(f);
		report.CheckValue("two Energy abilities receive their own pair bonus", 12.5f, TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "Energy2 Electric2");
		report.CheckValue("second independent Electric pair also receives bonus", 12.5f, TypedHit(f, *enemy, { DamageTypeSchema::Electric }), "Energy2 Electric2");
		Equip(f, kSlots[0], nullptr);
		auto duplicate = *content::AbilityContentCatalog::FindById(energy[0]);
		duplicate.slot = kSlots[0];
		duplicate.damageTags = { DamageTypeSchema::Energy, DamageTypeSchema::Electric, DamageTypeSchema::Electric };
		auto& asc = f.ship->GetAbilitySystemComponent();
		std::string failureReason;
		const auto invalid = asc.GrantAbility(duplicate, &failureReason);
		report.Check(!invalid.IsValid() && !asc.GetAbility(kSlots[0]) && failureReason.find("requires Energy damage") != std::string::npos, "shipped Energy Spear retains its single-type family contract", failureReason);
		GameAbilityDefinition multiType;
		multiType.abilityId = "Ability.Offense.MonochromeProbe.Basic";
		multiType.slot = kSlots[0];
		multiType.abilityTags = { GameplayTags::Ability::Offense };
		multiType.damageTags = { DamageTypeSchema::Energy, DamageTypeSchema::Electric, DamageTypeSchema::Electric };
		failureReason.clear();
		const auto multi = asc.GrantAbility(multiType, &failureReason);
		report.Check(multi.IsValid(), "configured multi-type fixture grants through canonical validation", failureReason);
		report.CheckValue("multi-type ability counts once for each distinct type", 15.f, TypedHit(f, *enemy, { DamageTypeSchema::Electric }), "one multi-type plus two Electric; duplicate tag ignored");
		report.CheckValue("multi-type ability preserves the other pair", 12.5f, TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "Energy2 Electric3");
		report.CheckValue("one multi-tag hit takes strongest bonus once", 15.f, TypedHit(f, *enemy, { DamageTypeSchema::Energy, DamageTypeSchema::Electric }), "Energy2 Electric3");
		report.CheckValue("periodic damage receives the matching type bonus", 12.5f, TypedHit(f, *enemy, { DamageTypeSchema::Energy }, DamageDeliveryType::Periodic), "Energy periodic hit");
		auto primary = Shot();
		report.CheckValue("Primary damage also receives the matching type bonus", 12.5f, Hit(*enemy, f.ship.get(), 10.f, primary, {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Projectile, { DamageTypeSchema::Energy }), "type bonus is outgoing damage, not ability-only");
		for (const auto slot : kSlots) Equip(f, slot, nullptr);
		report.CheckValue("clearing every loadout ability removes all type bonuses", 10.f, TypedHit(f, *enemy, { DamageTypeSchema::Energy }), "empty Q/E/F/R");
		Equip(f, kSlots[0], "Ability.Offense.WingSentinels.Basic"); Equip(f, kSlots[1], "Ability.Offense.CombatSentry.Basic");
		report.CheckValue("Photonic is counted from definition tags without a restricted type list", 12.5f, TypedHit(f, *enemy, { DamageTypeSchema::Photonic }), "two Photonic abilities");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context;
	vega::EngineContext::Scope scope{ context };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Epic new modules content registration failed" };
		std::string title = "Epic New Modules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		CatalogAndLoaderChecks(report); BarrelZoneChecks(report, application); FamilyCountChecks(report, application); CriticalCoexistenceChecks(report, application); BeamChecks(report, application); MonochromeChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
		report.artifact["test"] = "EpicNewModules20261025E2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
