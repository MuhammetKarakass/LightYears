#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
	using namespace ly;
	using namespace common_module_e2e;

	// Breacher has five bay cells, so a three-cell module can sit beside three ability shells (the Fighter tops out at two).
	// Capacity of a credited shell is its shipped maxCharges (1) plus the level bonus (L1 +1, L2 +2, L3 +3).
	constexpr const char* kShipId = "Ship.Player.Breacher.Basic";
	constexpr const char* kSpareId = "Module.Utility.RareSpareFixture.Rare";
	constexpr const char* kOverflowId = "Module.SideFeature.RareSpareFixture.Overflow";
	constexpr const char* kTrickleId = "Module.SideFeature.RareSpareFixture.Trickle";
	constexpr const char* kRealSpareId = "Module.Utility.SpareCapacitor.Rare";
	constexpr const char* kRealTrickleId = "Module.SideFeature.SpareCapacitor.TrickleCharge";
	constexpr const char* kModulesFile = "rare-spare-capacitor-modules.json";
	constexpr const char* kSideFeaturesFile = "rare-spare-capacitor-side-features.json";
	constexpr float kTrickleSeconds = 20.f;

	// Test-only definitions parsed from fixture JSON. They never enter shipped content.
	ly::List<ly::ModuleDefinition> gFixtureDefinitions;

	const ModuleDefinition* FixtureLookup(const sas::ContentId& id)
	{
		for (const ModuleDefinition& definition : gFixtureDefinitions)
			if (definition.moduleId == id) return &definition;
		return content::ModuleContentCatalog::FindById(id);
	}

	std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
	{
		std::filesystem::create_directories(root);
		const std::filesystem::path path = root / name;
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		output << value.dump(2) << '\n';
		return path;
	}

	Json OneCell()
	{
		return Json::array({ Json::array({ 0, 0 }) });
	}

	Json CapacityRules()
	{
		return Json::array({ Json{ { "chargePerLevel", Json::array({ 1, 2, 3 }) } } });
	}

	Json ChargeVariant(const Json& shape, const Json& chargeRules)
	{
		return Json{ { "shape", shape }, { "chargeRules", chargeRules } };
	}

	Json SpareVariants()
	{
		return Json::array({
			ChargeVariant(OneCell(), CapacityRules()),
			ChargeVariant(Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }) }), CapacityRules()),
			ChargeVariant(Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }), Json::array({ 2, 0 }) }), CapacityRules()) });
	}

	// Spare Capacitor fixture: one, two and three cells (ladder 1-2-3). Side features apply on the largest size only.
	Json SpareModule(const Json& variants)
	{
		return Json{
			{ "id", kSpareId }, { "displayName", "Spare capacitor fixture" }, { "description", "Spare capacitor fixture." }, { "rarity", "Rare" },
			{ "levelCount", 3 }, { "family", "Ability.Utility" }, { "sizeLadder", Json::array({ 1, 2, 3 }) },
			{ "sizeVariants", variants }, { "sideFeatures", Json::array({ kOverflowId, kTrickleId }) }
		};
	}

	// Overflow fixture: two charges per interval, so a grant that does not fit has room to spill to the next shell.
	Json OverflowFeature()
	{
		return Json{ { "id", kOverflowId }, { "displayName", "Spare overflow fixture" }, { "description", "Spare overflow fixture." }, { "levelCount", 3 },
			{ "rules", Json{ { "chargeRules", Json::array({ Json{ { "overflowTransfer", true }, { "trickleSeconds", kTrickleSeconds }, { "trickleCharges", 2 } } }) } } } };
	}

	// Trickle fixture: one charge per interval to the adjacent shell with the fewest charges.
	Json TrickleFeature()
	{
		return Json{ { "id", kTrickleId }, { "displayName", "Spare trickle fixture" }, { "description", "Spare trickle fixture." }, { "levelCount", 3 },
			{ "rules", Json{ { "chargeRules", Json::array({ Json{ { "trickleSeconds", kTrickleSeconds }, { "trickleCharges", 1 } } }) } } } };
	}

	Json ModuleFixtureSet(const Json& shipped)
	{
		Json modules = shipped;
		modules["modules"].push_back(SpareModule(SpareVariants()));
		return modules;
	}

	Json SideFeatureFixtureSet(const Json& shipped)
	{
		Json features = shipped;
		features["sideFeatures"].push_back(OverflowFeature());
		features["sideFeatures"].push_back(TrickleFeature());
		return features;
	}

	bool RunScenario(Json& scenarios, const char* name, const std::function<void()>& body)
	{
		Json entry{ { "name", name }, { "passed", false } };
		try
		{
			body();
			entry["passed"] = true;
		}
		catch (const std::exception& exception)
		{
			entry["error"] = exception.what();
			std::cerr << name << ": " << exception.what() << '\n';
		}
		scenarios.push_back(entry);
		return entry["passed"].get<bool>();
	}

	struct Fixture
	{
		Report& report;
		shared_ptr<HeadlessModuleLevel> level;
		shared_ptr<PlayerSpaceShip> ship;
		ModuleRulesContext rules;
		ModuleLoadout loadout;
		Fixture(Report& result, Application& application) : report{ result }, level{ std::make_shared<HeadlessModuleLevel>(&application) }, rules{ *content::ShipContentCatalog::GetPlayerShipDefinition(kShipId).schematic, [](const sas::ContentId& id) { return FixtureLookup(id); }, content::ModuleContentCatalog::GetZoneTable(), [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } }
		{
			level->BeginPlayInternal();
			const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition(kShipId);
			ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
			report.Check(ship != nullptr, "spare test spawns a real Breacher ship");
			level->TickInternal(0.f);
			ship->SetInvulnerability(false);
			ship->SetUseScreenClamp(false);
			ship->SetActorLocation({ 10000.f, 10000.f });
			report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "spare test uses canonical default modules and the runtime applier");
		}
	};

	const sas::AbilitySlot kAllSlots[4] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };

	GameAbility* AbilityIn(Fixture& f, sas::AbilitySlot slot)
	{
		return f.ship->GetAbilitySystemComponent().GetAbility(slot);
	}

	int CapacityOf(Fixture& f, sas::AbilitySlot slot)
	{
		GameAbility* ability = AbilityIn(f, slot);
		return ability ? ability->GetChargeCapacity() : -1;
	}

	int ChargesOf(Fixture& f, sas::AbilitySlot slot)
	{
		GameAbility* ability = AbilityIn(f, slot);
		return ability ? ability->GetCharges() : -1;
	}

	// One real use through TryActivate; ends it immediately, as the existing charge tests do.
	bool Spend(GameAbility* ability)
	{
		const bool activated = ability->TryActivate();
		ability->Cancel(sas::AbilityEndReason::Completed);
		return activated;
	}

	// Advances exactly the shipped anti-spam gap between two charge uses.
	void WaitUseGap(GameAbility* ability)
	{
		ability->Tick(ability->GetDefinition().chargeUseInterval);
	}

	bool Near(float actual, float expected, float tolerance = 1e-3f)
	{
		return std::abs(actual - expected) <= tolerance;
	}

	bool Credited(const List<sas::AbilitySlot>& slots, sas::AbilitySlot slot)
	{
		return std::find(slots.begin(), slots.end(), slot) != slots.end();
	}

	// Installs a Spare Capacitor of the given size and level, beside exactly `size` ability shells when such a spot exists.
	// `side` may name a side feature (only the largest size applies one). Returns the instance id.
	std::uint32_t InstallSpare(Fixture& f, int size, int level, const char* side = "", const char* moduleId = kSpareId)
	{
		const auto* definition = f.rules.lookup(moduleId);
		if (!definition) throw std::runtime_error{ "spare module is missing" };
		const std::uint32_t instance = f.loadout.AddToInventory(f.rules, *definition, size - 1, definition->rarity, level, sas::ContentId{ side });
		if (instance == 0) throw std::runtime_error{ "spare fixture inventory rejected the module" };
		const ModulePlacement item = *f.loadout.FindInventoryItem(instance);
		for (const bool exact : { true, false })
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < f.rules.schematic.height; ++y)
					for (int x = 0; x < f.rules.schematic.width; ++x)
					{
						ModulePlacement candidate = item;
						candidate.origin = sf::Vector2i{ x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
						const List<GameplayTag> adjacent = ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), candidate, 3);
						if (exact ? adjacent.size() != static_cast<std::size_t>(size) : adjacent.size() < static_cast<std::size_t>(size)) continue;
						if (f.loadout.Install(f.rules, instance, candidate.origin, rotation) != ModulePlacementFailure::None) continue;
						if (!f.ship->GetModuleApplier().Apply(f.loadout, f.rules))
						{
							(void)f.loadout.Uninstall(instance);
							continue;
						}
						return instance;
					}
		throw std::runtime_error{ "no legal spare placement beside " + std::to_string(size) + " ability shell(s)" };
	}

	// Ability slots the placement credits (the same adjacency the resolver reads, cut to three).
	List<sas::AbilitySlot> CreditedSlots(Fixture& f, std::uint32_t instance)
	{
		List<sas::AbilitySlot> slots;
		const ModulePlacement* placement = f.loadout.FindPlacement(instance);
		if (!placement) return slots;
		for (const GameplayTag& system : ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), *placement, 3))
		{
			slots.push_back(AbilitySlotForModuleSystem(system));
		}
		return slots;
	}

	// NanoPlague (E) fires only at an enemy under the cursor, and a headless level has none, so its shell cannot spend here.
	// That shell still holds the bonus (checked by the capacity scenarios); the spend scenarios use the other credited shells.
	bool NeedsEnemyTarget(Fixture& f, sas::AbilitySlot slot)
	{
		GameAbility* ability = AbilityIn(f, slot);
		if (!ability) return false;
		const std::string abilityId = ability->GetDefinition().abilityId;
		return abilityId == AbilityData::NanoPlague::AbilityId::Basic;
	}

	List<sas::AbilitySlot> SpendableCredited(Fixture& f, const List<sas::AbilitySlot>& credited)
	{
		List<sas::AbilitySlot> spendable;
		for (const sas::AbilitySlot slot : credited)
			if (!NeedsEnemyTarget(f, slot)) spendable.push_back(slot);
		return spendable;
	}

	// The index-th credited shell that activates without a target.
	sas::AbilitySlot SpendableSlot(Fixture& f, const List<sas::AbilitySlot>& credited, std::size_t index)
	{
		const List<sas::AbilitySlot> spendable = SpendableCredited(f, credited);
		if (index >= spendable.size()) throw std::runtime_error{ "placement has fewer headless-spendable shells than the scenario needs" };
		return spendable[index];
	}

	GameAbility* EnemyTargetShell(Fixture& f, const List<sas::AbilitySlot>& credited)
	{
		for (const sas::AbilitySlot slot : credited)
			if (NeedsEnemyTarget(f, slot)) return AbilityIn(f, slot);
		return nullptr;
	}

	// Credited shells hold capacity 1 + level; every other shell keeps the shipped capacity 1.
	void CheckCapacities(Report& report, Fixture& f, const List<sas::AbilitySlot>& credited, int level, const std::string& label)
	{
		for (const sas::AbilitySlot slot : kAllSlots)
		{
			const int expected = Credited(credited, slot) ? 1 + level : 1;
			report.Check(CapacityOf(f, slot) == expected, label + " shell " + std::to_string(static_cast<int>(slot)) + " capacity is " + std::to_string(expected),
				Json{ { "capacity", CapacityOf(f, slot) }, { "expected", expected } });
		}
	}

	// Loader checks: the accepted forms are read back, and each misuse is refused by the real loaders.
	void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shippedModules, const Json& shippedSideFeatures, const std::filesystem::path& sideFeaturesPath)
	{
		const ModuleDefinition* spare = nullptr;
		for (const ModuleDefinition& definition : gFixtureDefinitions)
			if (definition.moduleId == kSpareId) spare = &definition;
		report.Check(spare != nullptr && spare->sizeVariants.size() == 3, "loader keeps all three spare sizes");
		report.Check(spare != nullptr && spare->sizeVariants[2].chargeRules.size() == 1 && spare->sizeVariants[2].chargeRules[0].chargePerLevel.values.size() == 3 &&
			spare->sizeVariants[2].chargeRules[0].chargePerLevel.values[0] == 1 && spare->sizeVariants[2].chargeRules[0].chargePerLevel.values[2] == 3,
			"loader reads the per-level capacity table 1/2/3");

		const auto side = content::ModuleSideFeatureLoader::LoadFromFile(sideFeaturesPath);
		report.Check(side.Succeeded(), "spare side-feature fixtures load through the real side-feature loader", side.error);
		bool overflowRead = false;
		bool trickleRead = false;
		for (const ModuleSideFeatureDefinition& feature : side.definitions)
		{
			if (feature.sideFeatureId == kOverflowId) overflowRead = feature.rules.chargeRules.size() == 1 && feature.rules.chargeRules[0].overflowTransfer && feature.rules.chargeRules[0].trickleCharges == 2;
			if (feature.sideFeatureId == kTrickleId) trickleRead = feature.rules.chargeRules.size() == 1 && !feature.rules.chargeRules[0].overflowTransfer && feature.rules.chargeRules[0].trickleCharges == 1;
		}
		report.Check(overflowRead && trickleRead, "side loader reads overflowTransfer and trickle fields");

		const auto rejectModule = [&](const char* name, const Json& variants)
		{
			Json modules = shippedModules;
			modules["modules"].push_back(SpareModule(variants));
			const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", modules);
			const auto loaded = content::ModuleLoader::LoadFromFile(path);
			report.Check(!loaded.Succeeded(), std::string{ "module loader rejects " } + name, loaded.error);
		};
		const auto withFirst = [](const Json& first)
		{
			const Json base = SpareVariants();
			return Json::array({ first, base[1], base[2] });
		};
		rejectModule("main-trickle", withFirst(ChargeVariant(OneCell(), Json::array({ Json{ { "trickleSeconds", kTrickleSeconds }, { "trickleCharges", 1 } } }))));
		rejectModule("main-mixed", withFirst(ChargeVariant(OneCell(), Json::array({ Json{ { "chargePerLevel", Json::array({ 1, 2, 3 }) }, { "overflowTransfer", true } } }))));
		rejectModule("four-cells", withFirst(ChargeVariant(Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }), Json::array({ 2, 0 }), Json::array({ 3, 0 }) }), CapacityRules())));
		rejectModule("short-table", withFirst(ChargeVariant(OneCell(), Json::array({ Json{ { "chargePerLevel", Json::array({ 1, 2 }) } } }))));
		rejectModule("scalar-capacity", withFirst(ChargeVariant(OneCell(), Json::array({ Json{ { "chargePerLevel", 2 } } }))));
		rejectModule("unknown-field", withFirst(ChargeVariant(OneCell(), Json::array({ Json{ { "chargePerLevel", Json::array({ 1, 2, 3 }) }, { "bogus", true } } }))));
		rejectModule("empty-entry", withFirst(ChargeVariant(OneCell(), Json::array({ Json::object() }))));

		const auto sideFeature = [](const char* id, const Json& entries)
		{
			return Json{ { "id", id }, { "displayName", "Spare reject fixture" }, { "description", "Spare reject fixture." }, { "levelCount", 3 },
				{ "rules", Json{ { "chargeRules", entries } } } };
		};
		const auto rejectSide = [&](const char* name, const Json& entries)
		{
			Json features = shippedSideFeatures;
			features["sideFeatures"].push_back(sideFeature("Module.SideFeature.RareSpareFixture.Reject", entries));
			const auto path = WriteJson(root, std::string{ "reject-side-" } + name + ".json", features);
			const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
			report.Check(!loaded.Succeeded(), std::string{ "side loader rejects " } + name, loaded.error);
		};
		rejectSide("capacity-table", Json::array({ Json{ { "chargePerLevel", Json::array({ 1, 2, 3 }) } } }));
		rejectSide("trickle-without-charges", Json::array({ Json{ { "trickleSeconds", kTrickleSeconds } } }));
		rejectSide("no-effect", Json::array({ Json{ { "overflowTransfer", false } } }));
		rejectSide("zero-interval", Json::array({ Json{ { "trickleSeconds", 0 }, { "trickleCharges", 1 } } }));
		rejectSide("bool-not-boolean", Json::array({ Json{ { "overflowTransfer", 1 }, { "trickleSeconds", kTrickleSeconds }, { "trickleCharges", 1 } } }));
	}

	// Scenario 1: 1, 2 and 3 cells at levels 1-3. Only the credited shells gain capacity; the rest keep capacity 1.
	void CapacityChecks(Report& report, Application& application)
	{
		for (int size = 1; size <= 3; ++size)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				const std::string label = "size " + std::to_string(size) + " level " + std::to_string(level);
				const std::uint32_t instance = InstallSpare(f, size, level);
				const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
				report.Check(static_cast<int>(credited.size()) == size, label + " credits exactly size-many shells", Json{ { "credited", static_cast<int>(credited.size()) } });
				CheckCapacities(report, f, credited, level, label);
				for (const sas::AbilitySlot slot : credited)
				{
					GameAbility* ability = AbilityIn(f, slot);
					report.Check(ability != nullptr && ability->BuildSnapshot().chargeCapacity == 1 + level, label + " runtime snapshot carries the boosted capacity");
				}
			}
		}
	}

	// Scenario 2: a boosted shell is used back to back through the real ASC; the anti-spam gap gates uses, not the timer.
	void ConsecutiveUseChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::uint32_t instance = InstallSpare(f, 3, 2);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		report.Check(credited.size() == 3, "three-cell spare credits three shells beside the Breacher bay");
		report.Check(SpendableCredited(f, credited).size() == 2 && EnemyTargetShell(f, credited) != nullptr,
			"two credited shells spend headless; the NanoPlague shell needs an enemy target and is skipped");
		GameAbility* ability = AbilityIn(f, SpendableSlot(f, credited, 0));
		report.Check(ability != nullptr && ability->GetDefinition().maxCharges == 1, "credited shell ships with maxCharges 1 (the boost is a runtime bonus)");
		report.Check(ability->GetDefinition().cooldown > 5.f, "shipped cooldown is long enough for the staged checks", Json{ { "cooldown", ability->GetDefinition().cooldown } });
		report.Check(ability->GetChargeCapacity() == 3 && ability->GetCharges() == 3, "level 2 boosts capacity to 1 + 2 and arrives full", Json{ { "charges", ability->GetCharges() } });
		report.Check(Spend(ability), "use 1 of 3");
		report.Check(!ability->TryActivate(), "immediate second use is blocked by the anti-spam gap");
		WaitUseGap(ability);
		report.Check(Spend(ability), "use 2 of 3 after the gap, with the recharge timer still running");
		WaitUseGap(ability);
		report.Check(Spend(ability), "use 3 of 3 after the gap");
		report.Check(ability->GetCharges() == 0 && ability->IsOnCooldown(), "pool is empty with the recharge timer running");
		WaitUseGap(ability);
		report.Check(!ability->TryActivate(), "an empty pool blocks use after the gap");

		// The shell beside no spare keeps capacity 1 and one charge, and spends it once.
		for (const sas::AbilitySlot slot : kAllSlots)
		{
			if (Credited(credited, slot)) continue;
			GameAbility* other = AbilityIn(f, slot);
			report.Check(other != nullptr && other->GetChargeCapacity() == 1 && other->GetCharges() == 1, "non-adjacent shell keeps capacity 1 and one charge");
			report.Check(Spend(other) && other->GetCharges() == 0, "non-adjacent shell spends its single charge");
		}
	}

	// Scenario 3: removing the module restores the old capacity; ten attach/detach rounds keep the handle and the lease count stable.
	void DetachRoundChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::size_t baseline = f.ship->GetModuleApplier().GetOwnedHandleCount();
		const std::uint32_t instance = InstallSpare(f, 2, 3);
		const ModulePlacement placement = *f.loadout.FindPlacement(instance);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		report.Check(credited.size() == 2, "two-cell spare credits two shells");
		const std::size_t installed = f.ship->GetModuleApplier().GetOwnedHandleCount();
		GameAbility* boosted = AbilityIn(f, credited[0]);
		const sas::AbilityHandle handle = boosted->GetHandle();
		report.Check(boosted->GetChargeCapacity() == 4, "level 3 boosts the credited shell to capacity 4");

		report.Check(f.loadout.Uninstall(instance) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "uninstall reapplies the loadout");
		report.Check(CapacityOf(f, credited[0]) == 1 && CapacityOf(f, credited[1]) == 1 && ChargesOf(f, credited[0]) <= 1, "removing the spare restores capacity 1 and clips surplus charges",
			Json{ { "capacity", CapacityOf(f, credited[0]) }, { "charges", ChargesOf(f, credited[0]) } });
		report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, "removing the spare returns the lease count to baseline");

		for (int round = 1; round <= 10; ++round)
		{
			const std::string label = "round " + std::to_string(round);
			report.Check(f.loadout.Install(f.rules, instance, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules),
				label + " reinstalls the spare at its origin");
			report.Check(CapacityOf(f, credited[0]) == 4 && CapacityOf(f, credited[1]) == 4, label + " restores the boosted capacity");
			report.Check(AbilityIn(f, credited[0]) == boosted && boosted->GetHandle() == handle, label + " keeps the same ability object and handle");
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == installed, label + " keeps the same lease count while installed");
			report.Check(f.loadout.Uninstall(instance) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), label + " uninstalls again");
			report.Check(CapacityOf(f, credited[0]) == 1 && CapacityOf(f, credited[1]) == 1 && ChargesOf(f, credited[0]) <= 1, label + " leaves no stale capacity",
				Json{ { "capacity", CapacityOf(f, credited[0]) }, { "charges", ChargesOf(f, credited[0]) } });
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, label + " returns the lease count to baseline");
		}
		report.Check(AbilityIn(f, credited[0]) == boosted && boosted->GetHandle() == handle, "handle stays identical after ten rounds");
	}

	// Scenario 4: the grant path through the real ASC fills missing charges only and never raises capacity.
	void GrantChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::uint32_t instance = InstallSpare(f, 3, 2);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		auto& abilities = f.ship->GetAbilitySystemComponent();
		const sas::AbilitySlot spendSlot = SpendableSlot(f, credited, 0);
		GameAbility* ability = AbilityIn(f, spendSlot);
		const std::string abilityId = ability->GetDefinition().abilityId;

		report.Check(abilities.GrantAbilityCharge(spendSlot, 2) == 0 && ability->GetCharges() == 3 && ability->GetChargeCapacity() == 3,
			"a grant to a full pool adds nothing and never raises capacity");
		report.Check(Spend(ability), "grant check spends one charge");
		report.Check(ability->IsOnCooldown() && ability->GetCharges() == 2, "one spend leaves two charges and the recharge timer running");
		report.Check(abilities.GrantAbilityCharge(spendSlot, 9) == 1 && ability->GetCharges() == 3 && ability->GetChargeCapacity() == 3,
			"a grant of 9 fills only the one missing charge", Json{ { "charges", ability->GetCharges() } });
		report.Check(!ability->IsOnCooldown(), "a grant that fills the pool clears the recharge timer");

		// The anti-spam gap after the first spend still runs; the grant does not shorten it.
		WaitUseGap(ability);
		report.Check(Spend(ability), "spend after the grant");
		WaitUseGap(ability);
		report.Check(Spend(ability), "second spend after the gap");
		const float running = ability->GetCooldownRemaining();
		report.Check(ability->IsOnCooldown() && ability->GetCharges() == 1, "two spends leave one charge and a running timer");
		report.Check(abilities.GrantAbilityCharge(abilityId, 1) == 1 && ability->GetCharges() == 2 && Near(ability->GetCooldownRemaining(), running),
			"an id-addressed grant adds a charge and keeps the running timer's time", Json{ { "cooldownRemaining", ability->GetCooldownRemaining() }, { "before", running } });

		// A single-charge shell without a bonus: a grant returns one charge and clears the timer at once.
		GameAbility* plain = nullptr;
		for (const sas::AbilitySlot slot : kAllSlots)
		{
			if (!Credited(credited, slot)) plain = AbilityIn(f, slot);
			if (plain) break;
		}
		report.Check(plain != nullptr && plain->GetChargeCapacity() == 1, "a non-adjacent shell ships with capacity 1");
		report.Check(abilities.GrantAbilityCharge(plain->GetDefinition().abilityId, 1) == 0 && plain->GetCharges() == 1, "a grant to a full single-charge shell changes nothing");
		report.Check(Spend(plain) && plain->GetCharges() == 0 && plain->IsOnCooldown(), "single-charge shell spends its charge and starts the timer");
		report.Check(abilities.GrantAbilityCharge(plain->GetDefinition().abilityId, 1) == 1 && plain->GetCharges() == 1 && !plain->IsOnCooldown(),
			"a grant returns the single charge and clears the timer");
		report.Check(plain->TryActivate(), "single-charge shell activates after a grant");
		plain->Cancel(sas::AbilityEndReason::Completed);
		report.Check(abilities.GrantAbilityCharge(std::string{ "Ability.Missing.Id" }, 1) == 0, "a grant to an unknown ability id returns 0");
	}

	// Scenario 5: without a side feature a three-cell spare grants nothing over time; the capacity bonus alone stays.
	void NoSideFeatureChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::uint32_t instance = InstallSpare(f, 3, 2);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		GameAbility* ability = AbilityIn(f, SpendableSlot(f, credited, 0));
		report.Check(Spend(ability), "no-side check spends one charge");
		const int charges = ability->GetCharges();
		f.ship->GetModuleApplier().Tick(3.f * kTrickleSeconds);
		report.Check(ability->GetCharges() == charges && ability->GetChargeCapacity() == 3, "without a side feature nothing is granted over time", Json{ { "charges", ability->GetCharges() } });
	}

	// Scenario 6: Trickle Charge on the largest size. Each interval one charge goes to the adjacent shell with the fewest charges.
	void TrickleChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		auto& applier = f.ship->GetModuleApplier();
		const std::size_t baseline = applier.GetOwnedHandleCount();
		const std::uint32_t instance = InstallSpare(f, 3, 2, kTrickleId);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		GameAbility* first = AbilityIn(f, SpendableSlot(f, credited, 0));
		GameAbility* second = AbilityIn(f, SpendableSlot(f, credited, 1));
		GameAbility* third = EnemyTargetShell(f, credited);
		report.Check(third != nullptr, "trickle neighbourhood holds the NanoPlague shell, which stays full");

		report.Check(Spend(first), "trickle check: first spend");
		WaitUseGap(first);
		report.Check(Spend(first), "trickle check: two spends on the first shell");
		report.Check(Spend(second), "trickle check: one spend on the second shell");
		report.Check(first->GetCharges() == 1 && second->GetCharges() == 2 && third->GetCharges() == 3, "shells hold 1 / 2 / 3 charges before the trickle",
			Json{ { "first", first->GetCharges() }, { "second", second->GetCharges() }, { "third", third->GetCharges() } });

		applier.Tick(kTrickleSeconds - 1.f);
		report.Check(first->GetCharges() == 1, "no grant before the interval completes");
		applier.Tick(1.f);
		report.Check(first->GetCharges() == 2 && second->GetCharges() == 2 && third->GetCharges() == 3, "one interval grants one charge to the shell with the fewest",
			Json{ { "first", first->GetCharges() }, { "second", second->GetCharges() }, { "third", third->GetCharges() } });

		applier.Tick(3.f * kTrickleSeconds);
		report.Check(first->GetCharges() == 3 && second->GetCharges() == 3 && third->GetCharges() == 3, "the trickle fills the neighbourhood and never exceeds capacity",
			Json{ { "first", first->GetCharges() }, { "second", second->GetCharges() }, { "third", third->GetCharges() } });

		WaitUseGap(second);
		report.Check(Spend(second), "trickle check: a spend drops the second shell to two charges");
		WaitUseGap(second);
		report.Check(Spend(second) && second->GetCharges() == 1, "trickle check: a second spend leaves the second shell at one charge");
		applier.Tick(kTrickleSeconds);
		report.Check(second->GetCharges() == 2 && first->GetCharges() == 3 && third->GetCharges() == 3, "the lowest shell takes the trickle charge",
			Json{ { "second", second->GetCharges() }, { "first", first->GetCharges() } });

		report.Check(f.loadout.Uninstall(instance) && applier.Apply(f.loadout, f.rules), "uninstalling the spare reapplies the loadout");
		const int frozen = second->GetCharges();
		applier.Tick(3.f * kTrickleSeconds);
		report.Check(second->GetCharges() == frozen, "a removed spare grants nothing over time", Json{ { "charges", second->GetCharges() } });
		report.Check(CapacityOf(f, credited[0]) == 1 && CapacityOf(f, credited[1]) == 1 && CapacityOf(f, credited[2]) == 1, "removing the spare restores capacity 1");
		report.Check(applier.GetOwnedHandleCount() == baseline, "removing the spare releases its lease count");
	}

	// Scenario 7: Overflow Transfer. A grant that finds the chosen shell full spills to the next adjacent shell with room.
	void OverflowChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::uint32_t instance = InstallSpare(f, 3, 1, kOverflowId);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		auto& applier = f.ship->GetModuleApplier();
		GameAbility* first = AbilityIn(f, SpendableSlot(f, credited, 0));
		GameAbility* second = AbilityIn(f, SpendableSlot(f, credited, 1));
		GameAbility* third = EnemyTargetShell(f, credited);
		report.Check(third != nullptr, "overflow neighbourhood holds the NanoPlague shell, which stays full");
		report.Check(first->GetChargeCapacity() == 2 && second->GetChargeCapacity() == 2 && third->GetChargeCapacity() == 2, "level 1 boosts each shell to capacity 2");

		report.Check(Spend(first), "overflow check: spend one from the first shell");
		WaitUseGap(first);
		report.Check(Spend(second), "overflow check: spend one from the second shell");
		report.Check(first->GetCharges() == 1 && second->GetCharges() == 1 && third->GetCharges() == 2, "two shells hold one charge and the third is full");

		// The grant of two goes to the fewest-charged shell (one room) and the surplus spills to the other shell with room.
		applier.Tick(kTrickleSeconds);
		report.Check(first->GetCharges() == 2 && second->GetCharges() == 2 && third->GetCharges() == 2, "the surplus spills to the other shell with room",
			Json{ { "first", first->GetCharges() }, { "second", second->GetCharges() }, { "third", third->GetCharges() } });

		applier.Tick(3.f * kTrickleSeconds);
		report.Check(first->GetCharges() == 2 && second->GetCharges() == 2 && third->GetCharges() == 2, "a full neighbourhood drops the surplus and never exceeds capacity");
	}

	// Scenario 8: the interval belongs to the placement and accumulates across frames.
	void TrickleIntervalChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::uint32_t instance = InstallSpare(f, 3, 2, kTrickleId);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		auto& applier = f.ship->GetModuleApplier();
		GameAbility* target = AbilityIn(f, SpendableSlot(f, credited, 0));
		report.Check(Spend(target), "interval check spends one charge");
		applier.Tick(kTrickleSeconds * 0.5f);
		report.Check(target->GetCharges() == 2, "half an interval grants nothing");
		applier.Tick(kTrickleSeconds * 0.5f);
		report.Check(target->GetCharges() == 3, "two halves make one interval");
	}

	// Shipped content: ShippedLoaderChecks reads the real shipped files. The placement checks below use the same shipped entries,
	// which the fixture set copies verbatim, so they exercise the shipped values through the real loader and placement code.
	std::filesystem::path ShippedDataPath(const char* fileName)
	{
		return std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data" / fileName;
	}

	void ShippedLoaderChecks(Report& report)
	{
		const auto modules = content::ModuleLoader::LoadFromFile(ShippedDataPath("modules.json"));
		report.Check(modules.Succeeded(), "shipped modules.json loads through the real ModuleLoader", modules.error);
		const ModuleDefinition* spare = nullptr;
		for (const ModuleDefinition& definition : modules.definitions)
			if (definition.moduleId == kRealSpareId) spare = &definition;
		report.Check(spare != nullptr, "shipped Spare Capacitor is present in modules.json");
		report.Check(spare->levelCount == 3 && spare->sizeVariants.size() == 3, "shipped Spare Capacitor has three levels and three sizes");
		report.Check(AllowsPrivateSideFeature(*spare, sas::ContentId{ kRealTrickleId }), "shipped Spare Capacitor permits Trickle Charge");

		for (std::size_t index = 0; index < 3; ++index)
		{
			const auto& variant = spare->sizeVariants[index];
			const std::string label = "shipped size " + std::to_string(index + 1);
			report.Check(variant.shape.cells.size() == index + 1, label + " covers " + std::to_string(index + 1) + " cell(s)");
			const bool table = variant.chargeRules.size() == 1 && variant.chargeRules[0].chargePerLevel.values.size() == 3 &&
				variant.chargeRules[0].chargePerLevel.values[0] == 1 && variant.chargeRules[0].chargePerLevel.values[1] == 2 &&
				variant.chargeRules[0].chargePerLevel.values[2] == 3;
			report.Check(table, label + " carries the per-level capacity table 1/2/3");
		}

		const auto sides = content::ModuleSideFeatureLoader::LoadFromFile(ShippedDataPath("module_side_features.json"));
		report.Check(sides.Succeeded(), "shipped module_side_features.json loads through the real side-feature loader", sides.error);
		const ModuleSideFeatureDefinition* trickle = nullptr;
		for (const ModuleSideFeatureDefinition& feature : sides.definitions)
			if (feature.sideFeatureId == kRealTrickleId) trickle = &feature;
		report.Check(trickle != nullptr && trickle->levelCount == 3 && trickle->rules.chargeRules.size() == 1 &&
			Near(trickle->rules.chargeRules[0].trickleSeconds, kTrickleSeconds) && trickle->rules.chargeRules[0].trickleCharges == 1 &&
			!trickle->rules.chargeRules[0].overflowTransfer, "shipped Trickle Charge grants one charge every 20 s and no overflow");
		report.Check(content::ModuleContentCatalog::FindById(kRealSpareId) != nullptr && content::ModuleSideFeatureCatalog::FindById(kRealTrickleId) != nullptr,
			"the content catalog serves the shipped Spare Capacitor and Trickle Charge");
	}

	// Shipped content on the Breacher layout: level bonus per size, neighbour count, and the largest-size gate of the side feature.
	void ShippedSizeChecks(Report& report, Application& application)
	{
		for (int size = 1; size <= 3; ++size)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				const std::string label = "shipped size " + std::to_string(size) + " level " + std::to_string(level);
				const std::uint32_t instance = InstallSpare(f, size, level, "", kRealSpareId);
				const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
				report.Check(static_cast<int>(credited.size()) == size, label + " credits exactly size-many shells", Json{ { "credited", static_cast<int>(credited.size()) } });
				CheckCapacities(report, f, credited, level, label);

				const ResolvedModuleEffects effects = ModuleResolver::Resolve(f.rules, f.loadout.GetPlacements());
				int bonusRules = 0;
				for (const ResolvedModuleChargeRule& rule : effects.chargeRules)
				{
					if (rule.instanceId != instance) continue;
					++bonusRules;
					report.Check(rule.chargeBonus == level && rule.slots.size() == static_cast<std::size_t>(size), label + " resolves the level bonus on size-many slots",
						Json{ { "chargeBonus", rule.chargeBonus }, { "slots", static_cast<int>(rule.slots.size()) } });
				}
				report.Check(bonusRules == 1, label + " resolves exactly one bonus rule", Json{ { "rules", bonusRules } });
			}
		}

		// Trickle Charge is a largest-size rule: the resolver applies it only to the three-cell variant.
		for (int size = 1; size <= 3; ++size)
		{
			Fixture f{ report, application };
			const std::string label = "shipped size " + std::to_string(size) + " with Trickle Charge";
			const std::uint32_t instance = InstallSpare(f, size, 2, kRealTrickleId, kRealSpareId);
			const ResolvedModuleEffects effects = ModuleResolver::Resolve(f.rules, f.loadout.GetPlacements());
			int trickles = 0;
			for (const ResolvedModuleChargeRule& rule : effects.chargeRules)
			{
				if (rule.instanceId != instance || !(rule.trickleSeconds > 0.f)) continue;
				++trickles;
				report.Check(Near(rule.trickleSeconds, kTrickleSeconds) && rule.trickleCharges == 1 && rule.slots.size() == 3,
					label + " grants one charge per interval on three slots");
			}
			const int expected = size == 3 ? 1 : 0;
			report.Check(trickles == expected, size == 3 ? "largest size applies Trickle Charge" : "smaller size applies no Trickle Charge", Json{ { "trickleRules", trickles } });
		}
	}

	// Shipped content: ten attach/detach rounds of the largest level-3 spare with Trickle Charge keep capacity, handle and lease count stable.
	void ShippedRoundChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const std::size_t baseline = f.ship->GetModuleApplier().GetOwnedHandleCount();
		const std::uint32_t instance = InstallSpare(f, 3, 3, kRealTrickleId, kRealSpareId);
		const ModulePlacement placement = *f.loadout.FindPlacement(instance);
		const List<sas::AbilitySlot> credited = CreditedSlots(f, instance);
		report.Check(credited.size() == 3, "shipped size-3 spare credits three shells");
		const std::size_t installed = f.ship->GetModuleApplier().GetOwnedHandleCount();
		GameAbility* boosted = AbilityIn(f, credited[0]);
		const sas::AbilityHandle handle = boosted->GetHandle();
		report.Check(boosted->GetChargeCapacity() == 4, "shipped level 3 boosts the credited shell to capacity 4");

		report.Check(f.loadout.Uninstall(instance) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "shipped uninstall reapplies the loadout");
		report.Check(CapacityOf(f, credited[0]) == 1 && CapacityOf(f, credited[1]) == 1 && CapacityOf(f, credited[2]) == 1, "shipped removal restores capacity 1");
		report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, "shipped removal returns the lease count to baseline");

		for (int round = 1; round <= 10; ++round)
		{
			const std::string label = "shipped round " + std::to_string(round);
			report.Check(f.loadout.Install(f.rules, instance, placement.origin, placement.rotationQuarterTurns) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules),
				label + " reinstalls the spare at its origin");
			report.Check(CapacityOf(f, credited[0]) == 4 && CapacityOf(f, credited[1]) == 4 && CapacityOf(f, credited[2]) == 4, label + " restores the boosted capacity");
			report.Check(AbilityIn(f, credited[0]) == boosted && boosted->GetHandle() == handle, label + " keeps the same ability object and handle");
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == installed, label + " keeps the same lease count while installed");
			report.Check(f.loadout.Uninstall(instance) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), label + " uninstalls again");
			report.Check(CapacityOf(f, credited[0]) == 1 && CapacityOf(f, credited[1]) == 1 && CapacityOf(f, credited[2]) == 1, label + " leaves no stale capacity",
				Json{ { "capacity", CapacityOf(f, credited[0]) }, { "charges", ChargesOf(f, credited[0]) } });
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baseline, label + " returns the lease count to baseline");
		}
		report.Check(AbilityIn(f, credited[0]) == boosted && boosted->GetHandle() == handle, "shipped handle stays identical after ten rounds");
	}
}

int main(int argc, char** argv)
{
	ly::InstallLightYearsEngineAssets();
	if (argc != 2) return 2;
	Report report;
	report.artifact["test"] = "RareSpareCapacitorMechanicsE2E";
	report.artifact["inputs"] = Json{ { "ship", kShipId }, { "module", kSpareId }, { "chargePerLevel", Json::array({ 1, 2, 3 }) },
		{ "overflowFixture", "overflowTransfer true, trickleCharges 2" }, { "trickleFixture", "trickleCharges 1" }, { "trickleSeconds", kTrickleSeconds } };
	report.artifact["scenarios"] = Json::array();
	bool allPassed = true;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare spare capacitor content registration failed" };
		std::string title = "Rare Spare Capacitor E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-spare-capacitor-fixtures";
		Json shippedModules;
		Json shippedSideFeatures;
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json" };
			input >> shippedModules;
		}
		{
			std::ifstream input{ std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/module_side_features.json" };
			input >> shippedSideFeatures;
		}
		const auto modulesPath = WriteJson(root, kModulesFile, ModuleFixtureSet(shippedModules));
		const auto sideFeaturesPath = WriteJson(root, kSideFeaturesFile, SideFeatureFixtureSet(shippedSideFeatures));
		const auto loaded = content::ModuleLoader::LoadFromFile(modulesPath);
		report.Check(loaded.Succeeded(), "spare fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;
		std::string catalogFailure;
		report.Check(content::ModuleContentCatalog::LoadFromFiles(modulesPath, sideFeaturesPath, &catalogFailure),
			"the content catalog reads the same spare fixtures", catalogFailure);

		Json& scenarios = report.artifact["scenarios"];
		const auto run = [&](const char* name, const std::function<void()>& body) { if (!RunScenario(scenarios, name, body)) allPassed = false; };
		run("0 loader: accepted forms read back, misuse refused", [&] { LoaderChecks(report, root, shippedModules, shippedSideFeatures, sideFeaturesPath); });
		run("1 capacity: 1, 2 and 3 cells at levels 1-3 boost only the credited shells", [&] { CapacityChecks(report, application); });
		run("2 consecutive use: boosted shell spends through the real ASC and the anti-spam gap", [&] { ConsecutiveUseChecks(report, application); });
		run("3 detach and ten attach/detach rounds keep capacity, handle and lease count stable", [&] { DetachRoundChecks(report, application); });
		run("4 grant: ASC grant fills missing charges only and never raises capacity", [&] { GrantChecks(report, application); });
		run("5 no side feature: nothing is granted over time", [&] { NoSideFeatureChecks(report, application); });
		run("6 trickle charge: the fewest-charged shell takes one charge per interval, capacity kept", [&] { TrickleChecks(report, application); });
		run("7 overflow transfer: a surplus grant spills to the next shell with room", [&] { OverflowChecks(report, application); });
		run("8 trickle interval accumulates across frames", [&] { TrickleIntervalChecks(report, application); });
		run("9 shipped content: the real Spare Capacitor and Trickle Charge load through the real loaders", [&] { ShippedLoaderChecks(report); });
		run("10 shipped content: level table, neighbour count and the largest-size side gate", [&] { ShippedSizeChecks(report, application); });
		run("11 shipped content: ten attach/detach rounds keep capacity, handle and lease count stable", [&] { ShippedRoundChecks(report, application); });
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		std::cerr << exception.what() << '\n';
		allPassed = false;
	}
	report.artifact["passed"] = allPassed;
	WriteArtifact(argv[1], report.artifact);
	return allPassed ? 0 : 1;
}
