#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/MovementComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ModuleSideFeatureLoader.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageHitGroup.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/tags/GameplayTags.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

namespace rare_wave5_content_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		// Shipped content only: every module and side feature below is read from modules.json and module_side_features.json.
		constexpr const char* kTrophy = "Module.Offense.TrophyCore.Rare";
		constexpr const char* kDying = "Module.Offense.DyingStar.Rare";
		constexpr const char* kEmber = "Module.Offense.EmberAccumulation.Rare";
		constexpr const char* kShatter = "Module.Defense.ShatterShock.Rare";
		constexpr const char* kKinetic = "Module.Offense.KineticBreaker.Rare";
		constexpr const char* kEscape = "Module.Movement.EscapeVelocity.Rare";
		constexpr const char* kBounty = "Module.SideFeature.TrophyCore.BountyMark";
		constexpr const char* kWall = "Module.SideFeature.TrophyCore.TrophyWall";
		constexpr const char* kCollapse = "Module.SideFeature.DyingStar.CoreCollapse";
		constexpr const char* kRedGiant = "Module.SideFeature.DyingStar.RedGiant";
		constexpr const char* kArea = "Module.SideFeature.EmberAccumulation.AreaBlast";
		constexpr const char* kSmolder = "Module.SideFeature.EmberAccumulation.SmolderingRemains";
		constexpr const char* kRend = "Module.SideFeature.KineticBreaker.ArmorRend";
		constexpr const char* kStagger = "Module.SideFeature.ShatterShock.Stagger";
		constexpr const char* kAftershock = "Module.SideFeature.ShatterShock.AftershockWave";
		constexpr const char* kAfterglow = "Module.SideFeature.EscapeVelocity.Afterglow";
		constexpr const char* kSlipstream = "Module.SideFeature.EscapeVelocity.SlipstreamControl";

		// Size variant indices: the two-cell (small) and the three-cell (large) variant of the Rare ladder modules.
		// Shatter Shock's ladder is 1-3, so its large variant is index 1 as well.
		constexpr int kSmall = 0;
		constexpr int kLarge = 1;

		constexpr float kDriveStep = 0.1f;
		constexpr float kFastSpeed = 100000.f;
		constexpr float kProbeDamage = 10.f;
		constexpr float kProbeSpeed = 1100.f;
		constexpr float kFourStackDoT = 25.f;
		constexpr float kShatterRadius = 250.f;
		constexpr float kAftershockTargetDistance = 50.f;

		// Level tables as shipped. The Dying Star base damage (60/69/78) is a PLACEHOLDER pending the balance decision.
		constexpr std::array<float, 3> kTrophyPerKill{ 0.004f, 0.0045f, 0.005f };
		constexpr std::array<float, 3> kDyingDamage{ 60.f, 69.f, 78.f };
		constexpr std::array<float, 3> kShatterFractionLarge{ 0.25f, 0.31f, 0.37f };
		constexpr std::array<float, 3> kEmberFraction{ 1.30f, 1.40f, 1.50f };
		constexpr std::array<float, 3> kKineticMultiplier{ 1.10f, 1.15f, 1.20f };

		sas::ContentId ContentIdOf(const char* id)
		{
			return sas::ContentId{ std::string{ id } };
		}

		bool Near(float actual, float expected, float tolerance = 1e-4f)
		{
			return std::abs(actual - expected) <= tolerance;
		}

		bool Contains(const List<GameplayTag>& tags, const GameplayTag& tag)
		{
			return std::find(tags.begin(), tags.end(), tag) != tags.end();
		}

		float Health(SpaceShip& ship)
		{
			return ship.GetHealthComponent().GetHealth();
		}

		float Armor(SpaceShip& ship)
		{
			return ship.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor);
		}

		bool HasEffect(SpaceShip& ship, const char* effectId)
		{
			return ship.GetAbilitySystemComponent().FindGameplayEffectById(effectId) != nullptr;
		}

		int StackCount(SpaceShip& ship, const char* effectId)
		{
			const auto* effect = ship.GetAbilitySystemComponent().FindGameplayEffectById(effectId);
			return effect ? effect->stackCount : 0;
		}

		float Distance(SpaceShip& from, SpaceShip& to)
		{
			const sf::Vector2f delta = to.GetActorLocation() - from.GetActorLocation();
			return std::sqrt(delta.x * delta.x + delta.y * delta.y);
		}

		void Advance(Fixture& f, float seconds)
		{
			float remaining = seconds;
			while (remaining > 1e-4f)
			{
				const float step = std::min(0.1f, remaining);
				f.level->TickInternal(step);
				remaining -= step;
			}
		}

		float Ratio(float after, float before)
		{
			return before > 0.f ? after / before : 0.f;
		}

		int SharedChoiceCount(const ModuleDefinition& definition)
		{
			int count = 0;
			for (const auto& pool : definition.sharedPools) count += static_cast<int>(pool.allowedFeatureIds.size());
			return count;
		}

		const ModuleDefinition* Definition(const char* id)
		{
			return content::ModuleContentCatalog::FindById(ContentIdOf(id));
		}

		const ModuleStatusRules& RuleStatusOf(Fixture& f)
		{
			return f.ship->GetCombatRuntime().GetModuleStatusRules();
		}

		// A ship-side enemy whose shield is removed for good, so every point of damage lands on health.
		shared_ptr<SpaceShip> ZeroShieldEnemy(Fixture& f, float distance)
		{
			auto enemy = Enemy(f, distance);
			enemy->GetShieldComponent().SetMaxShield(0.f);
			return enemy;
		}

		// One owner kill of a fresh zero-shield enemy at the given distance, optionally with a payload.
		void KillAt(Fixture& f, float distance, const DamagePayload& payload = Plain())
		{
			auto enemy = ZeroShieldEnemy(f, distance);
			Hit(*enemy, f.ship.get(), 100000.f, payload);
		}

		void SetShield(SpaceShip& ship, float value)
		{
			ShieldComponent& shield = ship.GetShieldComponent();
			shield.ChangeShield(-shield.GetShield());
			if (value > 0.f) shield.ChangeShield(value);
		}

		void SetOwnerMaxHealth(Fixture& f, float maxHealth)
		{
			f.ship->GetHealthComponent().SetMaxHealth(maxHealth);
			f.ship->GetHealthComponent().Regenerate(10000.f);
		}

		// Breaks the owner's shield with one hit from source and checks that the shield actually reached zero.
		void Break(Report& report, Fixture& f, Actor* source)
		{
			SetShield(*f.ship, 10.f);
			Hit(*f.ship, source, 30.f, Plain());
			report.Check(f.ship->GetShieldComponent().GetShield() <= 0.f, "owner shield reaches zero in the break");
		}

		// Health lost by a 1-damage plain hit of the owner against a distant, unshielded target.
		float OutgoingScale(Fixture& f)
		{
			const auto gauge = Enemy(f);
			gauge->SetActorLocation({ 30000.f, 30000.f });
			return Hit(*gauge, f.ship.get(), 1.f, Plain());
		}

		DamagePayload Thermal(int stacks)
		{
			DamagePayload payload = Plain();
			payload.igniteStacks = stacks;
			return payload;
		}

		DamagePayload Kinetic(int stacks)
		{
			DamagePayload payload = Plain();
			payload.kineticStacks = stacks;
			return payload;
		}

		float ThermalHit(SpaceShip& target, Actor* source)
		{
			return Hit(target, source, 1.f, Thermal(1), {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Thermal });
		}

		float KineticHit(SpaceShip& target, Actor* source, int stacks)
		{
			return Hit(target, source, 1.f, Kinetic(stacks), {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Kinetic });
		}

		void CatalogChecks(Report& report)
		{
			const auto modules = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(modules.Succeeded(), "shipped modules.json loads through the real ModuleLoader", modules.error);
			const auto sides = content::ModuleSideFeatureLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/module_side_features.json");
			report.Check(sides.Succeeded(), "shipped module_side_features.json loads through the real side-feature loader", sides.error);

			struct Subject { const char* id; std::size_t privateCount; int sharedCount; };
			const Subject subjects[] = { { kTrophy, 2, 3 }, { kDying, 2, 3 } };
			for (const Subject& subject : subjects)
			{
				const ModuleDefinition* definition = Definition(subject.id);
				report.Check(definition != nullptr, std::string{ "catalog contains " } + subject.id);
				report.Check(definition->rarity == ModuleRarity::Rare && definition->levelCount == 3 && definition->family.name == "Ability.Offense",
					std::string{ "Rare, three levels, Offense family: " } + subject.id);
				report.Check(definition->sizeVariants.size() == 2 && definition->sizeLadderCellCounts.size() == 2 &&
					definition->sizeVariants[0].shape.cells.size() == 2 && definition->sizeVariants[1].shape.cells.size() == 3,
					std::string{ "two size variants, two and three cells: " } + subject.id);
				report.Check(definition->sideFeatures.size() == subject.privateCount && SharedChoiceCount(*definition) == subject.sharedCount,
					std::string{ "private and shared side choices match the design: " } + subject.id,
					Json{ { "private", definition->sideFeatures.size() }, { "shared", SharedChoiceCount(*definition) } });
				for (const ModuleSizeVariant& variant : definition->sizeVariants)
					for (int level = 1; level <= 3; ++level)
						report.Check(HasModuleRulePackageLevelCoverage(variant, level), std::string{ "level table covers level " } + std::to_string(level) + ": " + subject.id);
				report.Check(!HasModuleRulePackageLevelCoverage(definition->sizeVariants[1], 4), std::string{ "no level beyond three: " } + subject.id);
				for (const sas::ContentId& feature : definition->sideFeatures)
					report.Check(content::ModuleSideFeatureCatalog::FindById(feature) != nullptr, "side feature loads: " + feature.ToString());
			}

			const ModuleDefinition* trophy = Definition(kTrophy);
			const ModuleRunCounterRule& kill = trophy->sizeVariants[kLarge].runCounterRules.front();
			report.Check(kill.source == ModuleRunCounterSource::Kill && kill.adjacentAbilityLimit == 3 && Near(kill.killWeights.normal, 1.f) &&
				Near(kill.killWeights.elite, 7.5f) && Near(kill.killWeights.miniBoss, 7.5f) && Near(kill.killWeights.boss, 25.f),
				"Trophy Core kill rule: weights 1 / 7.5 / 7.5 / 25, adjacency limit 3");
			for (int level = 1; level <= 3; ++level)
				report.CheckValue("Trophy Core per-kill ratio at level " + std::to_string(level), kTrophyPerKill[static_cast<std::size_t>(level - 1)],
					kill.damagePerStackByLevel.ValueAtLevel(kill.damagePerStack, level), "level table 0.40 / 0.45 / 0.50 percent");

			const ModuleDefinition* dying = Definition(kDying);
			const ModuleOwnerBurstRule& burst = dying->sizeVariants[kLarge].ownerBurstRules.front();
			report.Check(burst.trigger == ModuleTriggerKind::KillConfirmed && burst.chargeKills == 10 && Near(burst.chargeRadius, 300.f) &&
				Near(burst.radius, 350.f) && burst.igniteStacks == 4 && Near(burst.cooldownSeconds, 0.f) && Near(burst.pullImpulse, 0.f),
				"Dying Star rule: ten charges within 300, burst radius 350, four Ignite stacks, no cooldown");
			for (int level = 1; level <= 3; ++level)
				report.CheckValue("Dying Star flat burst at level " + std::to_string(level) + " (PLACEHOLDER)", kDyingDamage[static_cast<std::size_t>(level - 1)],
					burst.damageByLevel.ValueAtLevel(burst.damage, level), "placeholder base damage, balance decision pending");
		}

		void DescriptionChecks(Report& report)
		{
			struct Text { const char* id; std::array<const char*, 3> expected; };
			const Text modules[] = {
				{ kTrophy, { "adds 0,4% damage for the run (elite 3%, boss 10%)", "adds 0,45% damage for the run (elite 3,375%, boss 11,25%)", "adds 0,5% damage for the run (elite 3,75%, boss 12,5%)" } },
				{ kDying, { "deals 60 damage and applies 4 Ignite stacks", "deals 69 damage and applies 4 Ignite stacks", "deals 78 damage and applies 4 Ignite stacks" } },
			};
			for (const Text& text : modules)
			{
				const ModuleDefinition* definition = Definition(text.id);
				report.Check(definition != nullptr, std::string{ "catalog contains " } + text.id);
				for (int level = 1; level <= 3; ++level)
				{
					const std::string formatted = FormatModuleDescription(*definition, definition->sizeVariants[kLarge], level);
					report.Check(formatted.find('{') == std::string::npos && formatted.find('}') == std::string::npos,
						"description level " + std::to_string(level) + " is fully formatted: " + text.id, Json{ { "text", formatted } });
					report.Check(formatted.find(text.expected[static_cast<std::size_t>(level - 1)]) != std::string::npos,
						"description shows the level " + std::to_string(level) + " value: " + text.id, Json{ { "text", formatted } });
				}
			}

			struct Side { const char* id; const char* expected; };
			const Side sides[] = {
				{ kBounty, "Each 25 kill stacks cut the adjacent ability's cooldown by 5%, then 4%, 3%, 2%, and 1% per step after that" },
				{ kWall, "50% of the kill bonus also applies to every Offense ability" },
				{ kCollapse, "1 sec before the burst, enemies in its radius are pulled toward you with 400 impulse" },
				{ kRedGiant, "Kills charge the star from 450 units around you" },
				{ kArea, "hits enemies within 200 units of the target for 60% of its damage" },
				{ kSmolder, "leaves a field for 3 sec within 150 units that adds 1 Thermal stack per second" },
				{ kRend, "reduced by 25% for 4 sec" },
				{ kStagger, "stunned for 1 sec" },
				{ kAftershock, "1,5 sec after the burst, a second wave pushes enemies in its radius back with 50% of the push" },
				{ kAfterglow, "lasts 1,5 sec longer" },
				{ kSlipstream, "turn capability increases by 25%" },
			};
			for (const Side& side : sides)
			{
				const ModuleSideFeatureDefinition* feature = content::ModuleSideFeatureCatalog::FindById(ContentIdOf(side.id));
				report.Check(feature != nullptr, std::string{ "side feature catalog contains " } + side.id);
				for (int level = 1; level <= 3; ++level)
				{
					const std::string formatted = FormatModuleSideFeatureDescription(*feature, level);
					report.Check(formatted.find('{') == std::string::npos && formatted.find('}') == std::string::npos,
						std::string{ "side description is fully formatted: " } + side.id, Json{ { "text", formatted } });
					report.Check(formatted.find(side.expected) != std::string::npos, std::string{ "side description shows its values: " } + side.id,
						Json{ { "text", formatted }, { "expected", side.expected } });
				}
			}
		}

		// Installs id on the first legal origin next to the given ability shell. Strict first: the shell must be the only neighbour.
		std::uint32_t InstallNextTo(Fixture& f, const char* id, const GameplayTag& system, int variant, int level = 1, const char* side = "")
		{
			const auto* definition = f.rules.lookup(id);
			if (!definition) throw std::runtime_error{ std::string{ "Rare wave 5 fixture is missing: " } + id };
			const std::uint32_t instance = f.loadout.AddToInventory(f.rules, *definition, variant, definition->rarity, level, sas::ContentId{ side });
			if (instance == 0) throw std::runtime_error{ "Rare wave 5 inventory rejected the module" };
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
			throw std::runtime_error{ "No legal Rare wave 5 placement next to the requested ability shell" };
		}

		void WireKillCredit(Fixture& f)
		{
			f.ship->GetModuleApplier().SetKillCreditSink([&f](std::uint32_t instanceId, double stacks, double damageBonus)
			{
				f.loadout.AddKillCredit(instanceId, stacks, damageBonus);
			});
		}

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

		// First of the three non-default shells whose system is not adjacent to the module, or None.
		sas::AbilitySlot FirstNonAdjacentSlot(Fixture& f, std::uint32_t module)
		{
			const ModulePlacement* placed = f.loadout.FindPlacement(module);
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
		void AbilityKill(Fixture& f, const sas::ContentId& ability, const List<GameplayTag>& tags, ControlTargetClass targetClass = ControlTargetClass::Normal)
		{
			const auto target = Enemy(f);
			target->SetControlTargetClass(targetClass);
			Hit(*target, f.ship.get(), 100000.f, Plain(), ability, tags);
		}

		// Trophy Core credit: the adjacent ability's kills build stacks; other kills do not.
		void TrophyCreditChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophy, GameplayTags::Module::System::AbilitySlot1, kSmall);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const sas::AbilitySlot otherSlot = FirstNonAdjacentSlot(f, trophy);
			report.Check(neighbour.present && otherSlot != sas::AbilitySlot::None, "trophy has an adjacent and a non-adjacent loadout ability");
			const SlotAbility other = SlotAbilityOf(f, otherSlot);
			const float baseline = AbilityHit(f, neighbour.id, neighbour.tags);

			AbilityKill(f, neighbour.id, neighbour.tags);
			report.CheckValue("a neighbour normal kill adds one stack", 1.f, static_cast<float>(f.loadout.GetKillStacks(trophy)), "small size, level one");
			report.CheckValue("a neighbour normal kill adds the level one ratio", kTrophyPerKill[0], static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), "0.40 percent");
			report.Check(Near(Ratio(AbilityHit(f, neighbour.id, neighbour.tags), baseline), 1.f + kTrophyPerKill[0], 1e-3f), "the neighbour's damage gains +0.40 percent");

			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Elite);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::MiniBoss);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.CheckValue("elite, miniboss and boss kills add 7.5, 7.5 and 25 stacks (total 41)", 41.f, static_cast<float>(f.loadout.GetKillStacks(trophy)), "class weights");
			report.Check(Near(Ratio(AbilityHit(f, neighbour.id, neighbour.tags), baseline), 1.f + 41.f * kTrophyPerKill[0], 2e-3f),
				"the neighbour's damage follows 1 + 41 x 0.40 percent");

			AbilityKill(f, other.id, other.tags, ControlTargetClass::Boss);
			AbilityKill(f, sas::ContentId{ "Fixture.Ability.NoSlot" }, {});
			report.CheckValue("kills by a non-adjacent ability or a non-loadout source add nothing", 41.f, static_cast<float>(f.loadout.GetKillStacks(trophy)), "adjacency only");
		}

		void TrophyLevelChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				WireKillCredit(f);
				const std::uint32_t trophy = InstallNextTo(f, kTrophy, GameplayTags::Module::System::AbilitySlot1, kLarge, level);
				const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
				const float baseline = AbilityHit(f, neighbour.id, neighbour.tags);
				AbilityKill(f, neighbour.id, neighbour.tags);
				report.CheckValue("level " + std::to_string(level) + " large trophy: per-kill ratio", kTrophyPerKill[static_cast<std::size_t>(level - 1)],
					static_cast<float>(f.loadout.GetKillDamageBonus(trophy)), "level table");
				report.Check(Near(Ratio(AbilityHit(f, neighbour.id, neighbour.tags), baseline), 1.f + kTrophyPerKill[static_cast<std::size_t>(level - 1)], 1e-3f),
					"level " + std::to_string(level) + " neighbour damage follows its own ratio");
			}
		}

		// Bounty Mark: 25 stacks per step, cuts 5, 4, 3, 2 then 1 percent per step (additive), with a one second floor. Other abilities are untouched.
		void TrophyBountyChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophy, GameplayTags::Module::System::AbilitySlot1, kLarge, 1, kBounty);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			const sas::AbilitySlot otherSlot = FirstNonAdjacentSlot(f, trophy);
			const auto* ability = f.ship->GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability1);
			const auto* otherAbility = f.ship->GetAbilitySystemComponent().GetAbility(otherSlot);
			report.Check(ability != nullptr && otherAbility != nullptr, "bounty fixture reads both abilities from the live ship");
			const float before = f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(ability->GetDefinition());
			const float beforeOther = f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(otherAbility->GetDefinition());
			const auto stepMultiplier = [&f, ability, before]() { return Ratio(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(ability->GetDefinition()), before); };

			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 25.f) && Near(stepMultiplier(), 0.95f, 1e-3f),
				"25 stacks (one boss kill) cut the neighbour's cooldown by 5 percent", Json{ { "multiplier", stepMultiplier() } });
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(stepMultiplier(), 0.91f, 1e-3f), "50 stacks cut 9 percent (5 + 4, additive)", Json{ { "multiplier", stepMultiplier() } });
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 125.f) && Near(stepMultiplier(), 0.85f, 1e-3f),
				"125 stacks cut 15 percent (5 + 4 + 3 + 2 + 1)", Json{ { "multiplier", stepMultiplier() } });
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			report.Check(Near(stepMultiplier(), 0.84f, 1e-3f), "after the list every further step cuts a flat 1 percent", Json{ { "multiplier", stepMultiplier() } });
			report.Check(Near(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMinimum(ability->GetDefinition()), 1.f, 1e-6f),
				"the neighbour's cooldown keeps the one second floor");
			report.Check(Near(Ratio(f.ship->GetAbilitySystemComponent().GetScopedAbilityCooldownMultiplier(otherAbility->GetDefinition()), beforeOther), 1.f, 1e-4f),
				"the bounty does not reach a non-adjacent ability");
		}

		// Trophy Wall: half of the placement's kill bonus reaches every Offense ability; a Defense ability is untouched.
		void TrophyWallChecks(Report& report, Application& application)
		{
			const sas::ContentId offense{ "Fixture.Ability.Offense" };
			const sas::ContentId defense{ "Fixture.Ability.Defense" };
			const List<GameplayTag> offenseTags{ GameplayTags::Ability::Offense };
			const List<GameplayTag> defenseTags{ GameplayTags::Ability::Defense };
			{
				Fixture f{ report, application };
				WireKillCredit(f);
				InstallNextTo(f, kTrophy, GameplayTags::Module::System::AbilitySlot1, kLarge, 1, kWall);
				const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
				const float offenseBefore = AbilityHit(f, offense, offenseTags);
				const float defenseBefore = AbilityHit(f, defense, defenseTags);
				AbilityKill(f, neighbour.id, neighbour.tags);
				AbilityKill(f, neighbour.id, neighbour.tags);
				report.Check(Near(Ratio(AbilityHit(f, offense, offenseTags), offenseBefore), 1.f + 0.5f * 2.f * kTrophyPerKill[0], 1e-3f),
					"Trophy Wall gives an Offense ability outside the neighbour set half of two stacks");
				report.Check(Near(Ratio(AbilityHit(f, defense, defenseTags), defenseBefore), 1.f, 1e-3f), "Trophy Wall does not reach a Defense ability");
			}
			{
				Fixture plain{ report, application };
				WireKillCredit(plain);
				InstallNextTo(plain, kTrophy, GameplayTags::Module::System::AbilitySlot1, kLarge, 1);
				const SlotAbility neighbour = SlotAbilityOf(plain, sas::AbilitySlot::Ability1);
				const float offenseBefore = AbilityHit(plain, offense, offenseTags);
				AbilityKill(plain, neighbour.id, neighbour.tags);
				AbilityKill(plain, neighbour.id, neighbour.tags);
				report.Check(Near(Ratio(AbilityHit(plain, offense, offenseTags), offenseBefore), 1.f, 1e-3f), "without Trophy Wall an Offense ability is unchanged");
			}
		}

		// Sell and re-fit ten times: the handle count is fixed and the stacks stay with the module.
		void TrophyCycleChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			WireKillCredit(f);
			const std::uint32_t trophy = InstallNextTo(f, kTrophy, GameplayTags::Module::System::AbilitySlot1, kLarge, 1, kWall);
			const SlotAbility neighbour = SlotAbilityOf(f, sas::AbilitySlot::Ability1);
			AbilityKill(f, neighbour.id, neighbour.tags, ControlTargetClass::Boss);
			const std::size_t handles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				const ModulePlacement placed = *f.loadout.FindPlacement(trophy);
				report.Check(f.loadout.Uninstall(trophy) && f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "trophy uninstalls through the real applier");
				report.Check(f.loadout.Install(f.rules, trophy, placed.origin, placed.rotationQuarterTurns) == ModulePlacementFailure::None &&
					f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "trophy reinstalls at its cell through the real applier");
			}
			report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == handles && Near(static_cast<float>(f.loadout.GetKillStacks(trophy)), 25.f),
				"ten sell and re-fit cycles keep the handle count and the 25 kill stacks");
		}

		// Dying Star: ten charges inside 300 fire one Thermal burst of the level's flat damage and four Ignite stacks, then reset.
		void DyingLevelChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				f.ship->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::EnergyPower, 100.f);
				f.Install(kDying, kLarge, level);
				auto bystander = ZeroShieldEnemy(f, 340.f);
				Advance(f, 0.1f);
				const float start = Health(*bystander);
				for (int kill = 0; kill < 9; ++kill) KillAt(f, 100.f);
				report.CheckValue("level " + std::to_string(level) + ": nine charges do not burst", 0.f, start - Health(*bystander), "charge count below ten");
				KillAt(f, 100.f);
				report.CheckValue("level " + std::to_string(level) + ": the tenth charge bursts for the level damage (PLACEHOLDER)", (kDyingDamage[static_cast<std::size_t>(level - 1)] + 0.4f * f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)),
					start - Health(*bystander), "flat damage table");
				report.CheckValue("level " + std::to_string(level) + ": each burst hit gets four Ignite stacks", 4.f,
					static_cast<float>(StackCount(*bystander, DamageStatusEffectIds::IgniteEffectId)), "igniteStacks");
				report.CheckValue("the burst leaves no pending wave", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "no detonation delay");
			}
		}

		// Charge radius 300 is the boundary: kills at 310 do not charge, kills at 300 do. A target outside the 350 burst radius takes nothing.
		void DyingRadiusChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.Install(kDying, kLarge, 1);
			auto bystander = ZeroShieldEnemy(f, 340.f);
			auto outsider = ZeroShieldEnemy(f, 370.f);
			Advance(f, 0.1f);
			const float start = Health(*bystander);
			const float outsideStart = Health(*outsider);
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 310.f);
			report.CheckValue("ten kills at 310 units (outside the 300 charge radius) do not charge", 0.f, start - Health(*bystander), "charge radius");
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 300.f);
			report.CheckValue("ten kills at exactly 300 units charge and burst", (kDyingDamage[0] + 0.4f * f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)), start - Health(*bystander), "charge radius boundary");
			report.CheckValue("a target outside the 350 burst radius takes nothing", 0.f, outsideStart - Health(*outsider), "burst radius");
		}

		// Core Collapse: one second before the detonation elites are pulled in, bosses are not; every class still takes the burst.
		void DyingCollapseChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.Install(kDying, kLarge, 1, kCollapse);
			auto normal = ZeroShieldEnemy(f, 250.f);
			auto elite = ZeroShieldEnemy(f, 230.f);
			auto boss = ZeroShieldEnemy(f, 210.f);
			elite->SetControlTargetClass(ControlTargetClass::Elite);
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			Advance(f, 0.1f);
			const float normalStart = Health(*normal);
			const float eliteStart = Health(*elite);
			const float bossStart = Health(*boss);
			const float normalDistance = Distance(*f.ship, *normal);
			const float eliteDistance = Distance(*f.ship, *elite);
			const float bossDistance = Distance(*f.ship, *boss);
			for (int kill = 0; kill < 10; ++kill) KillAt(f, 100.f);
			report.CheckValue("the charged burst waits one second: one detonation is pending", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "pull lead");
			Advance(f, 0.2f);
			report.Check(Distance(*f.ship, *normal) < normalDistance - 10.f, "Core Collapse pulls a normal enemy toward the owner before the detonation",
				Json{ { "before", normalDistance }, { "after", Distance(*f.ship, *normal) } });
			report.Check(Distance(*f.ship, *elite) < eliteDistance - 10.f, "Core Collapse pulls an elite enemy toward the owner");
			report.Check(std::abs(Distance(*f.ship, *boss) - bossDistance) < 1.f, "Core Collapse does not pull a boss",
				Json{ { "before", bossDistance }, { "after", Distance(*f.ship, *boss) } });
			Advance(f, 0.6f);
			report.CheckValue("no damage yet at 0.8 seconds", 0.f, normalStart - Health(*normal), "pull lead");
			for (int step = 0; step < 5 && f.ship->GetModuleApplier().GetOwnerBurstEchoCount() > 0; ++step) Advance(f, 0.1f);
			report.CheckValue("the detonation has fired once its lead has run out", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "pull cleanup");
			report.CheckValue("the detonation hits a normal enemy for the level damage (PLACEHOLDER)", (kDyingDamage[0] + 0.4f * f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)), normalStart - Health(*normal), "pull then burst");
			report.CheckValue("the detonation hits an elite enemy too", (kDyingDamage[0] + 0.4f * f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)), eliteStart - Health(*elite), "elite is pulled, not immune");
			report.CheckValue("the detonation still damages a boss; only the pull skips it", (kDyingDamage[0] + 0.4f * f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)), bossStart - Health(*boss), "boss pull immunity only");
		}

		// Red Giant: the charge radius grows from 300 to 450; without it, kills at 400 do not charge.
		void DyingRedGiantChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				f.Install(kDying, kLarge, 1, kRedGiant);
				auto bystander = ZeroShieldEnemy(f, 340.f);
				Advance(f, 0.1f);
				const float start = Health(*bystander);
				for (int kill = 0; kill < 10; ++kill) KillAt(f, 400.f);
				report.CheckValue("Red Giant: ten kills at 400 units charge (radius 450)", (kDyingDamage[0] + 0.4f * f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)), start - Health(*bystander), "chargeRadius 450");
			}
			{
				Fixture f{ report, application };
				f.Install(kDying, kLarge, 1);
				auto bystander = ZeroShieldEnemy(f, 340.f);
				Advance(f, 0.1f);
				const float start = Health(*bystander);
				for (int kill = 0; kill < 10; ++kill) KillAt(f, 400.f);
				report.CheckValue("without Red Giant, ten kills at 400 units do not charge (radius 300)", 0.f, start - Health(*bystander), "base chargeRadius");
			}
		}

		// Area Blast: the burst also hits enemies within 200 units for 60 percent of its damage; no stacks; outside the area nothing.
		void EmberAreaChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				const float fraction = kEmberFraction[static_cast<std::size_t>(level - 1)];
				Fixture f{ report, application };
				const float outgoing = OutgoingScale(f);
				f.Install(kEmber, kLarge, level, kArea);
				const auto& rules = RuleStatusOf(f).thermalBursts;
				report.Check(rules.size() == 1 && Near(rules[0].areaRadius, 200.f) && Near(rules[0].areaDamageFraction, 0.6f) && rules[0].smolderSeconds == 0.f,
					"Area Blast merges into the single Ember burst rule", Json{ { "level", level }, { "rules", rules.size() } });

				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
				Advance(f, 0.1f);
				const auto near = Enemy(f);
				const auto far = Enemy(f);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 120.f });
				far->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
				const float nearBefore = Health(*near);
				const float farBefore = Health(*far);
				const float fourthDrop = ThermalHit(*target, f.ship.get());
				const float nearDrop = nearBefore - Health(*near);
				const float farDrop = farBefore - Health(*far);
				report.Check(std::abs(fourthDrop - outgoing * (1.f + fraction * kFourStackDoT)) < 0.05f, "the target takes its hit plus exactly one burst",
					Json{ { "level", level }, { "drop", fourthDrop } });
				report.Check(std::abs(nearDrop - outgoing * 0.6f * fraction * kFourStackDoT) < 0.05f, "the neighbour inside the area takes 60 percent of the burst",
					Json{ { "level", level }, { "drop", nearDrop } });
				report.Check(farDrop < 0.01f, "the neighbour outside the area is untouched", Json{ { "drop", farDrop } });
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 0, "the area neighbour gets no Thermal stacks");
			}

			Fixture plain{ report, application };
			plain.Install(kEmber, kLarge, 1);
			const auto target = Enemy(plain);
			for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, plain.ship.get());
			Advance(plain, 0.1f);
			const auto near = Enemy(plain);
			near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 120.f });
			const float nearBefore = Health(*near);
			ThermalHit(*target, plain.ship.get());
			report.Check(std::abs(nearBefore - Health(*near)) < 0.01f, "without Area Blast the neighbour takes nothing from the burst");
		}

		// Smoldering Remains: the burst leaves a three second field that adds one Thermal stack per second inside 150 units.
		void EmberSmolderChecks(Report& report, Application& application)
		{
			{
				Fixture f{ report, application };
				const float outgoing = OutgoingScale(f);
				f.Install(kEmber, kLarge, 1, kSmolder);
				const auto& rules = RuleStatusOf(f).thermalBursts;
				report.Check(rules.size() == 1 && Near(rules[0].smolderSeconds, 3.f) && Near(rules[0].smolderRadius, 150.f) && rules[0].smolderStacksPerSecond == 1 &&
					Near(rules[0].areaRadius, 0.f), "Smoldering Remains merges into the single Ember burst rule");
				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
				Advance(f, 0.1f);
				const auto near = Enemy(f);
				const auto far = Enemy(f);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 100.f });
				far->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 400.f });
				const float fourthDrop = ThermalHit(*target, f.ship.get());
				report.Check(std::abs(fourthDrop - outgoing * (1.f + kEmberFraction[0] * kFourStackDoT)) < 0.05f, "the field adds no damage to the burst itself",
					Json{ { "drop", fourthDrop } });
				report.CheckValue("the burst leaves exactly one smoldering field", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetSmolderFieldCount()), "field count");

				Advance(f, 0.5f);
				report.CheckValue("no pulse before the first full second", 0.f, static_cast<float>(StackCount(*near, DamageStatusEffectIds::IgniteEffectId)), "pulse timing");
				Advance(f, 0.6f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 1 && StackCount(*far, DamageStatusEffectIds::IgniteEffectId) == 0,
					"the first pulse adds one Thermal stack inside the radius only");
				Advance(f, 1.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 2, "the second pulse adds the second stack");
				Advance(f, 1.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 3 && f.ship->GetModuleApplier().GetSmolderFieldCount() == 0,
					"the third pulse lands and the field ends after three seconds");
				Advance(f, 1.0f);
				report.Check(StackCount(*near, DamageStatusEffectIds::IgniteEffectId) == 3, "an ended field pulses no more");
				report.Check(StackCount(*far, DamageStatusEffectIds::IgniteEffectId) == 0, "the far enemy never receives a stack");
			}
			{
				Fixture plain{ report, application };
				plain.Install(kEmber, kLarge, 1);
				const auto target = Enemy(plain);
				for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, plain.ship.get());
				Advance(plain, 0.1f);
				const auto near = Enemy(plain);
				near->SetActorLocation(target->GetActorLocation() + sf::Vector2f{ 0.f, 100.f });
				ThermalHit(*target, plain.ship.get());
				report.CheckValue("without the side feature a burst leaves no field", 0.f, static_cast<float>(plain.ship->GetModuleApplier().GetSmolderFieldCount()), "no field");
				Advance(plain, 3.5f);
				report.CheckValue("without the side feature nothing pulses", 0.f, static_cast<float>(StackCount(*near, DamageStatusEffectIds::IgniteEffectId)), "no pulses");
			}
		}

		// Armor Rend: after the spend the target has 25 percent less Armor for four seconds; the spend itself keeps the level multiplier.
		void KineticRendChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				const auto gauge = Enemy(f);
				gauge->SetActorLocation({ 30000.f, 30000.f });
				const float baseline = Hit(*gauge, f.ship.get(), 10.f, Plain());
				f.Install(kKinetic, kLarge, level, kRend);
				const auto& consume = RuleStatusOf(f).kineticConsumes;
				report.Check(consume.size() == 1 && Near(consume[0].rendFraction, 0.25f) && Near(consume[0].rendSeconds, 4.f) && consume[0].stackThreshold == 4,
					"Armor Rend merges into the single Kinetic consume rule", Json{ { "level", level } });
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				for (int hit = 0; hit < 3; ++hit) KineticHit(*target, f.ship.get(), 1);
				Hit(*target, f.ship.get(), 1.f, Plain());
				KineticHit(*target, f.ship.get(), 1);
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.Check(std::abs(spend - baseline * kKineticMultiplier[static_cast<std::size_t>(level - 1)]) < 0.05f && !HasEffect(*target, DamageStatusEffectIds::KineticEffectId),
					"the spend applies the level multiplier and clears the stacks", Json{ { "level", level }, { "spend", spend } });
				report.Check(HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId) && std::abs(Armor(*target) - 75.f) < 0.01f,
					"Armor Rend lowers Armor by 25 percent right after the spend", Json{ { "armor", Armor(*target) } });
				Advance(f, 3.5f);
				report.Check(std::abs(Armor(*target) - 75.f) < 0.01f, "Armor Rend still applies at 3.5 seconds", Json{ { "armor", Armor(*target) } });
				Advance(f, 0.7f);
				report.Check(std::abs(Armor(*target) - 100.f) < 0.01f && !HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId),
					"Armor Rend ends after four seconds", Json{ { "armor", Armor(*target) } });
			}

			Fixture plain{ report, application };
			plain.Install(kKinetic, kLarge, 1);
			const auto target = Enemy(plain);
			target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
			for (int hit = 0; hit < 3; ++hit) KineticHit(*target, plain.ship.get(), 1);
			Hit(*target, plain.ship.get(), 1.f, Plain());
			KineticHit(*target, plain.ship.get(), 1);
			Hit(*target, plain.ship.get(), 10.f, Plain());
			report.Check(!HasEffect(*target, DamageStatusEffectIds::KineticArmorRendEffectId) && std::abs(Armor(*target) - 100.f) < 0.01f,
				"without Armor Rend the spend leaves Armor untouched");
		}

		// Stagger: enemies hit by the burst are stunned for one second; elite, miniboss and boss targets are immune.
		void ShatterStaggerChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.Install(kShatter, kLarge, 1, kStagger);
			SetOwnerMaxHealth(f, 400.f);
			auto normal = ZeroShieldEnemy(f, 100.f);
			auto elite = ZeroShieldEnemy(f, 120.f);
			auto miniBoss = ZeroShieldEnemy(f, 140.f);
			auto boss = ZeroShieldEnemy(f, 160.f);
			elite->SetControlTargetClass(ControlTargetClass::Elite);
			miniBoss->SetControlTargetClass(ControlTargetClass::MiniBoss);
			boss->SetControlTargetClass(ControlTargetClass::Boss);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float normalStart = Health(*normal);
			Break(report, f, attacker.get());
			Advance(f, 0.1f);
			const auto stunned = GameplayTags::State::Effect::Control::Stunned;
			report.Check(normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger stuns a normal enemy hit by the burst");
			report.Check(!elite->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger leaves an elite enemy unstunned");
			report.Check(!miniBoss->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger leaves a miniboss unstunned");
			report.Check(!boss->GetAbilitySystemComponent().HasOwnedTag(stunned), "Stagger leaves a boss unstunned");
			report.CheckValue("the burst still deals 25 percent of the owner's 400 MaxHealth (large size, level one)", kShatterFractionLarge[0] * 400.f,
				normalStart - Health(*normal), "Stagger changes no damage");
			Advance(f, 1.0f);
			report.Check(!normal->GetAbilitySystemComponent().HasOwnedTag(stunned), "the Stagger stun ends after one second");

			Fixture plain{ report, application };
			plain.Install(kShatter, kLarge, 1);
			SetOwnerMaxHealth(plain, 400.f);
			auto plainNormal = ZeroShieldEnemy(plain, 100.f);
			auto plainAttacker = ZeroShieldEnemy(plain, 400.f);
			Advance(plain, 0.1f);
			Break(report, plain, plainAttacker.get());
			Advance(plain, 0.1f);
			report.Check(!plainNormal->GetAbilitySystemComponent().HasOwnedTag(stunned), "without Stagger the burst does not stun");
		}

		// The first wave pushes a target about 177 units out before the 1.5 s echo fires (linearDamping 0.7, impulse 150). The echo
		// only reaches targets still inside the 250 radius, so the target starts at 50 units: 50 + 177 stays inside.
		float AftershockDistance(Report& report, Application& application, const char* side)
		{
			Fixture f{ report, application };
			f.Install(kShatter, kLarge, 1, side);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, kAftershockTargetDistance);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			Break(report, f, attacker.get());
			Advance(f, 1.4f);
			report.Check(Distance(*f.ship, *enemy) < kShatterRadius, "the target is still inside the 250 radius when the Aftershock fires",
				Json{ { "side", side }, { "distance", Distance(*f.ship, *enemy) } });
			Advance(f, 1.6f);
			return Distance(*f.ship, *enemy);
		}

		// Aftershock Wave: one second wave 1.5 seconds after the burst. It adds no damage, but it pushes again.
		void ShatterAftershockChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			f.Install(kShatter, kLarge, 1, kAftershock);
			SetOwnerMaxHealth(f, 400.f);
			auto enemy = ZeroShieldEnemy(f, 100.f);
			auto attacker = ZeroShieldEnemy(f, 400.f);
			Advance(f, 0.1f);
			const float start = Health(*enemy);
			Break(report, f, attacker.get());
			report.CheckValue("the first wave deals 25 percent of MaxHealth", kShatterFractionLarge[0] * 400.f, start - Health(*enemy), "main rule");
			report.CheckValue("one Aftershock wave is scheduled", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo schedule");
			Advance(f, 1.0f);
			report.CheckValue("the Aftershock waits 1.5 seconds", 1.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo delay");
			Advance(f, 0.6f);
			report.CheckValue("the Aftershock wave has fired", 0.f, static_cast<float>(f.ship->GetModuleApplier().GetOwnerBurstEchoCount()), "echo fired");
			report.CheckValue("the Aftershock wave deals no damage (only the first wave)", kShatterFractionLarge[0] * 400.f, start - Health(*enemy), "echoFraction 0");

			const float withWave = AftershockDistance(report, application, kAftershock);
			const float withoutWave = AftershockDistance(report, application, "");
			report.Check(withWave - withoutWave > 1.f, "the Aftershock wave pushes the enemy further away than the burst alone",
				Json{ { "withAftershock", withWave }, { "withoutAftershock", withoutWave } });
		}

		// Sonic probe: the same resolver the Primary fire action uses, read on the live ship.
		struct PrimaryValues
		{
			float damage = 0.f;
			float speed = 0.f;
			float pierce = 0.f;
		};

		bool NearSonic(float actual, float expected)
		{
			return std::abs(actual - expected) <= 1e-3f * std::max(1.f, std::abs(expected)) + 1e-3f;
		}

		sas::GameplayAttributeList Probe()
		{
			return sas::GameplayAttributeList{
				sas::GameplayAttribute{ CommonAttributeIds::Damage, kProbeDamage, 0.f, 100000.f },
				sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, kProbeSpeed, 0.f, 100000.f },
				sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 0.f, 0.f, 100.f }
			};
		}

		PrimaryValues ResolvePrimary(PlayerSpaceShip& ship)
		{
			GameAbility* primary = ship.GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::PrimaryFire);
			const FireWeaponAction* action = nullptr;
			if (primary)
			{
				for (const AbilityActionSpec& spec : primary->GetDefinition().actions)
				{
					if ((action = std::get_if<FireWeaponAction>(&spec.action))) break;
				}
			}
			if (!primary || !action) throw std::runtime_error{ "Rare wave 5: no live PrimaryFire weapon" };
			const PrimaryWeaponDefinition weapon = action->weaponDefinition;
			GameAbilityDefinition definition = primary->GetDefinition();
			AbilityExecutionContext context{ &ship.GetAbilitySystemComponent(), &definition, nullptr, nullptr };
			const sas::GameplayAttributeList values = AbilityActionAttributeResolver::ResolveAttributes(context, &weapon, Probe());
			PrimaryValues result;
			result.damage = sas::FindAttributeValue(values, CommonAttributeIds::Damage, -1.f);
			result.speed = sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::Speed, -1.f);
			result.pierce = sas::FindAttributeValue(values, PrimaryWeaponSchema::Projectile::Delivery::PierceCount, -1.f);
			return result;
		}

		bool SameValues(const PrimaryValues& left, const PrimaryValues& right)
		{
			return NearSonic(left.damage, right.damage) && NearSonic(left.speed, right.speed) && NearSonic(left.pierce, right.pierce);
		}

		// Sonic on: Primary speed x2 (+1100 on the probe) and pierce +1, with the resolved damage stat unchanged.
		bool SonicOn(const PrimaryValues& idle, const PrimaryValues& now)
		{
			return NearSonic(now.speed - idle.speed, kProbeSpeed * (2.f - 1.f)) && NearSonic(now.pierce - idle.pierce, 1.f) && NearSonic(now.damage, idle.damage);
		}

		void Drive(Fixture& f, float speed, int ticks)
		{
			for (int tick = 0; tick < ticks; ++tick)
			{
				f.ship->SetVelocity({ speed, 0.f });
				f.level->TickInternal(kDriveStep);
			}
		}

		// The live cap: one clamped tick at a commanded speed far above any cap.
		float MeasureCap(Fixture& f)
		{
			Drive(f, kFastSpeed, 1);
			return f.ship->GetMovementComponent().GetCurrentMovementSpeed();
		}

		// Afterglow: the sonic state lasts 1.5 seconds after the speed drops; without it the state ends at once.
		void EscapeAfterglowChecks(Report& report, Application& application)
		{
			Fixture plain{ report, application };
			plain.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
			const PrimaryValues base = ResolvePrimary(*plain.ship);
			{
				Fixture f{ report, application };
				f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
				const float cap = MeasureCap(f);
				f.Install(kEscape, kLarge, 1, kAfterglow);
				Drive(f, kFastSpeed, 20);
				report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "Escape Velocity (large) is sonic after 2 seconds above 90 percent of the cap");
				Drive(f, 0.5f * cap, 14);
				report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "Afterglow keeps sonic 1.4 seconds after the speed dropped");
				Drive(f, 0.5f * cap, 2);
				report.Check(SameValues(base, ResolvePrimary(*f.ship)), "Afterglow ends sonic after 1.5 seconds below the threshold");
			}
			{
				Fixture f{ report, application };
				f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
				const float cap = MeasureCap(f);
				f.Install(kEscape, kLarge, 1);
				Drive(f, kFastSpeed, 20);
				report.Check(SonicOn(base, ResolvePrimary(*f.ship)), "without Afterglow the large Escape Velocity is sonic after 2 seconds");
				Drive(f, 0.5f * cap, 1);
				report.Check(SameValues(base, ResolvePrimary(*f.ship)), "without Afterglow slowing down ends sonic at once");
			}
		}

		// Owner burst and status rules are read from their runtimes; they own no counted handle. Sustained speed has no counted
		// accessor either, so the Escape case checks only the handle count.
		bool RuleLive(Fixture& f, const char* id)
		{
			const std::string module{ id };
			if (module == kDying || module == kShatter) return f.ship->GetModuleApplier().GetOwnerBurstRuleCount() == 1;
			if (module == kEmber) return RuleStatusOf(f).thermalBursts.size() == 1;
			if (module == kKinetic) return RuleStatusOf(f).kineticConsumes.size() == 1;
			return true;
		}

		// Install and remove ten times with the side feature: the handle count and the live rule stay fixed for every ladder module.
		void CycleChecks(Report& report, Application& application)
		{
			struct Case { const char* id; int variant; const char* side; };
			const Case cases[] = { { kDying, kLarge, kCollapse }, { kEscape, kLarge, kAfterglow }, { kKinetic, kLarge, kRend }, { kEmber, kLarge, kSmolder }, { kShatter, kLarge, kStagger } };
			for (const Case& item : cases)
			{
				Fixture f{ report, application };
				const std::uint32_t id = f.Install(item.id, item.variant, 1, item.side);
				const ModulePlacement placed = *f.loadout.FindPlacement(id);
				const std::size_t installed = f.ship->GetModuleApplier().GetOwnedHandleCount();
				report.Check(RuleLive(f, item.id), std::string{ "the installed module is live before the cycles: " } + item.id);
				for (int cycle = 0; cycle < 10; ++cycle)
				{
					f.Remove(id);
					report.Check(f.loadout.Install(f.rules, id, placed.origin, placed.rotationQuarterTurns) == ModulePlacementFailure::None &&
						f.ship->GetModuleApplier().Apply(f.loadout, f.rules), "the module reinstalls at its cell during the cycles");
				}
				report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == installed && RuleLive(f, item.id),
					std::string{ "ten remove and reinstall cycles keep the handle count and the live rule: " } + item.id,
					Json{ { "installed", installed }, { "after", f.ship->GetModuleApplier().GetOwnedHandleCount() } });
			}
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_wave5_content_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare wave 5 content registration failed" };
		std::string title = "Rare Wave 5 Content E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		CatalogChecks(report);
		DescriptionChecks(report);
		TrophyCreditChecks(report, application);
		TrophyLevelChecks(report, application);
		TrophyBountyChecks(report, application);
		TrophyWallChecks(report, application);
		TrophyCycleChecks(report, application);
		DyingLevelChecks(report, application);
		DyingRadiusChecks(report, application);
		DyingCollapseChecks(report, application);
		DyingRedGiantChecks(report, application);
		EmberAreaChecks(report, application);
		EmberSmolderChecks(report, application);
		KineticRendChecks(report, application);
		ShatterStaggerChecks(report, application);
		ShatterAftershockChecks(report, application);
		EscapeAfterglowChecks(report, application);
		CycleChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareWave5ContentE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
