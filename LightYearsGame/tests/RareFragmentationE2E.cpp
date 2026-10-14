#include "UncommonModuleP4E2E.h"

#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/ModuleSideFeature.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>

namespace rare_fragmentation_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kEmber = "Module.Offense.EmberAccumulation.Rare";
		constexpr const char* kSpare = "Module.Utility.SpareCapacitor.Rare";
		constexpr const char* kTrophy = "Module.Offense.TrophyCore.Rare";
		constexpr const char* kStrippedHull = "Module.Utility.StrippedHull.Rare";
		constexpr const char* kOdometer = "Module.Movement.Odometer.Rare";
		constexpr const char* kFragment = "Module.SideFeature.Shared.F1.Rare";
		constexpr const char* kBountyMark = "Module.SideFeature.TrophyCore.BountyMark";
		constexpr const char* kLadderModules[] = { kEmber, kSpare, "Module.Defense.ShatterShock.Rare", "Module.Offense.KineticBreaker.Rare",
			"Module.Offense.GiantSlayer.Rare", "Module.Movement.EscapeVelocity.Rare", "Module.Offense.DyingStar.Rare" };

		// Free cells of the shipped fighter (Ship.Player.Fighter.Basic). The ability bay holds the shells at (5,5), (6,5), (9,5) and
		// (10,5). (4,5) is Wings beside the Ability1 shell, (9,6) is Hull below the Ability3 shell and (11,5) is Wings beside the
		// Ability4 shell. (5,6) is Hull below the Ability1 shell only. (2,6), (3,6) and (4,6) are free Wings cells.
		const sf::Vector2i kWingLeft{ 4, 5 };
		const sf::Vector2i kHull{ 9, 6 };
		const sf::Vector2i kWingRight{ 11, 5 };
		const sf::Vector2i kHullBelowShell1{ 5, 6 };
		const sf::Vector2i kWingFar{ 2, 6 };
		const sf::Vector2i kWingMid{ 3, 6 };
		const sf::Vector2i kWingNear{ 4, 6 };

		std::filesystem::path WriteJson(const std::filesystem::path& root, const std::string& name, const Json& value)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / name;
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			output << value.dump(2) << '\n';
			return path;
		}

		List<sf::Vector2i> Cells(std::initializer_list<sf::Vector2i> cells)
		{
			return List<sf::Vector2i>{ cells };
		}

		bool SameCells(const List<sf::Vector2i>& left, const List<sf::Vector2i>& right)
		{
			if (left.size() != right.size()) return false;
			for (std::size_t index = 0; index < left.size(); ++index)
				if (left[index] != right[index]) return false;
			return true;
		}

		template<typename T>
		bool Has(const List<T>& values, const T& value)
		{
			return std::find(values.begin(), values.end(), value) != values.end();
		}

		const ModuleDefinition* Definition(const Fixture& f, const char* id)
		{
			const ModuleDefinition* definition = f.rules.lookup(id);
			if (!definition) throw std::runtime_error{ std::string{ "Rare fragmentation module is missing: " } + id };
			return definition;
		}

		// Inventory item at a size variant with an optional side feature; 0 when the inventory refuses it.
		std::uint32_t Item(Fixture& f, const char* id, int variant, int level = 1, const char* side = "")
		{
			const ModuleDefinition* definition = Definition(f, id);
			return f.loadout.AddToInventory(f.rules, *definition, variant, definition->rarity, level, sas::ContentId{ side });
		}

		// Whole install of an inventory item on the first legal origin, the way Fixture::Install searches.
		bool InstallWhole(Fixture& f, std::uint32_t instance)
		{
			const ModulePlacement item = *f.loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < f.rules.schematic.height; ++y)
					for (int x = 0; x < f.rules.schematic.width; ++x)
					{
						ModulePlacement candidate = item;
						candidate.origin = sf::Vector2i{ x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
						return f.loadout.Install(f.rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
					}
			return false;
		}

		const ModulePlacement* Shell(const Fixture& f, const GameplayTag& system)
		{
			for (const ModulePlacement& placement : f.loadout.GetPlacements())
			{
				const ModuleDefinition* definition = f.rules.lookup(placement.moduleId);
				if (definition && definition->systemCategory == system) return &placement;
			}
			return nullptr;
		}

		// Zone bonus the fragments should share: each fragment's family bonus, summed and divided by the cell count.
		float ExpectedShare(const Fixture& f, const ModuleDefinition& module, const List<sf::Vector2i>& cells)
		{
			float total = 0.f;
			for (const sf::Vector2i& cell : cells)
			{
				const ModuleZoneEntry* entry = f.rules.zoneTable.FindZone(f.rules.schematic.GetZoneAt(cell.x, cell.y));
				if (entry && (entry->anyFamily || entry->family == module.family)) total += entry->bonus;
			}
			return total / static_cast<float>(cells.size());
		}

		const ResolvedModuleChargeRule* FindCharge(const ResolvedModuleEffects& effects, std::uint32_t instance)
		{
			for (const ResolvedModuleChargeRule& rule : effects.chargeRules)
				if (rule.instanceId == instance) return &rule;
			return nullptr;
		}

		void ShippedChecks(Report& report)
		{
			const ModuleSideFeatureDefinition* fragment = content::ModuleSideFeatureCatalog::FindById(sas::ContentId{ kFragment });
			report.Check(fragment != nullptr && fragment->rules.fragmentation, "the shipped F1 side feature is the fragmentation switch");
			for (const char* id : kLadderModules)
			{
				const ModuleDefinition* module = content::ModuleContentCatalog::FindById(sas::ContentId{ id });
				report.Check(module != nullptr && AllowsSideFeature(*module, sas::ContentId{ kFragment }), std::string{ "F1 is in the shared pool of " } + id);
			}
			const ModuleDefinition* trophy = content::ModuleContentCatalog::FindById(sas::ContentId{ kTrophy });
			report.Check(trophy != nullptr && !AllowsSideFeature(*trophy, sas::ContentId{ kFragment }), "Trophy Core has no F1 permission, so it never splits");
		}

		void RejectionChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::uint32_t plainEmber = Item(f, kEmber, 1);
			report.Check(plainEmber != 0, "an Ember without a side feature enters the inventory");
			report.Check(f.loadout.InstallFragmented(f.rules, plainEmber, Cells({ kWingLeft, kHull, kWingRight })) == ModulePlacementFailure::FragmentationNotAllowed,
				"a module without F1 cannot be fragmented");

			const std::uint32_t trophy = Item(f, kTrophy, 1, 1, kBountyMark);
			report.Check(trophy != 0, "Trophy Core with its own side feature enters the inventory");
			report.Check(f.loadout.InstallFragmented(f.rules, trophy, Cells({ kWingLeft, kHull })) == ModulePlacementFailure::FragmentationNotAllowed,
				"Trophy Core stays whole");

			const std::uint32_t hull = Item(f, kStrippedHull, 0);
			report.Check(hull != 0 && f.loadout.InstallFragmented(f.rules, hull, Cells({ kWingLeft })) == ModulePlacementFailure::FragmentationNotAllowed,
				"a single-size module without a side feature cannot be fragmented");

			const std::uint32_t smaller = Item(f, kEmber, 0, 1, kFragment);
			report.Check(smaller != 0 && f.loadout.InstallFragmented(f.rules, smaller, Cells({ kWingLeft, kHullBelowShell1 })) == ModulePlacementFailure::FragmentationNotAllowed,
				"F1 splits only the largest size");

			const std::uint32_t split = Item(f, kEmber, 1, 1, kFragment);
			report.Check(split != 0, "an Ember with F1 enters the inventory at the largest size");
			report.Check(f.loadout.InstallFragmented(f.rules, split, Cells({ kWingLeft, kHullBelowShell1 })) == ModulePlacementFailure::InvalidFragmentCells,
				"two fragments do not cover a three-cell module");
			report.Check(f.loadout.InstallFragmented(f.rules, split, Cells({ kWingLeft, kWingLeft, kHull })) == ModulePlacementFailure::InvalidFragmentCells,
				"a repeated fragment cell is rejected");
			report.Check(f.loadout.InstallFragmented(f.rules, split, Cells({ sf::Vector2i{ 5, 5 }, kHull, kWingRight })) != ModulePlacementFailure::None,
				"a fragment cannot sit on an ability shell");
			report.Check(f.loadout.FindPlacement(split) == nullptr && f.loadout.FindInventoryItem(split) != nullptr && !f.loadout.FindInventoryItem(split)->fragmented,
				"rejected fragment installs leave the item in the inventory, whole");
		}

		void FragmentChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::uint32_t ember = Item(f, kEmber, 1, 1, kFragment);
			report.Check(ember != 0, "an Ember with F1 enters the inventory");
			const List<sf::Vector2i> cells = Cells({ kWingLeft, kHull, kWingRight });
			report.Check(f.loadout.InstallFragmented(f.rules, ember, cells) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules),
				"a fragmentable Ember installs as three one-cell fragments through the real applier");
			const ModulePlacement* placed = f.loadout.FindPlacement(ember);
			report.Check(placed != nullptr && placed->fragmented && SameCells(placed->fragmentCells, cells) && f.loadout.FindInventoryItem(ember) == nullptr,
				"one placement holds the fragment cells and the inventory no longer holds the item");
			report.Check(SameCells(ModulePlacementRules::GetCoveredCells(*Definition(f, kEmber), *placed), cells), "GetCoveredCells lists exactly the fragment cells");

			// Zone share: two Wings fragments (Offense bonus 0.25) and one Hull fragment (any family 0.10), over three cells.
			const float expected = ExpectedShare(f, *Definition(f, kEmber), cells);
			report.Check(expected > 0.199f && expected < 0.201f, "the hand-checked share is (0.25 + 0.25 + 0.10) / 3",
				Json{ { "expected", expected } });
			report.CheckValue("the fragmented zone bonus is proportional to each fragment's zone", expected,
				ModuleResolver::GetZoneBonus(f.rules, *Definition(f, kEmber), *placed), "two Wings fragments and one Hull fragment");

			// Neighbourhood counts every fragment: the Ability1, Ability3 and Ability4 shells all touch the module.
			const List<GameplayTag> adjacent = ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), *placed, 4);
			report.Check(adjacent.size() == 3 && Has(adjacent, GameplayTags::Module::System::AbilitySlot1) &&
				Has(adjacent, GameplayTags::Module::System::AbilitySlot3) && Has(adjacent, GameplayTags::Module::System::AbilitySlot4),
				"neighbours are counted from every fragment: three shells touch three different fragments", Json{ { "neighbours", adjacent.size() } });

			// Move: a whole move is refused, the fragments move as a set.
			report.Check(f.loadout.Move(f.rules, ember, sf::Vector2i{ 2, 6 }, 0) == ModulePlacementFailure::FragmentedPlacement,
				"a fragmented placement refuses a whole Move");
			const List<sf::Vector2i> moved = Cells({ kWingFar, kWingMid, kWingNear });
			report.Check(f.loadout.MoveFragments(f.rules, ember, moved) == ModulePlacementFailure::None && SameCells(f.loadout.FindPlacement(ember)->fragmentCells, moved),
				"MoveFragments places every fragment on the new cells");
			report.Check(f.loadout.MoveFragments(f.rules, ember, Cells({ kWingFar, kWingMid })) == ModulePlacementFailure::InvalidFragmentCells,
				"MoveFragments keeps one fragment per cell of the largest size");

			// Swap: an ability shell dropped onto a fragment never swaps with it.
			const ModulePlacement* shell = Shell(f, GameplayTags::Module::System::AbilitySlot1);
			report.Check(shell != nullptr, "the Ability1 shell is installed");
			const std::uint32_t shellId = shell->instanceId;
			const sf::Vector2i shellOrigin = shell->origin;
			report.Check(f.loadout.Move(f.rules, shellId, kWingFar, 0) != ModulePlacementFailure::None && SameCells(f.loadout.FindPlacement(ember)->fragmentCells, moved) &&
				f.loadout.FindPlacement(shellId)->origin == shellOrigin, "an ability shell does not swap with a fragment");

			// Uninstall: the item returns whole to the inventory with its F1 side feature, and the fragments are gone.
			report.Check(f.loadout.Uninstall(ember) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "uninstalling a fragmented module goes through the real applier");
			const ModulePlacement* item = f.loadout.FindInventoryItem(ember);
			report.Check(item != nullptr && !item->fragmented && item->fragmentCells.empty() && item->sideFeatureId == sas::ContentId{ kFragment } && f.loadout.FindPlacement(ember) == nullptr,
				"the inventory item keeps F1 and drops its fragment cells");
			report.Check(f.loadout.InstallFragmented(f.rules, ember, moved) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules),
				"the same item re-installs as fragments");

			// Ten uninstall and fragment install cycles keep the applier's handle count and the placement.
			const std::size_t handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			bool cycles = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				cycles = f.loadout.Uninstall(ember) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && cycles;
				cycles = f.loadout.InstallFragmented(f.rules, ember, moved) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && cycles;
			}
			report.Check(cycles && f.ship->GetModuleApplier().GetOwnedHandleCount() == handles && SameCells(f.loadout.FindPlacement(ember)->fragmentCells, moved),
				"ten uninstall and fragment install cycles keep the handle count and the fragment cells");
		}

		void ChargeChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::uint32_t spare = Item(f, kSpare, 2, 1, kFragment);
			report.Check(spare != 0, "Spare Capacitor with F1 enters the inventory at the largest size");
			report.Check(f.loadout.InstallFragmented(f.rules, spare, Cells({ kWingLeft, kHull, kWingRight })) == ModulePlacementFailure::None &&
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "a fragmented Spare Capacitor installs");

			// Each fragment reaches its neighbour: Ability1, Ability3 and Ability4 each get the level-1 charge once.
			const ResolvedModuleEffects effects = ModuleResolver::Resolve(f.rules, f.loadout.GetPlacements());
			const ResolvedModuleChargeRule* charge = FindCharge(effects, spare);
			report.Check(charge != nullptr && charge->chargeBonus == 1 && charge->slots.size() == 3 && Has(charge->slots, sas::AbilitySlot::Ability1) &&
				Has(charge->slots, sas::AbilitySlot::Ability3) && Has(charge->slots, sas::AbilitySlot::Ability4),
				"three fragments reach three abilities, each credited with the level-1 charge");

			// Two fragments on the same shell credit it once: no stacking per fragment.
			report.Check(f.loadout.MoveFragments(f.rules, spare, Cells({ kWingLeft, kHullBelowShell1, kWingRight })) == ModulePlacementFailure::None &&
				f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "the fragments move so two touch Ability1");
			const ResolvedModuleEffects stacked = ModuleResolver::Resolve(f.rules, f.loadout.GetPlacements());
			const ResolvedModuleChargeRule* once = FindCharge(stacked, spare);
			report.Check(once != nullptr && once->chargeBonus == 1 && once->slots.size() == 2 && Has(once->slots, sas::AbilitySlot::Ability1) && Has(once->slots, sas::AbilitySlot::Ability4),
				"two fragments on one shell do not stack: Ability1 takes the charge once");

			// Level 3 raises the same single credit to three.
			report.Check(f.loadout.SetLevel(f.rules, spare, 3) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "the fragmented Spare Capacitor reaches level 3");
			const ResolvedModuleEffects level3 = ModuleResolver::Resolve(f.rules, f.loadout.GetPlacements());
			const ResolvedModuleChargeRule* top = FindCharge(level3, spare);
			report.Check(top != nullptr && top->chargeBonus == 3 && top->slots.size() == 2, "level 3 credits each reached ability three charges, still once each");
		}

		void WholeChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::uint32_t whole = Item(f, kEmber, 1, 1, kFragment);
			report.Check(whole != 0 && InstallWhole(f, whole), "a whole Ember with F1 installs at a legal origin");
			const ModulePlacement* placed = f.loadout.FindPlacement(whole);
			report.Check(placed != nullptr && !placed->fragmented && placed->fragmentCells.empty(), "an unsplit F1 module keeps its origin layout");
			const List<sf::Vector2i> covered = ModulePlacementRules::GetCoveredCells(*Definition(f, kEmber), *placed);
			report.Check(covered.size() == 3, "the unsplit placement still covers its three shape cells");

			// Unsplit rule as before: the whole placement takes its single zone's bonus, or nothing when its cells are mixed.
			const GameplayTag firstZone = f.rules.schematic.GetZoneAt(covered.front().x, covered.front().y);
			bool uniform = true;
			for (const sf::Vector2i& cell : covered)
				if (f.rules.schematic.GetZoneAt(cell.x, cell.y) != firstZone) uniform = false;
			const ModuleZoneEntry* entry = f.rules.zoneTable.FindZone(firstZone);
			const bool familyZone = entry != nullptr && (entry->anyFamily || entry->family == Definition(f, kEmber)->family);
			const float expected = uniform && familyZone ? entry->bonus : 0.f;
			report.CheckValue("an unsplit placement keeps the whole-zone bonus rule", expected,
				ModuleResolver::GetZoneBonus(f.rules, *Definition(f, kEmber), *placed), "one whole Ember placement");
		}

		void CounterChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::uint32_t odometer = f.Install(kOdometer);
			report.Check(f.loadout.AccrueRunCounters(f.rules, 6000.0) && f.loadout.GetRunCounter(odometer) == 6000.0,
				"the installed Odometer accrues distance");
			report.Check(f.loadout.Uninstall(odometer) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && f.loadout.GetRunCounter(odometer) == 6000.0,
				"uninstalling keeps the Odometer counter on the item");
			report.Check(InstallWhole(f, odometer) && f.loadout.GetRunCounter(odometer) == 6000.0, "reinstalling keeps the Odometer counter");
			const std::size_t handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			bool cycles = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				cycles = f.loadout.Uninstall(odometer) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && cycles;
				cycles = InstallWhole(f, odometer) && cycles;
			}
			report.Check(cycles && f.ship->GetModuleApplier().GetOwnedHandleCount() == handles && f.loadout.GetRunCounter(odometer) == 6000.0,
				"ten Odometer uninstall and install cycles keep the handle count and the counter");
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shippedModules, const Json& shippedSideFeatures)
		{
			Json wholeFragmented = shippedModules;
			for (auto& module : wholeFragmented["modules"])
				if (module["id"] == kEmber) module["sizeVariants"][0]["fragmentation"] = true;
			const auto modulePath = WriteJson(root, "reject-main-fragmentation.json", wholeFragmented);
			const auto loadedModules = content::ModuleLoader::LoadFromFile(modulePath);
			report.Check(!loadedModules.Succeeded() && loadedModules.error.find("side-feature only") != std::string::npos,
				"the main package rejects fragmentation", Json{ { "error", loadedModules.error } });

			Json falseFragmentation = shippedSideFeatures;
			falseFragmentation["sideFeatures"].push_back(Json{ { "id", "Module.SideFeature.RareFragmentationFixture.Off" }, { "displayName", "Fragment fixture off" },
				{ "description", "Fragmentation fixture." }, { "levelCount", 3 }, { "rules", Json{ { "fragmentation", false } } } });
			const auto featurePath = WriteJson(root, "reject-false-fragmentation.json", falseFragmentation);
			const auto loadedFeatures = content::ModuleSideFeatureLoader::LoadFromFile(featurePath);
			report.Check(!loadedFeatures.Succeeded() && loadedFeatures.error.find("must be true when present") != std::string::npos,
				"the side loader rejects an explicit false fragmentation switch", Json{ { "error", loadedFeatures.error } });
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_fragmentation_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare fragmentation content registration failed" };
		std::string title = "Rare Fragmentation E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-fragmentation-fixtures";
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

		ShippedChecks(report);
		RejectionChecks(report, application);
		FragmentChecks(report, application);
		ChargeChecks(report, application);
		WholeChecks(report, application);
		CounterChecks(report, application);
		LoaderChecks(report, root, shippedModules, shippedSideFeatures);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareFragmentationE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
