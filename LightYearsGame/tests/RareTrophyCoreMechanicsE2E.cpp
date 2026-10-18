#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/tags/GameplayTags.h"
#include "player/Player.h"
#include "player/PlayerSpaceShip.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace rare_trophy_core_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		constexpr const char* kTrophyId = "Module.Offense.RareTrophyFixture.Rare";
		constexpr const char* kOdometerId = "Module.Movement.RareTrophyOdometer.Rare";
		constexpr const char* kBountyFeature = "Module.SideFeature.RareTrophyFixture.Bounty";
		constexpr const char* kFloorFeature = "Module.SideFeature.RareTrophyFixture.Floor";
		constexpr const char* kWallFeature = "Module.SideFeature.RareTrophyFixture.Wall";
		constexpr const char* kNoSlotAbility = "Fixture.Ability.NoSlot";
		constexpr float kPerKill[3] = { 0.004f, 0.0045f, 0.005f };
		// The two-cell (largest) variant. Side features apply only on the largest variant, so every trophy install uses it.
		constexpr int kTrophyVariant = 1;

		// Test-only definitions parsed from fixture JSON. They never enter shipped content.
		vega::List<vega::ModuleDefinition> gFixtureDefinitions;

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

		Json KillRule(int adjacentLimit = 4)
		{
			return Json{ { "source", "Kill" }, { "damagePerStack", Json::array({ 0.004, 0.0045, 0.005 }) },
				{ "killWeights", Json{ { "normal", 1 }, { "elite", 7.5 }, { "miniboss", 7.5 }, { "boss", 25 } } }, { "adjacentAbilityLimit", adjacentLimit } };
		}

		// Trophy Core: a one-cell and a two-cell Offense variant, both carrying the kill rule. A module with side features needs more
		// than one size variant, and the side features apply on the largest one (kTrophyVariant).
		Json TrophyModule(const Json& runCounterRules)
		{
			return Json{
				{ "id", kTrophyId }, { "displayName", "Trophy core fixture" }, { "description", "Trophy core fixture." }, { "rarity", "Rare" },
				{ "levelCount", 3 }, { "family", "Ability.Offense" }, { "sizeLadder", Json::array({ 1, 2 }) },
				{ "sizeVariants", Json::array({
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) }, { "runCounterRules", runCounterRules } },
					Json{ { "shape", Json::array({ Json::array({ 0, 0 }), Json::array({ 1, 0 }) }) }, { "runCounterRules", runCounterRules } } }) },
				{ "sideFeatures", Json::array({ kBountyFeature, kFloorFeature, kWallFeature }) }
			};
		}

		// Odometer (Distance) fixture, so the Distance regression runs beside the kill source.
		Json OdometerModule()
		{
			return Json{
				{ "id", kOdometerId }, { "displayName", "Trophy odometer fixture" }, { "description", "Trophy odometer fixture." }, { "rarity", "Rare" },
				{ "levelCount", 3 }, { "family", "Ability.Movement" }, { "sizeLadder", Json::array({ 1 }) },
				{ "sizeVariants", Json::array({ Json{ { "shape", Json::array({ Json::array({ 0, 0 }) }) },
					{ "runCounterRules", Json::array({ Json{ { "source", "Distance" }, { "unitsPerStack", 5000 },
						{ "damagePerStack", Json::array({ 0.01, 0.012, 0.014 }) }, { "moveSpeedPerStack", Json::array({ 0.01, 0.012, 0.014 }) } } }) } } }) }
			};
		}

		Json SideFeature(const char* id, const Json& rules)
		{
			return Json{ { "id", id }, { "displayName", "Trophy fixture side" }, { "description", "Trophy fixture side feature." }, { "levelCount", 3 }, { "rules", rules } };
		}

		// Bounty Mark fixture: 5, 4, 3, 2, then 1 percent per 25 stacks, floored at one second.
		Json BountyRules()
		{
			return Json{ { "killStackCooldownRules", Json::array({ Json{ { "stacksPerStep", 25 }, { "reductionPerStep", Json::array({ 0.05, 0.04, 0.03, 0.02 }) },
				{ "tailReduction", 0.01 }, { "minimumSeconds", 1 }, { "adjacentAbilityLimit", 4 } } }) } };
		}

		// Floor fixture: one stack removes 99 percent of the cooldown, so only the one second floor can keep it above zero.
		Json FloorRules()
		{
			return Json{ { "killStackCooldownRules", Json::array({ Json{ { "stacksPerStep", 1 }, { "reductionPerStep", Json::array({ 0.99 }) },
				{ "minimumSeconds", 1 }, { "adjacentAbilityLimit", 4 } } }) } };
		}

		Json WallRules()
		{
			return Json{ { "killStackDamageRules", Json::array({ Json{ { "family", "Ability.Offense" }, { "fraction", 0.5 } } }) } };
		}

		Json ModuleFixtureSet(const Json& shipped)
		{
			Json modules = shipped;
			modules["modules"].push_back(TrophyModule(Json::array({ KillRule() })));
			modules["modules"].push_back(OdometerModule());
			return modules;
		}

		Json SideFeatureFixtureSet(const Json& shipped)
		{
			Json features = shipped;
			features["sideFeatures"].push_back(SideFeature(kBountyFeature, BountyRules()));
			features["sideFeatures"].push_back(SideFeature(kFloorFeature, FloorRules()));
			features["sideFeatures"].push_back(SideFeature(kWallFeature, WallRules()));
			return features;
		}

		bool Near(float actual, float expected, float tolerance = 1e-4f)
		{
			return std::abs(actual - expected) <= tolerance;
		}

		bool Contains(const List<GameplayTag>& tags, const GameplayTag& tag)
		{
			return std::find(tags.begin(), tags.end(), tag) != tags.end();
		}

		// Pulls kill credit from the ship's applier into the fixture loadout, the way the Player wires it.
		void WireKillCredit(Fixture& f)
		{
			f.ship->GetModuleApplier().SetKillCreditSink([&f](std::uint32_t instanceId, double stacks, double damageBonus)
			{
				f.loadout.AddKillCredit(instanceId, stacks, damageBonus);
			});
		}

		// Installs id with an optional side feature on the first legal origin whose ability-shell neighbours include `system`.
		// Prefers an origin whose only ability neighbour is `system`, so other slots stay non-adjacent when possible.
		std::uint32_t InstallNextTo(Fixture& f, const char* id, const GameplayTag& system, int level = 1, const char* side = "")
		{
			const auto* definition = f.rules.lookup(id);
			if (!definition) throw std::runtime_error{ std::string{ "Trophy fixture is missing: " } + id };
			const std::uint32_t instance = f.loadout.AddToInventory(f.rules, *definition, kTrophyVariant, definition->rarity, level, sas::ContentId{ side });
			if (instance == 0) throw std::runtime_error{ "Trophy fixture inventory rejected the module" };
			const ModulePlacement item = *f.loadout.FindInventoryItem(instance);
			for (const bool strict : { true, false })
				for (int rotation = 0; rotation < 4; ++rotation)
					for (int y = 0; y < f.rules.schematic.height; ++y)
						for (int x = 0; x < f.rules.schematic.width; ++x)
						{
							ModulePlacement candidate = item;
							candidate.origin = sf::Vector2i{ x, y };
							candidate.rotationQuarterTurns = rotation;
							if (ModulePlacementRules::CanPlace(f.rules, f.loadout, candidate) != ModulePlacementFailure::None) continue;
							const List<GameplayTag> adjacent = ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), candidate, 4);
							if (!Contains(adjacent, system) || (strict && adjacent.size() != 1)) continue;
							if (f.loadout.Install(f.rules, instance, candidate.origin, rotation) != ModulePlacementFailure::None) continue;
							if (!f.ship->GetModuleApplier().Apply(f.loadout, f.rules)) continue;
							return instance;
						}
			throw std::runtime_error{ "No legal Trophy placement next to the requested ability shell" };
		}

		// Ability identity for an equipped loadout slot: id and tags, read from the live ship.
		struct SlotAbility
		{
			sas::ContentId id;
			List<GameplayTag> tags;
			bool present = false;
		};

		SlotAbility SlotAbilityOf(Fixture& f, sas::AbilitySlot slot)
		{
			SlotAbility result;
			const auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(slot);
			if (!ability) return result;
			result.id = sas::ContentId{ ability->GetDefinition().abilityId };
			for (const GameplayTag& tag : ability->GetDefinition().abilityTags) result.tags.push_back(tag);
			result.present = true;
			return result;
		}

		// First of the three slots that are not adjacent to the trophy, or None.
		sas::AbilitySlot FirstNonAdjacentSlot(Fixture& f, std::uint32_t trophy)
		{
			const ModulePlacement* placed = f.loadout.FindPlacement(trophy);
			if (!placed) return sas::AbilitySlot::None;
			const List<GameplayTag> adjacent = ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), *placed, 4);
			for (const sas::AbilitySlot slot : { sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
			{
				const GameplayTag system = slot == sas::AbilitySlot::Ability2 ? GameplayTags::Module::System::AbilitySlot2
					: slot == sas::AbilitySlot::Ability3 ? GameplayTags::Module::System::AbilitySlot3 : GameplayTags::Module::System::AbilitySlot4;
				if (!Contains(adjacent, system)) return slot;
			}
			return sas::AbilitySlot::None;
		}

		// Non-lethal hit from an ability to a fresh enemy. Returns the damage dealt.
		float AbilityHit(Fixture& f, const sas::ContentId& ability, const List<GameplayTag>& tags, float damage = 100.f)
		{
			const auto target = Enemy(f);
			return Hit(*target, f.ship.get(), damage, Plain(), ability, tags);
		}

		// Lethal hit from an ability to a fresh enemy of the given class.
		shared_ptr<SpaceShip> AbilityKill(Fixture& f, const sas::ContentId& ability, const List<GameplayTag>& tags, ControlTargetClass targetClass = ControlTargetClass::Normal, DamagePayload payload = Plain())
		{
			const auto target = Enemy(f);
			target->SetControlTargetClass(targetClass);
			Hit(*target, f.ship.get(), 100000.f, payload, ability, tags);
			return target;
		}

		float Ratio(float after, float before)
		{
			return before > 0.f ? after / before : 0.f;
		}

		// Damage ratio of the neighbour ability against its undamaged baseline; the baseline is measured before any credit.
		float NeighbourRatio(Fixture& f, const SlotAbility& neighbour, float baseline)
		{
			return Ratio(AbilityHit(f, neighbour.id, neighbour.tags), baseline);
		}

		void CoreChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophyId, GameplayTags::Module::System::AbilitySlot1);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const sas::AbilitySlot otherSlot = FirstNonAdjacentSlot(f, trophy);
			report.Check(neighbour.present && otherSlot != sas::AbilitySlot::None, "trophy fixture has an adjacent and a non-adjacent loadout ability");
			const SlotAbility other = SlotAbilityOf(f, otherSlot);

			const float baseline = AbilityHit(f, neighbour.id, neighbour.tags);
			report.Check(Contains(ModulePlacementRules::GetAdjacentAbilitySlotSystems(f.rules, f.loadout.GetPlacements(), *f.loadout.FindPlacement(trophy), 4), GameplayTags::Module::System::AbilitySlot1),
				"the trophy sits next to the Ability1 shell");
			report.Check(f.loadout.GetKillStacks(trophy) == 0.0 && f.loadout.GetKillDamageBonus(trophy) == 0.0 && Near(NeighbourRatio(f, neighbour, baseline), 1.f),
				"a fresh trophy starts with no kill credit and no damage bonus");

			// Kill credit: a neighbour normal kill adds one stack and the Level 1 ratio.
			AbilityKill(f, neighbour.id, neighbour.tags);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 1.f) && Near(static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), kPerKill[0]),
				"a neighbour normal kill adds 1 stack and +0.40% to the placement");
			report.Check(Near(NeighbourRatio(f, neighbour, baseline), 1.f + kPerKill[0], 1e-3f),
				"the neighbour's outgoing damage gains the permanent kill bonus");

			// Only the adjacent ability's kills count. Non-loadout sources and non-lethal hits do not.
			AbilityKill(f, other.id, other.tags);
			AbilityKill(f, sas::ContentId{ kNoSlotAbility }, {});
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 1.f), "kills by a non-adjacent ability or a non-loadout source add nothing");
			AbilityHit(f, neighbour.id, neighbour.tags);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 1.f), "a non-lethal neighbour hit adds nothing");

			// Module-generated damage that kills is not a kill for the trophy.
			DamagePayload generated = Plain();
			generated.skipModuleDamageRules = true;
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Normal, generated);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 1.f), "a module-generated kill is not counted");

			// Class weights: elite 7.5, miniboss 7.5 and boss 25 stacks per kill.
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Elite);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::MiniBoss);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 41.f) && Near(static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), 41.f * kPerKill[0], 1e-5f),
				"elite, miniboss and boss kills add 7.5, 7.5 and 25 stacks",
				Json{ { "stacks", f.loadout.GetKillStacks(trophy) }, { "bonus", f.loadout.GetKillDamageBonus(trophy) } });
			report.Check(Near(NeighbourRatio(f, neighbour, baseline), 1.f + 41.f * kPerKill[0], 2e-3f),
				"the neighbour's damage follows the accumulated ratio (1 + 41 x 0.40%)");

			// Sell and re-fit: the stacks stay with the module, the damage bonus follows the placement.
			const sf::Vector2i origin = f.loadout.FindPlacement(trophy)->origin;
			const int rotation = f.loadout.FindPlacement(trophy)->rotationQuarterTurns;
			const std::size_t installedHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			report.Check(f.loadout.Uninstall(trophy) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules),
				"the trophy uninstalls through the real applier");
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 41.f) && Near(static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), 41.f * kPerKill[0], 1e-5f) &&
				Near(NeighbourRatio(f, neighbour, baseline), 1.f, 1e-3f) && f.ship->GetModuleApplier().GetOwnedHandleCount() < installedHandles,
				"uninstalling removes the bonus from the neighbour and keeps the kill credit in the module");
			report.Check(f.loadout.Install(f.rules, trophy, origin, rotation) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) &&
				Near(NeighbourRatio(f, neighbour, baseline), 1.f + 41.f * kPerKill[0], 2e-3f) && f.ship->GetModuleApplier().GetOwnedHandleCount() == installedHandles,
				"reinstalling at the same origin restores the bonus at once with the kept credit");

			bool cyclesOk = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				cyclesOk = f.loadout.Uninstall(trophy) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && cyclesOk;
				cyclesOk = f.loadout.Install(f.rules, trophy, origin, rotation) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && cyclesOk;
			}
			report.Check(cyclesOk && f.ship->GetModuleApplier().GetOwnedHandleCount() == installedHandles && Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 41.f),
				"ten uninstall and reinstall cycles keep the handle count and the kill credit");

			// Moving the trophy keeps its credit; the bonus then follows whatever ability it now neighbours.
			bool moved = false;
			for (int y = 0; y < f.rules.schematic.height && !moved; ++y)
				for (int x = 0; x < f.rules.schematic.width && !moved; ++x)
				{
					if (x == origin.x && y == origin.y) continue;
					moved = f.loadout.Move(f.rules, trophy, sf::Vector2i{ x, y }, rotation) == ModulePlacementFailure::None;
				}
			report.Check(moved && f.ship->GetModuleApplier().Apply(f.loadout, f.rules) && Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 41.f) &&
				Near(static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), 41.f * kPerKill[0], 1e-5f),
				"moving the trophy keeps its kill credit");
			report.Check(f.loadout.Move(f.rules, trophy, origin, rotation) == ModulePlacementFailure::None && f.ship->GetModuleApplier().Apply(f.loadout, f.rules),
				"the trophy moves back to its neighbour");
		}

		void LevelChecks(Report& report, Application& application, int level)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophyId, GameplayTags::Module::System::AbilitySlot1, level);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const float baseline = AbilityHit(f, neighbour.id, neighbour.tags);
			AbilityKill(f, neighbour.id, neighbour.tags);
			report.Check(Near(static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), kPerKill[level - 1], 1e-5f) &&
				Near(NeighbourRatio(f, neighbour, baseline), 1.f + kPerKill[level - 1], 1e-3f),
				"level " + std::to_string(level) + " uses its own per-kill ratio",
				Json{ { "bonus", f.loadout.GetKillDamageBonus(trophy) }, { "expected", kPerKill[level - 1] } });
		}

		void OdometerChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophyId, GameplayTags::Module::System::AbilitySlot1);
			const std::uint32_t odometer = f.Install(kOdometerId);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const SlotAbility other = SlotAbilityOf(f, FirstNonAdjacentSlot(f, trophy));
			const float baseline = AbilityHit(f, neighbour.id, neighbour.tags);
			const float baselineOther = AbilityHit(f, other.id, other.tags);

			AbilityKill(f, neighbour.id, neighbour.tags);
			f.loadout.AccrueRunCounters(f.rules, 5000.0);
			f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
			report.Check(f.loadout.GetRunCounter(odometer) == 5000.0 && f.loadout.GetRunCounter(trophy) == 0.0,
				"distance accrues to the odometer only; the trophy keeps a zero distance counter");
			report.Check(Near(NeighbourRatio(f, neighbour, baseline), (1.f + 0.01f) * (1.f + kPerKill[0]), 2e-3f),
				"the neighbour's damage multiplies the odometer stack with the kill bonus");
			report.Check(Near(Ratio(AbilityHit(f, other.id, other.tags), baselineOther), 1.01f, 1e-3f),
				"the odometer's stack reaches a non-adjacent ability while the kill bonus does not");
		}

		void BountyChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophyId, GameplayTags::Module::System::AbilitySlot1, 1, kBountyFeature);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const sas::AbilitySlot otherSlot = FirstNonAdjacentSlot(f, trophy);
			const SlotAbility other = SlotAbilityOf(f, otherSlot);
			const auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			const auto* otherAbility = f.ship->GetAbilitySystemComponent().GetAbility(otherSlot);
			report.Check(ability != nullptr && otherAbility != nullptr, "bounty fixture reads both abilities from the live ship");
			if (!ability || !otherAbility) return;
			const float before = f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(ability->GetDefinition());
			const float beforeOther = f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(otherAbility->GetDefinition());

			// Stacks: one boss kill is 25 stacks (one step, 5 percent), so the steps are 25, 50, 125 and 150.
			const auto stepMultiplier = [&f, ability, before]() { return Ratio(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(ability->GetDefinition()), before); };
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 25.f) && Near(stepMultiplier(), 0.95f, 1e-3f),
				"25 stacks cut the neighbour's cooldown by 5 percent",
				Json{ { "stacks", f.loadout.GetKillStacks(trophy) }, { "multiplier", stepMultiplier() } });
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(stepMultiplier(), 0.91f, 1e-3f), "50 stacks cut 9 percent (5 + 4, additive)");
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 125.f) && Near(stepMultiplier(), 0.85f, 1e-3f),
				"125 stacks cut 15 percent (5 + 4 + 3 + 2 + 1)");
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(stepMultiplier(), 0.84f, 1e-3f), "after the list, every further step cuts a flat 1 percent");

			// Only the neighbour is scoped; a non-adjacent ability is untouched.
			report.Check(Near(Ratio(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(otherAbility->GetDefinition()), beforeOther), 1.f, 1e-4f),
				"the bounty does not reach a non-adjacent ability");
			report.Check(Near(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMinimum(ability->GetDefinition()), 1.f, 1e-6f),
				"the neighbour's cooldown has an absolute one second floor");
			AbilityKill(f, other.id, other.tags, ControlTargetClass::Boss);
			report.Check(Near(stepMultiplier(), 0.84f, 1e-3f), "kills by a non-adjacent ability do not move the bounty");
		}

		void FloorChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophyId, GameplayTags::Module::System::AbilitySlot1, 1, kFloorFeature);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			report.Check(ability != nullptr, "floor fixture reads the neighbour ability");
			if (!ability) return;
			const float before = f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(ability->GetDefinition());
			AbilityKill(f, neighbour.id, neighbour.tags);
			const float after = f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(ability->GetDefinition());
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 1.f) && Near(Ratio(after, before), 0.01f, 1e-3f),
				"one stack removes 99 percent of the cooldown, leaving the floor to decide the result",
				Json{ { "multiplier", Ratio(after, before) } });
			report.Check(Near(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMinimum(ability->GetDefinition()), 1.f, 1e-6f),
				"the floor fixture carries the one second minimum");
		}

		void WallChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.rules.lookup = FixtureLookup;
			WireKillCredit(f);
			InstallNextTo(f, kTrophyId, GameplayTags::Module::System::AbilitySlot1, 1, kWallFeature);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const sas::ContentId offense{ "Fixture.Ability.Offense" };
			const sas::ContentId defense{ "Fixture.Ability.Defense" };
			const List<GameplayTag> offenseTags{ GameplayTags::Ability::Offense };
			const List<GameplayTag> defenseTags{ GameplayTags::Ability::Defense };
			const float offenseBefore = AbilityHit(f, offense, offenseTags);
			const float defenseBefore = AbilityHit(f, defense, defenseTags);
			AbilityKill(f, neighbour.id, neighbour.tags);
			AbilityKill(f, neighbour.id, neighbour.tags);
			report.Check(Near(Ratio(AbilityHit(f, offense, offenseTags), offenseBefore), 1.f + 0.5f * 2.f * kPerKill[0], 2e-3f),
				"Trophy Wall gives every Offense ability half of the stacked bonus, from an ability outside the neighbour set");
			report.Check(Near(Ratio(AbilityHit(f, defense, defenseTags), defenseBefore), 1.f, 1e-3f),
				"Trophy Wall does not reach a non-Offense ability");
		}

		void PlayerChecks(Report& report, Application& application)
		{
			auto level = std::make_shared<HeadlessModuleLevel>(&application);
			level->BeginPlayInternal();
			Player player;
			shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(ship != nullptr, "the trophy player spawns a real ship through Player::SpawnSpaceShip");
			level->TickInternal(0.f);
			ship->SetUseScreenClamp(false);
			const std::optional<ModuleRulesContext> rules = player.BuildModuleRules();
			report.Check(rules.has_value(), "the player ship has a module schematic");
			if (!rules) return;

			const std::uint32_t trophy = player.AddModuleToInventory(kTrophyId, kTrophyVariant, ModuleRarity::Rare, 1);
			bool installed = false;
			for (int rotation = 0; rotation < 4 && !installed; ++rotation)
				for (int y = 0; y < rules->schematic.height && !installed; ++y)
					for (int x = 0; x < rules->schematic.width && !installed; ++x)
					{
						ModulePlacement candidate = *player.GetModuleLoadout().FindInventoryItem(trophy);
						candidate.origin = sf::Vector2i{ x, y };
						candidate.rotationQuarterTurns = rotation;
						if (ModulePlacementRules::CanPlace(*rules, player.GetModuleLoadout(), candidate) != ModulePlacementFailure::None) continue;
						const List<GameplayTag> adjacent = ModulePlacementRules::GetAdjacentAbilitySlotSystems(*rules, player.GetModuleLoadout().GetPlacements(), candidate, 4);
						if (!Contains(adjacent, GameplayTags::Module::System::AbilitySlot1)) continue;
						installed = player.TryInstallModule(trophy, candidate.origin, rotation) == ModulePlacementFailure::None;
					}
			report.Check(installed, "the trophy installs through the Player next to the Ability1 shell");
			if (!installed) return;

			const auto* ability = ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			report.Check(ability != nullptr, "the player ship has an Ability1 loadout ability");
			if (!ability) return;
			const sas::ContentId abilityId{ ability->GetDefinition().abilityId };
			List<GameplayTag> tags;
			for (const GameplayTag& tag : ability->GetDefinition().abilityTags) tags.push_back(tag);

			const auto target = level->SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			level->TickInternal(0.f);
			target->SetCollisionLayer(vega::GameCollision::Enemy);
			target->SetCollisionMask(vega::CollisionLayer::None);
			target->SetEnablePhysics(false);
			target->SetActorLocation(ship->GetActorLocation() + sf::Vector2f{ 50.f, 0.f });
			target->GetShieldComponent().ChangeShield(-10000.f);
			target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			target->GetHealthComponent().SetMaxHealth(1000.f);
			target->GetHealthComponent().Regenerate(1000.f);
			Hit(*target, ship.get(), 100000.f, Plain(), abilityId, tags);
			report.Check(target->GetHealthComponent().GetHealth() == 0.f && Near(static_cast<float>(player.GetModuleLoadout().GetKillStacks(trophy)), 1.f) &&
				Near(static_cast<float>(player.GetModuleLoadout().GetKillDamageBonus(trophy)), kPerKill[0], 1e-5f),
				"a live Player kill reaches the loadout through the applier's kill credit sink");

			for (int attempt = 0; attempt < 3; ++attempt) player.SpawnSpaceShip(level.get());
			report.Check(player.GetModuleLoadout().FindPlacement(trophy) == nullptr && player.GetModuleLoadout().GetKillStacks(trophy) == 0.0 &&
				player.GetModuleLoadout().GetKillDamageBonus(trophy) == 0.0,
				"a new run resets the kill credit with the loadout");
		}

		void LoaderChecks(Report& report, const std::filesystem::path& root, const Json& shipped, const Json& shippedSideFeatures)
		{
			const auto reject = [&](const char* name, const Json& runCounterRules)
			{
				Json modules = shipped;
				modules["modules"].push_back(TrophyModule(runCounterRules));
				const auto path = WriteJson(root, std::string{ "reject-" } + name + ".json", modules);
				const auto loaded = content::ModuleLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "Kill loader rejects " } + name, loaded.error);
			};
			Json withoutWeights = KillRule();
			withoutWeights.erase("killWeights");
			reject("missing-killWeights", Json::array({ withoutWeights }));
			Json missingBoss = KillRule();
			missingBoss["killWeights"].erase("boss");
			reject("missing-boss-weight", Json::array({ missingBoss }));
			Json withUnits = KillRule();
			withUnits["unitsPerStack"] = 5000;
			reject("kill-with-units", Json::array({ withUnits }));
			reject("adjacency-limit-five", Json::array({ KillRule(5) }));
			reject("adjacency-limit-zero", Json::array({ KillRule(0) }));
			Json unknownWeight = KillRule();
			unknownWeight["killWeights"]["champion"] = 3;
			reject("unknown-weight-class", Json::array({ unknownWeight }));
			Json distanceWithWeights{ { "source", "Distance" }, { "unitsPerStack", 5000 }, { "damagePerStack", 0.01 }, { "killWeights", Json{ { "normal", 1 }, { "elite", 1 }, { "miniboss", 1 }, { "boss", 1 } } } };
			reject("distance-with-kill-weights", Json::array({ distanceWithWeights }));

			const auto rejectSide = [&](const char* name, const Json& feature)
			{
				Json features = shippedSideFeatures;
				features["sideFeatures"].push_back(feature);
				const auto path = WriteJson(root, std::string{ "reject-side-" } + name + ".json", features);
				const auto loaded = content::ModuleSideFeatureLoader::LoadFromFile(path);
				report.Check(!loaded.Succeeded(), std::string{ "Bounty loader rejects " } + name, loaded.error);
			};
			rejectSide("empty-steps", SideFeature("Module.SideFeature.RareTrophyFixture.BadSteps", Json{ { "killStackCooldownRules", Json::array({ Json{ { "stacksPerStep", 25 },
				{ "reductionPerStep", Json::array() }, { "minimumSeconds", 1 } } }) } }));
			rejectSide("full-reduction", SideFeature("Module.SideFeature.RareTrophyFixture.BadFull", Json{ { "killStackCooldownRules", Json::array({ Json{ { "stacksPerStep", 25 },
				{ "reductionPerStep", Json::array({ 1.0 }) }, { "minimumSeconds", 1 } } }) } }));
			rejectSide("zero-floor", SideFeature("Module.SideFeature.RareTrophyFixture.BadFloor", Json{ { "killStackCooldownRules", Json::array({ Json{ { "stacksPerStep", 25 },
				{ "reductionPerStep", Json::array({ 0.05 }) }, { "minimumSeconds", 0 } } }) } }));
			rejectSide("wall-fraction-zero", SideFeature("Module.SideFeature.RareTrophyFixture.BadWall", Json{ { "killStackDamageRules", Json::array({ Json{ { "family", "Ability.Offense" }, { "fraction", 0 } } }) } }));
		}
	}
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace rare_trophy_core_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare trophy core content registration failed" };
		std::string title = "Rare Trophy Core E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);

		const std::filesystem::path root = std::filesystem::path{ argv[1] }.parent_path() / "rare-trophy-core-fixtures";
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
		const auto modulesPath = WriteJson(root, "rare-trophy-core-modules.json", ModuleFixtureSet(shippedModules));
		const auto sideFeaturesPath = WriteJson(root, "rare-trophy-core-side-features.json", SideFeatureFixtureSet(shippedSideFeatures));
		const auto loaded = content::ModuleLoader::LoadFromFile(modulesPath);
		report.Check(loaded.Succeeded(), "trophy fixture modules load through the real ModuleLoader", loaded.error);
		gFixtureDefinitions = loaded.definitions;
		std::string catalogFailure;
		report.Check(content::ModuleContentCatalog::LoadFromFiles(modulesPath, sideFeaturesPath, &catalogFailure),
			"the player's content catalog reads the same trophy fixtures", catalogFailure);

		CoreChecks(report, application);
		LevelChecks(report, application, 2);
		LevelChecks(report, application, 3);
		OdometerChecks(report, application);
		BountyChecks(report, application);
		FloorChecks(report, application);
		WallChecks(report, application);
		PlayerChecks(report, application);
		LoaderChecks(report, root, shippedModules, shippedSideFeatures);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareTrophyCoreMechanicsE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
