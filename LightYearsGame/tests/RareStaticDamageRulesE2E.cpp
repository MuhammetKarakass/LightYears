#include "CommonModuleE2ETestSupport.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/ModuleRulePredicates.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace rare_static_damage_rules_e2e
{
	using namespace ly;
	using namespace common_module_e2e;
	namespace
	{
		constexpr const char* kPlainId = "Module.Utility.StaticPlainFixture.Rare";
		constexpr const char* kFillerId = "Module.Utility.StaticFillerFixture.Rare";
		constexpr const char* kHullId = "Module.Utility.StrippedHullFixture.Rare";
		constexpr const char* kCoreId = "Module.Utility.TripleCoreFixture.Rare";
		constexpr const char* kRegressionId = "Module.Utility.StaticRegressionFixture.Rare";
		constexpr float kProbeDamage = 1000.f;
		constexpr std::array<float, 3> kHullPerCell{ 0.025f, 0.03f, 0.035f };
		constexpr std::array<float, 3> kCoreMultiplier{ 1.25f, 1.30f, 1.35f };

		struct TextCase
		{
			const char* name;
			const char* rules;
			const char* expected;
		};

		struct AcceptCase
		{
			const char* name;
			const char* rules;
			int slots;
			float perCell;
		};

		DamagePayload Plain()
		{
			DamagePayload payload;
			payload.criticalPolicy = DamageCriticalPolicy::Disabled;
			payload.kineticStacks = 0;
			return payload;
		}

		// Every fixture module keeps the shipped one-cell footprint and the Ability.Utility family.
		Json TestModule(const char* id, const Json& damageRules)
		{
			return Json{
				{ "id", id },
				{ "displayName", id },
				{ "description", "Rare static damage rule test fixture" },
				{ "family", "Ability.Utility" },
				{ "levelCount", 3 },
				{ "sizeVariants", Json::array({ Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) }, { "damageRules", damageRules } } }) }
			};
		}

		// The shipped catalog plus test modules. The test modules are appended, so every shipped module still resolves.
		Json FixtureDocument()
		{
			const auto dataPath = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data";
			std::ifstream input{ dataPath / "modules.json" };
			if (!input) throw std::runtime_error{ "Shipped modules.json can be opened" };
			Json document;
			input >> document;
			document["modules"].push_back(TestModule(kPlainId, Json::array()));
			document["modules"].push_back(TestModule(kFillerId, Json::array()));
			document["modules"].push_back(TestModule(kHullId, Json::array({ Json{ { "condition", "Always" }, { "perEmptyCellMultiplier", Json::array({ 0.025, 0.03, 0.035 }) } } })));
			document["modules"].push_back(TestModule(kCoreId, Json::array({ Json{ { "condition", "AbilityDirectHit" }, { "requiredEmptyAbilitySlots", 1 }, { "damageMultiplier", Json::array({ 1.25, 1.3, 1.35 }) } } })));
			document["modules"].push_back(TestModule(kRegressionId, Json::array({ Json{ { "condition", "PrimaryShotOnly" }, { "damageMultiplier", 1.2 } }, Json{ { "condition", "Always" }, { "damageMultiplier", 1.1 } } })));
			return document;
		}

		void SetCoreRules(Json& document, const Json& damageRules)
		{
			for (auto& module : document["modules"])
				if (module["id"] == kCoreId) module["sizeVariants"][0]["damageRules"] = damageRules;
		}

		content::ModuleLoader::Result LoadFixture(const std::filesystem::path& root, const std::string& name, const Json& document)
		{
			std::filesystem::create_directories(root);
			const std::filesystem::path path = root / (name + ".json");
			std::ofstream output{ path, std::ios::out | std::ios::trunc };
			if (!output) throw std::runtime_error{ "Could not write static damage fixture" };
			output << document.dump(2) << '\n';
			output.close();
			return content::ModuleLoader::LoadFromFile(path);
		}

		// Test modules come from the loaded fixture catalog; everything else from the shipped catalog.
		const ModuleDefinition* FindFixtureModule(const content::ModuleLoader::Result& loaded, const sas::ContentId& moduleId)
		{
			for (const ModuleDefinition& definition : loaded.definitions)
				if (definition.moduleId.ToString() == moduleId.ToString()) return &definition;
			return content::ModuleContentCatalog::FindById(moduleId);
		}

		const ModuleDamageRule* ParsedCoreRule(const content::ModuleLoader::Result& loaded)
		{
			for (const ModuleDefinition& definition : loaded.definitions)
				if (definition.moduleId.ToString() == kCoreId && !definition.sizeVariants.empty() && !definition.sizeVariants.front().damageRules.empty())
					return &definition.sizeVariants.front().damageRules.front();
			return nullptr;
		}

		struct Fixture
		{
			Report& report;
			std::shared_ptr<HeadlessModuleLevel> level;
			std::shared_ptr<PlayerSpaceShip> ship;
			content::ModuleLoader::Result loaded;
			ModuleRulesContext rules;
			ModuleLoadout loadout;

			Fixture(Report& result, Application& application, const std::filesystem::path& root, const std::string& name)
				: report{ result }
				, level{ std::make_shared<HeadlessModuleLevel>(&application) }
				, loaded{ LoadFixture(root, name, FixtureDocument()) }
				, rules{ *content::ShipContentCatalog::GetPlayerFighterDefinition().schematic,
					[this](const sas::ContentId& moduleId) { return FindFixtureModule(loaded, moduleId); },
					content::ModuleContentCatalog::GetZoneTable(),
					[](const sas::ContentId& sideFeatureId) { return content::ModuleSideFeatureCatalog::FindById(sideFeatureId); } }
			{
				report.Check(loaded.Succeeded(), "Static damage fixture loads through the real ModuleLoader", Json{ { "error", loaded.error } });
				level->BeginPlayInternal();
				const auto& definition = content::ShipContentCatalog::GetPlayerShipDefinition("Ship.Player.Fighter.Basic");
				ship = level->SpawnActor<PlayerSpaceShip>(definition).lock();
				report.Check(ship != nullptr, "Static damage E2E spawns a real player ship");
				level->TickInternal(0.f);
				ship->SetInvulnerability(false);
				ship->SetUseScreenClamp(false);
				ship->SetActorLocation({ 10000.f, 10000.f });
				report.Check(loadout.SeedDefaults(rules) && ship->GetModuleApplier().Apply(loadout, rules), "Static damage E2E applies the canonical default loadout through the real applier");
			}
		};

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

		struct Parked
		{
			std::uint32_t instance = 0;
			sf::Vector2i origin{};
			int rotation = 0;
		};

		// Uninstalls the instance through the real loadout and reapplies the layout. Its cell is remembered.
		Parked Park(Fixture& f, std::uint32_t instance)
		{
			const ModulePlacement placed = FindInstalled(f.loadout, instance);
			const Parked parked{ instance, placed.origin, placed.rotationQuarterTurns };
			f.report.Check(f.loadout.Uninstall(instance), "Parks an installed module through the real loadout");
			f.report.Check(f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "Parking reapplies the real layout");
			return parked;
		}

		void Unpark(Fixture& f, const Parked& parked)
		{
			f.report.Check(f.loadout.Install(f.rules, parked.instance, parked.origin, parked.rotation) == ModulePlacementFailure::None,
				"Reinstalls the parked module on its own cell");
			f.report.Check(f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "Reinstalling reapplies the real layout");
		}

		std::uint32_t Add(Fixture& f, const char* moduleId, int level)
		{
			const ModuleDefinition* definition = f.rules.lookup(sas::ContentId{ moduleId });
			f.report.Check(definition != nullptr, std::string{ "fixture catalog contains " } + moduleId);
			const std::uint32_t instance = f.loadout.AddToInventory(f.rules, *definition, 0, definition->rarity, level);
			f.report.Check(instance != 0, std::string{ "inventory accepts " } + moduleId + " at level " + std::to_string(level));
			return instance;
		}

		// Installs an inventory instance on the first legal cell of the real schematic and reapplies the layout.
		void PlaceFirstLegal(Fixture& f, std::uint32_t instance)
		{
			ModulePlacement candidate = *f.loadout.FindInventoryItem(instance);
			for (int rotation = 0; rotation < 4; ++rotation)
			{
				for (int y = 0; y < f.rules.schematic.height; ++y)
				{
					for (int x = 0; x < f.rules.schematic.width; ++x)
					{
						candidate.origin = { x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
						f.report.Check(f.loadout.Install(f.rules, instance, candidate.origin, rotation) == ModulePlacementFailure::None &&
							f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "Installs on a legal cell through the real placement rules");
						return;
					}
				}
			}
			throw std::runtime_error{ "No legal cell for the static damage fixture" };
		}

		// Replaces the installed instance with another module on the same cell at the given level. Returns the new instance.
		std::uint32_t Swap(Fixture& f, std::uint32_t instance, const char* moduleId, int level)
		{
			const Parked parked = Park(f, instance);
			const std::uint32_t next = Add(f, moduleId, level);
			f.report.Check(f.loadout.Install(f.rules, next, parked.origin, parked.rotation) == ModulePlacementFailure::None,
				std::string{ "replacement " } + moduleId + " installs on the same cell");
			f.report.Check(f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "Replacement reapplies the real layout");
			return next;
		}

		std::shared_ptr<SpaceShip> SpawnTarget(Fixture& f)
		{
			const auto enemy = f.level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			f.level->TickInternal(0.f);
			f.report.Check(enemy != nullptr, "Static damage E2E spawns a real enemy SpaceShip");
			enemy->SetCollisionLayer(CollisionLayer::Enemy);
			enemy->SetCollisionMask(CollisionLayer::None);
			enemy->SetEnablePhysics(false);
			enemy->SetActorLocation({ 20000.f, 20000.f });
			enemy->GetShieldComponent().ChangeShield(-10000.f);
			enemy->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			enemy->GetHealthComponent().SetMaxHealth(100000.f);
			enemy->GetHealthComponent().Regenerate(100000.f);
			return enemy;
		}

		// Health removed by one real hit through ApplyCombatDamage; the target is restored afterwards.
		float Dealt(Fixture& f, Actor& target, const sas::ContentId& abilityId, const List<GameplayTag>& abilityTags, DamageDeliveryType delivery, const DamagePayload& payload)
		{
			HealthComponent* health = target.FindComponent<HealthComponent>();
			const float before = health->GetHealth();
			ApplyCombatDamage(target, kProbeDamage, f.ship.get(), {}, payload, abilityId, abilityTags, delivery, f.ship.get());
			const float dealt = before - health->GetHealth();
			health->Regenerate(health->GetMaxHealth());
			return dealt;
		}

		// Ratio of one real hit with the installed module to the same hit with a rule-free stand-in on the same cell.
		// The stand-in has the same footprint, so the empty-cell count and ability-slot count are identical in both states.
		float Ratio(Fixture& f, std::uint32_t& instance, const char* moduleId, Actor& target, const sas::ContentId& abilityId,
			const List<GameplayTag>& abilityTags, DamageDeliveryType delivery = DamageDeliveryType::Direct, const DamagePayload& payload = Plain())
		{
			const int level = FindInstalled(f.loadout, instance).level;
			const float withModule = Dealt(f, target, abilityId, abilityTags, delivery, payload);
			instance = Swap(f, instance, kPlainId, level);
			const float withoutModule = Dealt(f, target, abilityId, abilityTags, delivery, payload);
			instance = Swap(f, instance, moduleId, level);
			f.report.Check(withoutModule > 0.f, "Stand-in baseline hit deals real health damage", Json{ { "withoutModule", withoutModule } });
			return withModule / withoutModule;
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

		void LoaderChecks(Report& report, const std::filesystem::path& root)
		{
			const TextCase rejects[] = {
				{ "per-cell-incoming", R"([{"incoming":true,"condition":"Always","perEmptyCellMultiplier":0.03}])", "perEmptyCellMultiplier requires an outgoing Always rule" },
				{ "per-cell-not-always", R"([{"condition":"PrimaryShotOnly","perEmptyCellMultiplier":0.03}])", "perEmptyCellMultiplier requires an outgoing Always rule" },
				{ "per-cell-with-multiplier", R"([{"condition":"Always","damageMultiplier":1.2,"perEmptyCellMultiplier":0.03}])", "perEmptyCellMultiplier cannot be combined with damageMultiplier" },
				{ "per-cell-zero", R"([{"condition":"Always","perEmptyCellMultiplier":0}])", "perEmptyCellMultiplier must be positive" },
				{ "per-cell-negative", R"([{"condition":"Always","perEmptyCellMultiplier":-0.1}])", "perEmptyCellMultiplier must be positive" },
				{ "slots-incoming", R"([{"incoming":true,"condition":"Always","damageMultiplier":0.85,"requiredEmptyAbilitySlots":1}])", "requiredEmptyAbilitySlots requires an outgoing rule" },
				{ "slots-zero", R"([{"condition":"AbilityDirectHit","damageMultiplier":1.25,"requiredEmptyAbilitySlots":0}])", "requiredEmptyAbilitySlots" },
				{ "slots-fraction", R"([{"condition":"AbilityDirectHit","damageMultiplier":1.25,"requiredEmptyAbilitySlots":1.5}])", "requiredEmptyAbilitySlots" },
				{ "slots-five", R"([{"condition":"AbilityDirectHit","damageMultiplier":1.25,"requiredEmptyAbilitySlots":5}])", "requiredEmptyAbilitySlots must be between 1 and 4" },
				{ "slots-without-change", R"([{"condition":"AbilityDirectHit","requiredEmptyAbilitySlots":1}])", "must change outgoing damage" },
				{ "ability-direct-with-value", R"([{"condition":"AbilityDirectHit","conditionValue":1,"damageMultiplier":1.25}])", "this damage condition does not accept conditionValue" }
			};
			for (const TextCase& item : rejects)
			{
				Json document = FixtureDocument();
				SetCoreRules(document, Json::parse(item.rules));
				const auto loaded = LoadFixture(root, std::string{ "reject-" } + item.name, document);
				report.Check(!loaded.Succeeded() && loaded.error.find(item.expected) != std::string::npos && loaded.error.find("Module ID") == std::string::npos,
					std::string{ "Loader rejects invalid static damage rule: " } + item.name,
					Json{ { "input", item.rules }, { "error", loaded.error } });
			}

			const AcceptCase accepts[] = {
				{ "per-cell-scalar", R"([{"condition":"Always","perEmptyCellMultiplier":0.025}])", 0, 0.025f },
				{ "per-cell-levels", R"([{"condition":"Always","perEmptyCellMultiplier":[0.025,0.03,0.035]}])", 0, 0.025f },
				{ "slots-four", R"([{"condition":"AbilityDirectHit","damageMultiplier":1.25,"requiredEmptyAbilitySlots":4}])", 4, 0.f },
				{ "slots-one-levels", R"([{"condition":"AbilityDirectHit","damageMultiplier":[1.25,1.3,1.35],"requiredEmptyAbilitySlots":1}])", 1, 0.f }
			};
			for (const AcceptCase& item : accepts)
			{
				Json document = FixtureDocument();
				SetCoreRules(document, Json::parse(item.rules));
				const auto loaded = LoadFixture(root, std::string{ "accept-" } + item.name, document);
				const ModuleDamageRule* parsed = ParsedCoreRule(loaded);
				const bool matches = loaded.Succeeded() && parsed != nullptr && parsed->requiredEmptyAbilitySlots == item.slots &&
					std::abs(parsed->perEmptyCellMultiplier - item.perCell) < 1e-6f;
				report.Check(matches, std::string{ "Loader accepts static damage rule: " } + item.name,
					Json{ { "input", item.rules }, { "error", loaded.error } });
			}
		}

		void StrippedHullChecks(Report& report, Application& application, const std::filesystem::path& root)
		{
			Fixture f{ report, application, root, "stripped-hull" };
			const auto ids = CaptureAbilities(f);
			const sas::ContentId& directAbility = ids[1];
			const List<GameplayTag> primary{ GameplayTags::Ability::Primary };
			const List<GameplayTag> none{};
			const auto enemy = SpawnTarget(f);

			std::uint32_t hull = Add(f, kHullId, 1);
			PlaceFirstLegal(f, hull);
			for (int level = 1; level <= 3; ++level)
			{
				if (level > 1) hull = Swap(f, hull, kHullId, level);
				const int empty = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
				report.Check(empty > 0, "Stripped Hull has usable empty cells to count", Json{ { "empty", empty } });
				const float expected = 1.f + kHullPerCell[static_cast<std::size_t>(level - 1)] * static_cast<float>(empty);
				const std::string at = "Stripped Hull level " + std::to_string(level);
				report.CheckValue(at + " multiplies primary hits by 1 + perCell x empty cells", expected,
					Ratio(f, hull, kHullId, *enemy, sas::ContentId{ "" }, primary), at + " primary");
				report.CheckValue(at + " multiplies ability hits by the same factor", expected,
					Ratio(f, hull, kHullId, *enemy, directAbility, none), at + " ability");
				report.CheckValue(at + " resolves the same multiplier into the applier output", expected,
					ResolvedMultiplier(f, hull), at + " resolved");
			}

			const int emptyBefore = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			const std::uint32_t filler = Add(f, kFillerId, 1);
			PlaceFirstLegal(f, filler);
			const int emptyWith = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			report.Check(emptyWith == emptyBefore - 1, "Installing a one-cell module removes exactly one empty cell",
				Json{ { "before", emptyBefore }, { "after", emptyWith } });
			report.CheckValue("A second module lowers the Stripped Hull factor by one cell", 1.f + kHullPerCell[2] * static_cast<float>(emptyWith),
				Ratio(f, hull, kHullId, *enemy, directAbility, none), "filler installed");
			report.CheckValue("The applier output follows the installed layout", 1.f + kHullPerCell[2] * static_cast<float>(emptyWith),
				ResolvedMultiplier(f, hull), "filler installed resolved");

			Park(f, filler);
			const int emptyAfter = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			report.Check(emptyAfter == emptyBefore, "Removing the module restores the empty-cell count", Json{ { "after", emptyAfter } });
			report.CheckValue("Removing the second module restores the Stripped Hull factor", 1.f + kHullPerCell[2] * static_cast<float>(emptyBefore),
				Ratio(f, hull, kHullId, *enemy, directAbility, none), "filler removed");
		}

		void TripleCoreChecks(Report& report, Application& application, const std::filesystem::path& root)
		{
			Fixture f{ report, application, root, "triple-core" };
			const auto ids = CaptureAbilities(f);
			const List<GameplayTag> primary{ GameplayTags::Ability::Primary };
			const List<GameplayTag> none{};
			const auto enemy = SpawnTarget(f);

			std::array<std::uint32_t, 4> shells{};
			for (int index = 0; index < 4; ++index)
			{
				shells[static_cast<std::size_t>(index)] = FindShell(f, index);
				report.Check(shells[static_cast<std::size_t>(index)] != 0, "Default loadout installs every ability-slot shell", Json{ { "shellIndex", index + 1 } });
			}

			std::uint32_t core = Add(f, kCoreId, 1);
			PlaceFirstLegal(f, core);
			report.CheckValue("Four shells installed: Triple Core is inert on Q/E/F/R hits", 1.f,
				Ratio(f, core, kCoreId, *enemy, ids[1], none), "four shells");
			report.Check(ResolvedRuleCount(f, core) == 0, "Four shells: the inactive rule is absent from the resolved output",
				Json{ { "rules", ResolvedRuleCount(f, core) } });

			const Parked slot1 = Park(f, shells[0]);
			report.CheckValue("One empty slot: Triple Core multiplies a Q/E/F/R direct hit by 1.25", kCoreMultiplier[0],
				Ratio(f, core, kCoreId, *enemy, ids[1], none), "one empty slot, E");
			report.CheckValue("One empty slot: a second Q/E/F/R ability is multiplied too", kCoreMultiplier[0],
				Ratio(f, core, kCoreId, *enemy, ids[2], none), "one empty slot, F");
			report.CheckValue("One empty slot: a third Q/E/F/R ability is multiplied too", kCoreMultiplier[0],
				Ratio(f, core, kCoreId, *enemy, ids[3], none), "one empty slot, R");
			report.CheckValue("One empty slot: Primary hits are not multiplied", 1.f,
				Ratio(f, core, kCoreId, *enemy, sas::ContentId{ "" }, primary), "one empty slot, primary");
			report.CheckValue("One empty slot: periodic (DoT) ticks are not multiplied", 1.f,
				Ratio(f, core, kCoreId, *enemy, ids[1], none, DamageDeliveryType::Periodic), "one empty slot, periodic");
			report.CheckValue("One empty slot: module-generated damage is not multiplied", 1.f,
				Ratio(f, core, kCoreId, *enemy, sas::ContentId{ "" }, none, DamageDeliveryType::Direct, MakeModuleGeneratedDamagePayload()),
				"one empty slot, generated");
			report.Check(ResolvedRuleCount(f, core) == 1 && std::abs(ResolvedMultiplier(f, core) - kCoreMultiplier[0]) < 1e-6f,
				"One empty slot: the applier output carries the level 1 multiplier", Json{ { "rules", ResolvedRuleCount(f, core) } });

			for (int level = 2; level <= 3; ++level)
			{
				core = Swap(f, core, kCoreId, level);
				report.CheckValue("One empty slot at level " + std::to_string(level) + " uses the level table", kCoreMultiplier[static_cast<std::size_t>(level - 1)],
					Ratio(f, core, kCoreId, *enemy, ids[1], none), "level table");
			}

			const Parked slot2 = Park(f, shells[1]);
			report.CheckValue("Two empty slots: Triple Core remains active", kCoreMultiplier[2], Ratio(f, core, kCoreId, *enemy, ids[2], none), "two empty slots");
			report.Check(ResolvedRuleCount(f, core) == 1, "Two empty slots: the active rule remains in the resolved output");

			const Parked slot3 = Park(f, shells[2]);
			report.CheckValue("Three empty slots: the remaining ability stays empowered", kCoreMultiplier[2], Ratio(f, core, kCoreId, *enemy, ids[3], none), "three empty slots");

			Unpark(f, slot3);
			report.CheckValue("Back to two empty slots after one shell returns: still empowered", kCoreMultiplier[2], Ratio(f, core, kCoreId, *enemy, ids[3], none), "two empty slots again");
			Unpark(f, slot2);
			report.CheckValue("Back to one empty slot after a second shell returns: multiplier returns", kCoreMultiplier[2],
				Ratio(f, core, kCoreId, *enemy, ids[2], none), "one empty slot again, level 3");
			Unpark(f, slot1);
			report.CheckValue("All four shells installed again: Triple Core is inert", 1.f, Ratio(f, core, kCoreId, *enemy, ids[1], none), "four shells again");
			report.Check(ResolvedRuleCount(f, core) == 0, "All four shells again: the inactive rule is absent from the resolved output");
		}

		void RegressionChecks(Report& report, Application& application, const std::filesystem::path& root)
		{
			Fixture f{ report, application, root, "regression" };
			const auto ids = CaptureAbilities(f);
			const List<GameplayTag> primary{ GameplayTags::Ability::Primary };
			const List<GameplayTag> none{};
			const auto enemy = SpawnTarget(f);
			std::uint32_t regression = Add(f, kRegressionId, 1);
			PlaceFirstLegal(f, regression);
			report.CheckValue("PrimaryShotOnly and Always still stack on primary hits (1.2 x 1.1)", 1.32f,
				Ratio(f, regression, kRegressionId, *enemy, sas::ContentId{ "" }, primary), "primary");
			report.CheckValue("Always still applies to ability hits and PrimaryShotOnly stays off", 1.1f,
				Ratio(f, regression, kRegressionId, *enemy, ids[1], none), "ability");
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_static_damage_rules_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare static damage content registration failed" };
		std::string title = "Rare Static Damage Rules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-static-damage-fixtures";
		LoaderChecks(report, root);
		StrippedHullChecks(report, application, root);
		TripleCoreChecks(report, application, root);
		RegressionChecks(report, application, root);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareStaticDamageRulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
