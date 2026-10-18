#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"

#include "gameplay/MovementComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"
#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace rare_wave3_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		// Shipped content only. Nothing here is copied, reduced or re-parsed from a fixture file.
		constexpr const char* kCore = "Module.Utility.TripleCore.Rare";
		constexpr const char* kHull = "Module.Utility.StrippedHull.Rare";
		constexpr const char* kOmni = "Module.Movement.OmniThrusters.Uncommon";
		constexpr const char* kOdometer = "Module.Movement.Odometer.Rare";
		// Shipped one-cell Common module without a damage rule; installing it changes the empty-cell count only.
		constexpr const char* kFiller = "Module.Utility.TargetingComputer.Basic";
		// Shipped Uncommon Utility module, used as the regression probe for an unchanged Uncommon module.
		constexpr const char* kUncommon = "Module.Utility.DoubleFeedCore.Uncommon";

		constexpr std::array<float, 3> kCoreMultiplier{ 1.25f, 1.30f, 1.35f };
		constexpr std::array<float, 3> kHullPerCell{ 0.025f, 0.03f, 0.035f };
		constexpr std::array<float, 3> kOmniMultiplier{ 0.70f, 0.715f, 0.73f };
		constexpr std::array<float, 3> kOdometerPerStack{ 0.01f, 0.012f, 0.014f };
		constexpr float kTolerance = 0.5f;
		constexpr float kProbeDamage = 100.f;
		constexpr float kDriveSpeed = 400.f;
		constexpr float kDriveStep = 0.1f;

		sas::ContentId ContentIdOf(const char* id)
		{
			return sas::ContentId{ std::string{ id } };
		}

		// Level value of the first attribute modifier of a shipped module, read from its own JSON-loaded definition.
		float BaseMagnitude(const ModuleDefinition& definition, int level)
		{
			const ModuleAttributeModifier& entry = definition.sizeVariants[0].attributeModifiers[0];
			return entry.magnitudeByLevel.ValueAtLevel(entry.modifier.magnitude, level);
		}

		sas::AttributeId FirstAttribute(const ModuleDefinition& definition)
		{
			return definition.sizeVariants[0].attributeModifiers[0].modifier.attributeId;
		}

		bool Near(float left, float right, float tolerance = 1e-4f)
		{
			return std::abs(left - right) <= tolerance;
		}

		// Thrust the movement component resolves right now (ship bonus and module bonus applied).
		struct Thrust
		{
			float forward = 0.f;
			float reverse = 0.f;
			float strafe = 0.f;
		};

		Json ThrustJson(const Thrust& thrust)
		{
			return Json{ { "forward", thrust.forward }, { "reverse", thrust.reverse }, { "strafe", thrust.strafe } };
		}

		bool SameThrust(const Thrust& left, const Thrust& right, float tolerance = kTolerance)
		{
			return Near(left.forward, right.forward, tolerance) && Near(left.reverse, right.reverse, tolerance) && Near(left.strafe, right.strafe, tolerance);
		}

		Thrust Resolved(const Fixture& f)
		{
			const MovementComponent& movement = f.ship->GetMovementComponent();
			return Thrust{ movement.ResolveForwardThrust(), movement.ResolveReverseThrust(), movement.ResolveStrafeThrust() };
		}

		// Raw movement attributes before any thrust multiplier or equalization.
		Thrust Attributes(const Fixture& f)
		{
			const ShipMovementAttributes& attributes = f.ship->GetMovementComponent().GetAttributes();
			return Thrust{ attributes.forwardThrust.currentValue, attributes.reverseThrust.currentValue, attributes.strafeThrust.currentValue };
		}

		// Expected thrust: the ship's own thrust bonus plus moduleBonus, applied to the raw attributes; reverse and strafe take the forward value when equalize is set.
		Thrust Expected(const Fixture& f, const Thrust& base, float moduleBonus, bool equalize)
		{
			const float multiplier = std::max(0.f, 1.f + f.ship->GetRuntimeModifiers().GetThrustBonus() + moduleBonus);
			const Thrust scaled{ base.forward * multiplier, base.reverse * multiplier, base.strafe * multiplier };
			return equalize ? Thrust{ scaled.forward, scaled.forward, scaled.forward } : scaled;
		}

		// Uninstalls through the real loadout and reapplies the layout. The instance keeps its cell for Unpark.
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

		// One real hit on a target whose health is restored first, so every measurement starts from full health.
		float Dealt(Fixture& f, Actor& target, const sas::ContentId& ability, const List<GameplayTag>& tags)
		{
			HealthComponent* health = target.FindComponent<HealthComponent>();
			health->Regenerate(health->GetMaxHealth());
			return Hit(target, f.ship.get(), kProbeDamage, Plain(), ability, tags);
		}

		// Ratio of one real hit with the module installed to the same hit with it parked, in the shell state that is current.
		float Ratio(Fixture& f, std::uint32_t instance, Actor& target, const sas::ContentId& ability, const List<GameplayTag>& tags)
		{
			const float withModule = Dealt(f, target, ability, tags);
			const Parked parked = Park(f, instance);
			const float withoutModule = Dealt(f, target, ability, tags);
			Unpark(f, parked);
			f.report.Check(withoutModule > 0.f, "Baseline hit without the module deals real health damage", Json{ { "withoutModule", withoutModule } });
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

		void Accrue(Fixture& f, double units)
		{
			f.loadout.AccrueRunCounters(f.rules, units);
			f.ship->GetModuleApplier().Apply(f.loadout, f.rules);
		}

		float Multiplier(const SpaceShip& ship)
		{
			return ship.GetCombatRuntime().GetOutgoingDamageMultiplier();
		}

		float SpeedMultiplier(const SpaceShip& ship)
		{
			return ship.GetRuntimeModifiers().GetMovementSpeedMultiplier();
		}

		// One tick at a commanded speed. Returns the distance the ship covered, from its velocity after the tick.
		double Step(HeadlessModuleLevel& level, PlayerSpaceShip& ship, float speed)
		{
			ship.SetVelocity({ speed, 0.f });
			level.TickInternal(kDriveStep);
			const sf::Vector2f velocity = ship.GetVelocity();
			return std::sqrt(static_cast<double>(velocity.x) * velocity.x + static_cast<double>(velocity.y) * velocity.y) * kDriveStep;
		}

		// Installs through the Player's own mutation path, which applies to the live ship.
		bool InstallOnPlayer(Player& player, const ModuleRulesContext& rules, std::uint32_t id)
		{
			for (int rotation = 0; rotation < 4; ++rotation)
				for (int y = 0; y < rules.schematic.height; ++y)
					for (int x = 0; x < rules.schematic.width; ++x)
						if (player.TryInstallModule(id, sf::Vector2i{ x, y }, rotation) == ModulePlacementFailure::None) return true;
			return false;
		}

		// Instance id of the Rare module the test grant placed in the Player's inventory, or 0.
		std::uint32_t GrantedInstance(const Player& player, const char* moduleId)
		{
			for (const ModulePlacement& item : player.GetModuleLoadout().GetInventory())
				if (item.moduleId.ToString() == moduleId) return item.instanceId;
			return 0;
		}

		// A hittable target far enough from the drive path that it never shares a cell with the ship.
		shared_ptr<SpaceShip> SpawnDummy(HeadlessModuleLevel& level, PlayerSpaceShip& ship)
		{
			const auto dummy = level.SpawnActor<SpaceShip>(content::ShipContentCatalog::GetPlayerFighterDefinition()).lock();
			level.TickInternal(0.f);
			dummy->SetCollisionLayer(vega::GameCollision::Enemy);
			dummy->SetCollisionMask(CollisionLayer::None);
			dummy->SetEnablePhysics(false);
			dummy->SetActorLocation(ship.GetActorLocation() + sf::Vector2f{ 0.f, 2000.f });
			dummy->GetShieldComponent().ChangeShield(-10000.f);
			dummy->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 0.f);
			dummy->GetHealthComponent().SetMaxHealth(1000.f);
			dummy->GetHealthComponent().Regenerate(1000.f);
			return dummy;
		}

		// The dummy ticks during the drive, so its shield regenerates; clear it too or the hit is absorbed by the shield.
		float RegenHit(SpaceShip& target, Actor* source)
		{
			auto& shield = target.GetShieldComponent();
			shield.ChangeShield(-shield.GetShield());
			auto& health = target.GetHealthComponent();
			health.Regenerate(health.GetMaxHealth());
			return Hit(target, source, kProbeDamage, Plain());
		}

		void CatalogChecks(Report& report)
		{
			const auto loaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(loaded.Succeeded(), "shipped modules.json loads through the real ModuleLoader", loaded.error);

			struct Subject { const char* id; const char* family; std::size_t cells; };
			const Subject subjects[] = {
				{ kCore, "Ability.Utility", 2 },
				{ kHull, "Ability.Utility", 1 },
				{ kOmni, "Ability.Movement", 2 },
				{ kOdometer, "Ability.Movement", 1 },
			};
			for (const Subject& subject : subjects)
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(ContentIdOf(subject.id));
				report.Check(definition != nullptr, std::string{ "catalog contains " } + subject.id);
				const ModuleRarity rarity = std::string{ subject.id } == kOmni ? ModuleRarity::Uncommon : ModuleRarity::Rare;
				report.Check(definition->rarity == rarity && definition->levelCount == 3 && definition->family.name == subject.family &&
					definition->sizeVariants.size() == 1, std::string{ "current rarity, three levels, one size variant: " } + subject.id,
					Json{ { "levelCount", definition->levelCount }, { "variants", definition->sizeVariants.size() } });
				report.Check(definition->sizeVariants[0].shape.cells.size() == subject.cells, std::string{ "cell count matches the design: " } + subject.id,
					Json{ { "cells", definition->sizeVariants[0].shape.cells.size() } });
				report.Check(definition->sideFeatures.empty() && definition->sharedPools.empty() && definition->conflictsWith.empty(),
					std::string{ "single-size module has no side features, shared pools or conflicts: " } + subject.id);
				for (int level = 1; level <= 3; ++level)
					report.Check(HasModuleRulePackageLevelCoverage(definition->sizeVariants[0], level),
						std::string{ "level table covers level " } + std::to_string(level) + ": " + subject.id);
				report.Check(!HasModuleRulePackageLevelCoverage(definition->sizeVariants[0], 4), std::string{ "no level beyond three: " } + subject.id);
			}
		}

		void DescriptionChecks(Report& report)
		{
			struct Text { const char* id; std::array<const char*, 3> expected; };
			const Text texts[] = {
				{ kCore, { "x1,25.", "x1,3.", "x1,35." } },
				{ kHull, { "+2,5% damage", "+3% damage", "+3,5% damage" } },
				{ kOmni, { "-30%.", "-28,5%.", "-27%." } },
				{ kOdometer, { "+1% damage", "+1,2% damage", "+1,4% damage" } },
			};
			for (const Text& text : texts)
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(ContentIdOf(text.id));
				report.Check(definition != nullptr, std::string{ "catalog contains " } + text.id);
				for (int level = 1; level <= 3; ++level)
				{
					const std::string formatted = FormatModuleDescription(*definition, definition->sizeVariants[0], level);
					const std::string label = std::string{ "description level " } + std::to_string(level) + " is fully formatted: " + text.id;
					report.Check(formatted.find('{') == std::string::npos && formatted.find('}') == std::string::npos, label, Json{ { "text", formatted } });
					report.Check(formatted.find(text.expected[level - 1]) != std::string::npos,
						std::string{ "description shows the level " } + std::to_string(level) + " value: " + text.id, Json{ { "text", formatted } });
				}
			}
		}

		void TripleCoreChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::array<sas::ContentId, 4> ids = CaptureAbilities(f);
			const List<GameplayTag> primary{ GameplayTags::Ability::Primary };
			const List<GameplayTag> none{};
			const auto enemy = Enemy(f);

			std::array<std::uint32_t, 4> shells{};
			for (int index = 0; index < 4; ++index)
			{
				shells[static_cast<std::size_t>(index)] = FindShell(f, index);
				report.Check(shells[static_cast<std::size_t>(index)] != 0, "Default loadout installs every ability-slot shell", Json{ { "shellIndex", index + 1 } });
			}

			std::uint32_t core = f.Install(kCore, 0, 1);
			for (std::size_t slot = 1; slot < 4; ++slot)
				report.CheckValue("four shells installed: Triple Core is inert on ability " + std::to_string(slot + 1), 1.f,
					Ratio(f, core, *enemy, ids[slot], none), "four shells, level 1");
			report.Check(ResolvedRuleCount(f, core) == 0, "four shells: the inactive rule is absent from the resolved output",
				Json{ { "rules", ResolvedRuleCount(f, core) } });

			const Parked shell1 = Park(f, shells[0]);
			for (std::size_t slot = 1; slot < 4; ++slot)
				report.CheckValue("one empty slot: a direct hit of the other Q/E/F/R ability is multiplied by 1.25 (ability " + std::to_string(slot + 1) + ")",
					kCoreMultiplier[0], Ratio(f, core, *enemy, ids[slot], none), "one empty slot, level 1");
			report.CheckValue("one empty slot: Primary hits are not multiplied", 1.f, Ratio(f, core, *enemy, sas::ContentId{ "" }, primary), "one empty slot, primary");
			report.Check(ResolvedRuleCount(f, core) == 1 && std::abs(ResolvedMultiplier(f, core) - kCoreMultiplier[0]) < 1e-6f,
				"one empty slot: the applier output carries exactly the level 1 multiplier", Json{ { "rules", ResolvedRuleCount(f, core) } });

			const Parked shell2 = Park(f, shells[1]);
			report.CheckValue("two empty slots: Triple Core is inert on ability 3", 1.f, Ratio(f, core, *enemy, ids[2], none), "two empty slots");
			report.CheckValue("two empty slots: Triple Core is inert on ability 4", 1.f, Ratio(f, core, *enemy, ids[3], none), "two empty slots");
			report.Check(ResolvedRuleCount(f, core) == 0, "two empty slots: the inactive rule is absent from the resolved output");

			Unpark(f, shell2);
			report.CheckValue("back to one empty slot: the multiplier returns on ability 3", kCoreMultiplier[0], Ratio(f, core, *enemy, ids[2], none), "one empty slot again");
			Unpark(f, shell1);
			report.CheckValue("all four shells again: Triple Core is inert", 1.f, Ratio(f, core, *enemy, ids[1], none), "four shells again");

			for (int level = 2; level <= 3; ++level)
			{
				f.Remove(core);
				core = f.Install(kCore, 0, level);
				report.CheckValue("four shells at level " + std::to_string(level) + ": still inert", 1.f, Ratio(f, core, *enemy, ids[1], none), "four shells");
				const Parked parked = Park(f, shells[0]);
				report.CheckValue("one empty slot at level " + std::to_string(level) + " uses its own table entry", kCoreMultiplier[static_cast<std::size_t>(level - 1)],
					Ratio(f, core, *enemy, ids[1], none), "level table");
				report.CheckValue("one empty slot at level " + std::to_string(level) + " keeps Primary at 1", 1.f,
					Ratio(f, core, *enemy, sas::ContentId{ "" }, primary), "primary");
				Unpark(f, parked);
			}
			f.Remove(core);
		}

		void StrippedHullChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::array<sas::ContentId, 4> ids = CaptureAbilities(f);
			const List<GameplayTag> primary{ GameplayTags::Ability::Primary };
			const List<GameplayTag> none{};
			const auto enemy = Enemy(f);

			for (int level = 1; level <= 3; ++level)
			{
				const std::uint32_t hull = f.Install(kHull, 0, level);
				const int empty = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
				report.Check(empty > 0, "Stripped Hull has empty cells to count", Json{ { "empty", empty } });
				const float expected = 1.f + kHullPerCell[static_cast<std::size_t>(level - 1)] * static_cast<float>(empty);
				const std::string at = "Stripped Hull level " + std::to_string(level);
				report.CheckValue(at + " multiplies primary hits by 1 + perCell x empty cells", expected,
					Ratio(f, hull, *enemy, sas::ContentId{ "" }, primary), at + " primary");
				report.CheckValue(at + " multiplies Q/E/F/R hits by the same factor", expected,
					Ratio(f, hull, *enemy, ids[0], none), at + " ability");
				report.CheckValue(at + " resolves the same factor in the applier output", expected,
					ResolvedMultiplier(f, hull), at + " resolved");
				f.Remove(hull);
			}

			const std::uint32_t hull = f.Install(kHull, 0, 1);
			const int emptyBefore = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			const std::uint32_t filler = f.Install(kFiller, 0, 1);
			const int emptyWith = ModuleResolver::CountEmptyCells(f.rules, f.loadout.GetPlacements());
			report.Check(emptyWith == emptyBefore - 1, "installing a one-cell module removes exactly one empty cell",
				Json{ { "before", emptyBefore }, { "after", emptyWith } });
			report.CheckValue("a second module lowers the Stripped Hull factor by one cell", 1.f + kHullPerCell[0] * static_cast<float>(emptyWith),
				Ratio(f, hull, *enemy, ids[0], none), "filler installed");
			report.CheckValue("the applier output follows the installed layout", 1.f + kHullPerCell[0] * static_cast<float>(emptyWith),
				ResolvedMultiplier(f, hull), "filler installed, resolved");

			f.Remove(filler);
			report.CheckValue("removing the filler restores the Stripped Hull factor", 1.f + kHullPerCell[0] * static_cast<float>(emptyBefore),
				Ratio(f, hull, *enemy, ids[0], none), "filler removed");
			f.Remove(hull);
		}

		void OmniChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			MovementComponent& movement = f.ship->GetMovementComponent();
			report.Check(!movement.HasModuleDirectionalThrustSync() && movement.GetModuleThrustBonus() == 0.f,
				"precondition: the default Fighter loadout carries no module movement rule");
			const Thrust raw = Attributes(f);
			const Thrust bare = Resolved(f);
			const float shipBonus = f.ship->GetRuntimeModifiers().GetThrustBonus();

			for (int level = 1; level <= 3; ++level)
			{
				const std::uint32_t omni = f.Install(kOmni, 0, level);
				const float multiplier = kOmniMultiplier[static_cast<std::size_t>(level - 1)];
				const Thrust resolved = Resolved(f);
				const std::string label = "Omni Thrusters level " + std::to_string(level);
				report.CheckValue(label + " sets the forward thrust multiplier", multiplier - 1.f, movement.GetModuleThrustBonus(),
					"thrustMultiplier table through the real applier");
				report.Check(movement.HasModuleDirectionalThrustSync(), label + " registers module equalization");
				const Thrust expected = Expected(f, raw, multiplier - 1.f, true);
				report.Check(SameThrust(resolved, expected) && Near(resolved.reverse, resolved.forward, kTolerance) && Near(resolved.strafe, resolved.forward, kTolerance),
					label + " makes forward, reverse and strafe thrust equal", Json{ { "resolved", ThrustJson(resolved) }, { "expected", ThrustJson(expected) } });
				report.CheckValue(label + " forward thrust is the bare forward thrust times the level multiplier",
					(1.f + shipBonus + multiplier - 1.f) / (1.f + shipBonus), resolved.forward / bare.forward, "ratio to bare forward thrust");
				f.Remove(omni);
				const Thrust restored = Resolved(f);
				report.Check(SameThrust(restored, bare, 1e-3f) && movement.GetModuleThrustBonus() == 0.f && !movement.HasModuleDirectionalThrustSync(),
					label + " removal restores the bare thrust exactly", Json{ { "resolved", ThrustJson(restored) } });
			}

			const std::size_t handlesBefore = f.ship->GetModuleApplier().GetOwnedHandleCount();
			bool cyclesStable = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				const std::uint32_t omni = f.Install(kOmni, 0, 1 + cycle % 3);
				f.Remove(omni);
				cyclesStable = cyclesStable && f.ship->GetModuleApplier().GetOwnedHandleCount() == handlesBefore && SameThrust(Resolved(f), bare, 1e-3f);
			}
			report.Check(cyclesStable, "ten Omni Thrusters install and remove cycles keep handle counts and thrust stable",
				Json{ { "handlesBefore", handlesBefore } });
		}

		void OdometerChecks(Report& report, Application& application)
		{
			auto level = std::make_shared<HeadlessModuleLevel>(&application);
			level->BeginPlayInternal();
			Player player;
			shared_ptr<PlayerSpaceShip> ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(ship != nullptr, "the odometer player spawns a real ship through Player::SpawnSpaceShip");
			level->TickInternal(0.f);
			ship->SetUseScreenClamp(false);
			ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const std::optional<ModuleRulesContext> rules = player.BuildModuleRules();
			report.Check(rules.has_value(), "the player ship has a module schematic");

			const std::uint32_t id = GrantedInstance(player, kOdometer);
			report.Check(id != 0, "test grant puts the Rare Odometer in the inventory after the first spawn");
			report.Check(id != 0 && InstallOnPlayer(player, *rules, id), "the granted Odometer installs on the live player loadout");
			const float base = Multiplier(*ship);
			const float baseSpeed = SpeedMultiplier(*ship);
			const auto counter = [&player, id]() { return player.GetModuleLoadout().GetRunCounter(id); };
			const auto dummy = SpawnDummy(*level, *ship);
			const float hitBase = RegenHit(*dummy, ship.get());

			double expected = 0.0;
			int mismatches = 0;
			double firstStackOne = -1.0;
			const sf::Vector2f start = ship->GetActorLocation();
			for (int tick = 0; tick < 400 && counter() < 5000.0; ++tick)
			{
				expected += Step(*level, *ship, kDriveSpeed);
				const int stack = static_cast<int>(counter() / 5000.0);
				if (!Near(Multiplier(*ship) / base, 1.f + kOdometerPerStack[0] * static_cast<float>(stack), 1e-4f)) ++mismatches;
				if (firstStackOne < 0.0 && stack >= 1) firstStackOne = counter();
			}
			const double travelled = static_cast<double>(ship->GetActorLocation().x - start.x);
			report.Check(mismatches == 0 && firstStackOne >= 5000.0 && firstStackOne < 5000.0 + 40.0,
				"driving reaches 5.000 units: stack 1 starts there and the stack follows floor(counter / 5000) on every tick",
				Json{ { "mismatches", mismatches }, { "firstStackOneCounter", firstStackOne } });
			report.Check(std::abs(counter() - expected) <= 1e-3 * expected && std::abs(travelled - expected) <= 0.02 * expected + 1.0,
				"the counter matches the distance the ship really covered", Json{ { "counter", counter() }, { "expected", expected }, { "positionDelta", travelled } });
			report.CheckValue("stack 1 adds +1% outgoing damage (level 1 table)", 1.01f, Multiplier(*ship) / base, "damagePerStack from modules.json");
			report.CheckValue("stack 1 adds +1% movement speed (level 1 table)", 1.01f, SpeedMultiplier(*ship) / baseSpeed, "moveSpeedPerStack from modules.json");
			const float hitStack1 = RegenHit(*dummy, ship.get());
			report.Check(hitBase > 0.f && hitStack1 > 0.f, "the dummy takes real damage from both pipeline hits",
				Json{ { "hitBase", hitBase }, { "hitStack1", hitStack1 }, { "dummyHealth", dummy->GetHealthComponent().GetHealth() } });
			report.CheckValue("a real hit at stack 1 deals 1.01 times the stack 0 damage", 1.01f, hitStack1 / hitBase, "pipeline hit");

			const double atStack1 = counter();
			report.Check(player.TryUninstallModule(id), "the Odometer uninstalls through the Player");
			report.Check(Near(Multiplier(*ship) / base, 1.f) && Near(SpeedMultiplier(*ship) / baseSpeed, 1.f) && counter() == atStack1,
				"uninstalling clears the stack effects and keeps the counter");
			const sf::Vector2f offStart = ship->GetActorLocation();
			for (int tick = 0; tick < 200; ++tick) Step(*level, *ship, kDriveSpeed);
			report.Check(counter() == atStack1 && ship->GetActorLocation().x - offStart.x > 1.f,
				"the ship keeps moving while the Odometer is uninstalled and the counter does not grow");
			report.Check(InstallOnPlayer(player, *rules, id) && Near(Multiplier(*ship) / base, 1.01f, 1e-3f) && counter() == atStack1,
				"reinstalling restores stack 1 at once from the kept counter");

			const std::size_t handles = ship->GetModuleApplier().GetOwnedHandleCount();
			bool cyclesStable = true;
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				report.Check(player.TryUninstallModule(id), "Odometer cycle uninstall through the Player");
				report.Check(InstallOnPlayer(player, *rules, id), "Odometer cycle reinstall through the Player");
				cyclesStable = cyclesStable && ship->GetModuleApplier().GetOwnedHandleCount() == handles && counter() == atStack1 &&
					Near(Multiplier(*ship) / base, 1.01f, 1e-3f);
			}
			report.Check(cyclesStable, "ten Odometer uninstall and reinstall cycles keep the handle count and the stack", Json{ { "handles", handles } });
		}

		void OdometerLevelChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				const float base = Multiplier(*f.ship);
				const float baseSpeed = SpeedMultiplier(*f.ship);
				const std::uint32_t id = f.Install(kOdometer, 0, level);
				Accrue(f, 10000.0);
				const float perStack = kOdometerPerStack[static_cast<std::size_t>(level - 1)];
				const std::string label = "Odometer level " + std::to_string(level) + " stack 2";
				report.CheckValue(label + " damage uses its own per-stack value", 1.f + 2.f * perStack, Multiplier(*f.ship) / base, "runCounterRules table");
				report.CheckValue(label + " speed uses its own per-stack value", 1.f + 2.f * perStack, SpeedMultiplier(*f.ship) / baseSpeed, "runCounterRules table");
				f.Remove(id);
			}
		}

		void HandleChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::size_t baselineHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const std::size_t baselineModifiers = f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount();
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				const int level = 1 + cycle % 3;
				const std::uint32_t core = f.Install(kCore, 0, level);
				const std::uint32_t hull = f.Install(kHull, 0, level);
				const std::uint32_t omni = f.Install(kOmni, 0, level);
				const std::uint32_t odometer = f.Install(kOdometer, 0, level);
				f.Remove(odometer);
				f.Remove(omni);
				f.Remove(hull);
				f.Remove(core);
				report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baselineHandles &&
					f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == baselineModifiers,
					"ten install and remove cycles of all four Rare modules leak no handles", Json{ { "cycle", cycle } });
			}
		}

		void RegressionChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };

			const ModuleDefinition* common = f.rules.lookup(kFiller);
			const sas::AttributeId commonAttribute = FirstAttribute(*common);
			const float commonBefore = f.Owner(commonAttribute);
			const std::uint32_t commonInstance = f.Install(kFiller, 0, 1);
			const ModulePlacement commonPlacement = *f.loadout.FindPlacement(commonInstance);
			const float commonZone = ModuleResolver::GetZoneBonus(f.rules, *common, commonPlacement);
			report.CheckValue("regression: the shipped Common module scales by its own zone bonus only",
				BaseMagnitude(*common, 1) * (1.f + commonZone), f.Owner(commonAttribute) - commonBefore, "Targeting Computer with the Rare set installed on the ship");
			f.Remove(commonInstance);
			report.CheckValue("regression: removing the Common module restores its attribute", 0.f, f.Owner(commonAttribute) - commonBefore,
				"Targeting Computer removed");

			const ModuleDefinition* uncommon = f.rules.lookup(kUncommon);
			const sas::AttributeId attribute = FirstAttribute(*uncommon);
			const float before = f.Owner(attribute);
			const std::uint32_t instance = f.Install(kUncommon, 0, 1);
			const ModulePlacement placement = *f.loadout.FindPlacement(instance);
			const float zoneBonus = ModuleResolver::GetZoneBonus(f.rules, *uncommon, placement);
			report.CheckValue("regression: the shipped Uncommon module scales by its own zone bonus only",
				BaseMagnitude(*uncommon, 1) * (1.f + zoneBonus), f.Owner(attribute) - before, "Double Feed Core, no Rare module installed");
			f.Remove(instance);
			report.CheckValue("regression: removing the Uncommon module restores its attribute", 0.f, f.Owner(attribute) - before, "Double Feed Core removed");
		}
	}
}

int main(int argc, char** argv)
{
	vega::InstallVegaEngineAssets();
	using namespace rare_wave3_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare wave 3 content registration failed" };
		std::string title = "Rare Wave 3 Modules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		CatalogChecks(report);
		DescriptionChecks(report);
		TripleCoreChecks(report, application);
		StrippedHullChecks(report, application);
		OmniChecks(report, application);
		OdometerChecks(report, application);
		OdometerLevelChecks(report, application);
		HandleChecks(report, application);
		RegressionChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareWave3ModulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
