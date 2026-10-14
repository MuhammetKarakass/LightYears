#include "UncommonModuleP4E2E.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/MovementComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleLoader.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/damage/DamageHitGroup.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "spaceShip/SpaceShip.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace rare_ladder_e2e
{
	using namespace uncommon_p4_e2e;
	using Json = common_module_e2e::Json;

	namespace
	{
		// Shipped content only: every module and side feature below is read from the real modules.json and module_side_features.json.
		constexpr const char* kEmber = "Module.Offense.EmberAccumulation.Rare";
		constexpr const char* kShatter = "Module.Defense.ShatterShock.Rare";
		constexpr const char* kKinetic = "Module.Offense.KineticBreaker.Rare";
		constexpr const char* kGiant = "Module.Offense.GiantSlayer.Rare";
		constexpr const char* kEscape = "Module.Movement.EscapeVelocity.Rare";

		constexpr const char* kSideO4 = "Module.SideFeature.Shared.O4.Rare";
		constexpr const char* kSideO5 = "Module.SideFeature.Shared.O5.Rare";
		constexpr const char* kSideD4 = "Module.SideFeature.Shared.D4.Rare";
		constexpr const char* kSideR2 = "Module.SideFeature.Shared.R2";
		constexpr const char* kSideO2 = "Module.SideFeature.Shared.O2.Rare";
		constexpr const char* kSideR5 = "Module.SideFeature.Shared.R5";
		constexpr const char* kSideR3 = "Module.SideFeature.Shared.R3";
		constexpr const char* kSideG4 = "Module.SideFeature.Shared.G4.Rare";
		constexpr const char* kSideR6 = "Module.SideFeature.Shared.R6";
		constexpr const char* kHairTrigger = "Module.SideFeature.KineticBreaker.HairTrigger";
		constexpr const char* kToppler = "Module.SideFeature.GiantSlayer.Toppler";
		constexpr const char* kUnderdog = "Module.SideFeature.GiantSlayer.Underdog";

		constexpr std::array<float, 3> kKineticMultiplier{ 1.10f, 1.15f, 1.20f };
		constexpr std::array<float, 3> kGiantBase{ 1.10f, 1.15f, 1.20f };
		constexpr std::array<float, 3> kEscapeMultiplier{ 1.10f, 1.15f, 1.20f };
		constexpr std::array<float, 3> kEmberFraction{ 1.30f, 1.40f, 1.50f };
		constexpr std::array<float, 3> kShatterSmallFraction{ 0.10f, 0.16f, 0.22f };
		constexpr std::array<float, 3> kShatterLargeFraction{ 0.25f, 0.31f, 0.37f };
		constexpr float kOwnerMaxHealth = 400.f;
		constexpr float kProbeDamage = 10.f;
		constexpr float kDriveStep = 0.1f;
		// Commanded far above any cap: the ThrustDrift clamp leaves the ship exactly at its live cap.
		constexpr float kFastSpeed = 100000.f;

		sas::ContentId ContentIdOf(const char* id)
		{
			return sas::ContentId{ std::string{ id } };
		}

		bool Near(float left, float right, float tolerance)
		{
			return std::abs(left - right) <= tolerance;
		}

		DamagePayload Thermal()
		{
			DamagePayload payload = Plain();
			payload.igniteStacks = 1;
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
			return Hit(target, source, 1.f, Thermal(), {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Thermal });
		}

		float KineticHit(SpaceShip& target, Actor* source)
		{
			return Hit(target, source, 1.f, Kinetic(1), {}, {}, DamageDeliveryType::Direct, { DamageTypeSchema::Kinetic });
		}

		float Health(SpaceShip& ship)
		{
			return ship.GetHealthComponent().GetHealth();
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

		void Advance(Fixture& f, float seconds)
		{
			const int ticks = static_cast<int>(std::round(seconds / kDriveStep));
			for (int tick = 0; tick < ticks; ++tick) f.level->TickInternal(kDriveStep);
		}

		// Health removed by one plain hit of the owner against a fresh, unshielded, unarmored enemy far from the others.
		float GaugeHit(Fixture& f, float damage)
		{
			const auto gauge = Enemy(f);
			gauge->SetActorLocation({ 30000.f, 30000.f });
			return Hit(*gauge, f.ship.get(), damage, Plain());
		}

		float OutgoingScale(Fixture& f)
		{
			return GaugeHit(f, 1.f);
		}

		sas::AttributeId G4Attribute()
		{
			return content::ModuleSideFeatureCatalog::FindById(ContentIdOf(kSideG4))->rules.attributeModifiers[0].modifier.attributeId;
		}

		// Enemy whose hull MaxHealth is ratio times the owner's hull MaxHealth, restored to full health.
		shared_ptr<SpaceShip> RatioTarget(Fixture& f, float ratio)
		{
			const float ownerMax = f.ship->GetHealthComponent().GetMaxHealth();
			const auto target = Enemy(f);
			target->GetHealthComponent().SetMaxHealth(ratio * ownerMax);
			target->GetHealthComponent().Regenerate(ratio * ownerMax);
			return target;
		}

		// Health removed by one plain hit, after which the target is restored to full health.
		float Dealt(Fixture& f, SpaceShip& target, float damage = 100.f)
		{
			const float dealt = Hit(target, f.ship.get(), damage, Plain());
			target.GetHealthComponent().Regenerate(target.GetHealthComponent().GetMaxHealth());
			return dealt;
		}

		// Health removed by one incoming hit from source, with the owner shield emptied first.
		float Incoming(Fixture& f, Actor& source, float damage = 100.f)
		{
			f.ship->GetShieldComponent().ChangeShield(-f.ship->GetShieldComponent().GetShield());
			const float dealt = Hit(*f.ship, &source, damage, Plain());
			f.ship->GetHealthComponent().Regenerate(f.ship->GetHealthComponent().GetMaxHealth());
			return dealt;
		}

		void SetShield(SpaceShip& ship, float value)
		{
			auto& shield = ship.GetShieldComponent();
			shield.ChangeShield(-shield.GetShield());
			if (value > 0.f) shield.ChangeShield(value);
		}

		// Breaks the owner's shield with one hit from source and checks that the shield actually reached zero.
		void Break(Report& report, Fixture& f, Actor* source)
		{
			SetShield(*f.ship, 10.f);
			Hit(*f.ship, source, 30.f, Plain());
			report.Check(f.ship->GetShieldComponent().GetShield() <= 0.f, "owner shield reaches zero in the break",
				Json{ { "shield", f.ship->GetShieldComponent().GetShield() } });
		}

		void SetOwnerMaxHealth(Fixture& f, float maxHealth)
		{
			f.ship->GetHealthComponent().SetMaxHealth(maxHealth);
			f.ship->GetHealthComponent().Regenerate(10000.f);
		}

		// The shared Enemy() fixture leaves shield regeneration running, so burst targets get zero shield capacity.
		auto ZeroShieldEnemy(Fixture& f, float distance)
		{
			auto enemy = Enemy(f, distance);
			enemy->GetShieldComponent().SetMaxShield(0.f);
			return enemy;
		}

		bool HasSlow(SpaceShip& target)
		{
			return target.GetAbilitySystemComponent().FindGameplayEffectById(MovementEffectSchema::SlowEffectId) != nullptr;
		}

		float SlowMagnitude(SpaceShip& target)
		{
			return target.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow);
		}

		float Shot(Actor& target, PlayerSpaceShip& ship, float damage, bool primary)
		{
			DamagePayload payload = Plain();
			payload.hitGroup = std::make_shared<DamageHitGroup>(primary ? DamageHitGroup::Kind::PrimaryShot : DamageHitGroup::Kind::AbilityUse);
			return Hit(target, &ship, damage, payload, sas::ContentId{}, primary ? List<GameplayTag>{ GameplayTags::Ability::Primary } : List<GameplayTag>{});
		}

		void Drive(Fixture& f, float speed, int ticks)
		{
			for (int tick = 0; tick < ticks; ++tick)
			{
				f.ship->SetVelocity({ speed, 0.f });
				f.level->TickInternal(kDriveStep);
			}
		}

		// The live cap: one clamped tick. The module does not change the cap, so it can be read before installing.
		float MeasureCap(Fixture& f)
		{
			Drive(f, kFastSpeed, 1);
			return f.ship->GetMovementComponent().GetCurrentMovementSpeed();
		}

		// Number of resolved damage rules the applier carries for one installed instance.
		int ResolvedRuleCount(const Fixture& f, std::uint32_t instance)
		{
			int count = 0;
			for (const ResolvedModuleDamageRule& damage : f.ship->GetModuleApplier().GetResolvedEffects().damageRules)
				if (damage.instanceId == instance) ++count;
			return count;
		}

		int SharedChoiceCount(const ModuleDefinition& definition)
		{
			int count = 0;
			for (const auto& pool : definition.sharedPools) count += static_cast<int>(pool.allowedFeatureIds.size());
			return count;
		}

		void CatalogChecks(Report& report)
		{
			const auto loaded = content::ModuleLoader::LoadFromFile(
				std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets/content/data/modules.json");
			report.Check(loaded.Succeeded(), "shipped modules.json loads through the real ModuleLoader", loaded.error);

			struct Subject { const char* id; const char* family; std::size_t variants; std::size_t privateCount; std::size_t sharedCount; };
			const Subject subjects[] = {
				{ kEmber, "Ability.Offense", 2, 2, 3 },
				{ kShatter, "Ability.Defense", 2, 2, 3 },
				{ kKinetic, "Ability.Offense", 2, 2, 3 },
				{ kGiant, "Ability.Offense", 2, 2, 3 },
				{ kEscape, "Ability.Movement", 2, 2, 3 },
			};
			for (const Subject& subject : subjects)
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(ContentIdOf(subject.id));
				report.Check(definition != nullptr, std::string{ "catalog contains " } + subject.id);
				report.Check(definition->rarity == ModuleRarity::Rare && definition->levelCount == 3 && definition->family.name == subject.family,
					std::string{ "Rare, three levels, expected family: " } + subject.id);
				report.Check(definition->sizeVariants.size() == subject.variants && definition->sizeLadderCellCounts.size() == subject.variants,
					std::string{ "two size variants with an explicit ladder: " } + subject.id,
					Json{ { "variants", definition->sizeVariants.size() } });
				for (std::size_t variant = 0; variant < definition->sizeVariants.size(); ++variant)
					for (int level = 1; level <= 3; ++level)
						report.Check(HasModuleRulePackageLevelCoverage(definition->sizeVariants[variant], level),
							std::string{ "level table covers level " } + std::to_string(level) + ": " + subject.id);
				report.Check(!HasModuleRulePackageLevelCoverage(definition->sizeVariants[1], 4), std::string{ "no level beyond three: " } + subject.id);
				report.Check(definition->sideFeatures.size() == subject.privateCount && SharedChoiceCount(*definition) == static_cast<int>(subject.sharedCount),
					std::string{ "private and shared side choices match the design: " } + subject.id,
					Json{ { "private", definition->sideFeatures.size() }, { "shared", SharedChoiceCount(*definition) } });
				report.Check(definition->sizeVariants[0].shape.cells.size() < definition->sizeVariants[1].shape.cells.size(),
					std::string{ "the small variant has fewer cells than the large one: " } + subject.id);
			}

			for (const char* feature : { kHairTrigger, kToppler, kUnderdog })
				report.Check(content::ModuleSideFeatureCatalog::FindById(ContentIdOf(feature)) != nullptr, std::string{ "side feature loads: " } + feature);
		}

		void DescriptionChecks(Report& report)
		{
			struct Text { const char* id; int variant; std::array<const char*, 3> expected; };
			const Text texts[] = {
				{ kEmber, 1, { "130%", "140%", "150%" } },
				{ kShatter, 0, { "10%", "16%", "22%" } },
				{ kShatter, 1, { "25%", "31%", "37%" } },
				{ kKinetic, 1, { "10%", "15%", "20%" } },
				{ kGiant, 1, { "10%", "15%", "20%" } },
				{ kEscape, 1, { "10%", "15%", "20%" } },
			};
			for (const Text& text : texts)
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(ContentIdOf(text.id));
				report.Check(definition != nullptr, std::string{ "catalog contains " } + text.id);
				for (int level = 1; level <= 3; ++level)
				{
					const std::string formatted = FormatModuleDescription(*definition, definition->sizeVariants[static_cast<std::size_t>(text.variant)], level);
					const std::string label = std::string{ "description is fully formatted at level " } + std::to_string(level) + ": " + text.id;
					report.Check(formatted.find('{') == std::string::npos && formatted.find('}') == std::string::npos, label, Json{ { "text", formatted } });
					report.Check(formatted.find(text.expected[static_cast<std::size_t>(level - 1)]) != std::string::npos,
						std::string{ "description shows the level " } + std::to_string(level) + " value: " + text.id, Json{ { "text", formatted } });
				}
			}

			const struct { const char* id; const char* expected; } sides[] = {
				{ kHairTrigger, "3 stacks" }, { kToppler, "2,5" }, { kUnderdog, "8%" },
			};
			for (const auto& side : sides)
			{
				const ModuleSideFeatureDefinition* feature = content::ModuleSideFeatureCatalog::FindById(ContentIdOf(side.id));
				report.Check(feature != nullptr, std::string{ "side feature catalog contains " } + side.id);
				const std::string formatted = FormatModuleSideFeatureDescription(*feature, 1);
				report.Check(formatted.find('{') == std::string::npos && formatted.find(side.expected) != std::string::npos,
					std::string{ "side description is formatted: " } + side.id, Json{ { "text", formatted } });
			}
		}

		// Ember Accumulation: the burst is identical at both sizes; Opening Hit (O5) acts only on the large size.
		void EmberChecks(Report& report, Application& application)
		{
			const auto& balance = ly::content::DamageStatusBalanceCatalog::Get();
			const float totalDoT = balance.ThermalDamagePerSecond(balance.thermal.maxStacks) * balance.thermal.duration;
			for (int level = 1; level <= 3; ++level)
			{
				for (int variant = 0; variant < 2; ++variant)
				{
					Fixture f{ report, application };
					const float outgoing = OutgoingScale(f);
					f.Install(kEmber, variant, level);
					const auto target = Enemy(f);
					for (int stack = 0; stack < 3; ++stack) ThermalHit(*target, f.ship.get());
					const float beforeFourth = Health(*target);
					Advance(f, 0.1f);
					const float fourthDrop = ThermalHit(*target, f.ship.get());
					const float expected = outgoing * (1.f + kEmberFraction[static_cast<std::size_t>(level - 1)] * totalDoT);
					const std::string label = "Ember Accumulation level " + std::to_string(level) + " size " + std::to_string(variant + 1);
					report.CheckValue(label + ": the fourth stack bursts for its level fraction of the total DoT", expected, fourthDrop, "thermal burst");
					report.Check(StackCount(*target, DamageStatusEffectIds::IgniteEffectId) == 0 && !HasEffect(*target, DamageStatusEffectIds::IgniteEffectId),
						label + ": the burst clears the Thermal stacks", Json{ { "beforeFourth", beforeFourth } });
					const float afterBurst = Health(*target);
					Advance(f, 1.f);
					report.Check(std::abs(afterBurst - Health(*target)) < 0.01f, label + ": no Ignite DoT is delivered after the burst");
				}
			}

			// Opening Hit: the first hit on a fresh enemy is x1.25 on the large size only; Lasting Status (O4) does not change the hit.
			const auto firstRatio = [&report, &application](int variant, const char* side) {
				Fixture f{ report, application };
				f.Install(kEmber, variant, 1, side);
				const auto target = Enemy(f);
				const float first = Dealt(f, *target, 10.f);
				const float firstAgain = Dealt(f, *target, 10.f);
				return first / firstAgain;
			};
			report.CheckValue("Opening Hit on the large size: the first hit is x1.25", 1.25f, firstRatio(1, kSideO5), "large size with O5.Rare");
			report.CheckValue("Opening Hit on the small size is absent", 1.f, firstRatio(0, kSideO5), "small size with O5.Rare");
			report.CheckValue("Lasting Status on the large size leaves the first hit unchanged", 1.f, firstRatio(1, kSideO4), "large size with O4.Rare");
		}

		// Shatter Shock: the burst scales with the size; Reactive Plating (R2) acts only on the large size.
		void ShatterChecks(Report& report, Application& application)
		{
			for (int variant = 0; variant < 2; ++variant)
			{
				for (int level = 1; level <= 3; ++level)
				{
					Fixture f{ report, application };
					// Install re-syncs the owner health max from its MaxHealth attribute (250 base), so MaxHealth is set after install.
					f.Install(kShatter, variant, level);
					SetOwnerMaxHealth(f, kOwnerMaxHealth);
					const auto target = ZeroShieldEnemy(f, 100.f);
					const auto attacker = Enemy(f, 600.f);
					const float before = Health(*target);
					Break(report, f, attacker.get());
					const float drop = before - Health(*target);
					const float fraction = variant == 0 ? kShatterSmallFraction[static_cast<std::size_t>(level - 1)] : kShatterLargeFraction[static_cast<std::size_t>(level - 1)];
					report.CheckValue("Shatter Shock size " + std::to_string(variant + 1) + " level " + std::to_string(level) + " bursts for its fraction of owner MaxHealth",
						fraction * kOwnerMaxHealth, drop, "shield break burst");
				}
			}

			{
				Fixture f{ report, application };
				f.Install(kShatter, 1, 1, kSideR2);
				SetOwnerMaxHealth(f, kOwnerMaxHealth);
				const float armorBase = f.Owner(OwnerAttributeIds::Armor);
				Break(report, f, Enemy(f, 600.f).get());
				report.CheckValue("Reactive Plating on the large size adds Armor 30 after the break", armorBase + 30.f, f.Owner(OwnerAttributeIds::Armor), "R2 on 3 cells");
			}
			{
				Fixture f{ report, application };
				f.Install(kShatter, 0, 1, kSideR2);
				SetOwnerMaxHealth(f, kOwnerMaxHealth);
				const float armorBase = f.Owner(OwnerAttributeIds::Armor);
				Break(report, f, Enemy(f, 600.f).get());
				report.CheckValue("Reactive Plating on the small size is absent", armorBase, f.Owner(OwnerAttributeIds::Armor), "R2 on 1 cell");
			}
			{
				Fixture f{ report, application };
				f.Install(kShatter, 1, 1, kSideD4);
				SetOwnerMaxHealth(f, kOwnerMaxHealth);
				const float armorBase = f.Owner(OwnerAttributeIds::Armor);
				Break(report, f, Enemy(f, 600.f).get());
				report.CheckValue("Shield Recharge (D4) does not add Armor on break", armorBase, f.Owner(OwnerAttributeIds::Armor), "D4 on 3 cells");
			}
		}

		// Kinetic Breaker: four stacks spend for the level multiplier and ignore Armor; Hair Trigger spends at three for Normal targets.
		void KineticChecks(Report& report, Application& application)
		{
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				const float baseline = GaugeHit(f, kProbeDamage);
				f.Install(kKinetic, 1, level);
				const auto target = Enemy(f);
				target->GetAbilitySystemComponent().GetAttributes().SetBaseValue(OwnerAttributeIds::Armor, 100.f);
				for (int stack = 0; stack < 4; ++stack) KineticHit(*target, f.ship.get());
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 4, "four Kinetic stacks are present", Json{ { "level", level } });
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.CheckValue("Kinetic Breaker level " + std::to_string(level) + " spends four stacks for its multiplier and ignores Armor 100",
					baseline * kKineticMultiplier[static_cast<std::size_t>(level - 1)], spend, "large size, four stacks");
				report.Check(!HasEffect(*target, DamageStatusEffectIds::KineticEffectId), "the spend clears every Kinetic stack", Json{ { "level", level } });
			}

			{
				Fixture f{ report, application };
				const float baseline = GaugeHit(f, kProbeDamage);
				f.Install(kKinetic, 1, 1, kHairTrigger);
				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) KineticHit(*target, f.ship.get());
				const float spend = Hit(*target, f.ship.get(), 10.f, Plain());
				report.CheckValue("Hair Trigger: a Normal target spends at three stacks for x1.10", baseline * 1.10f, spend, "large size, three stacks");
				report.Check(!HasEffect(*target, DamageStatusEffectIds::KineticEffectId), "Hair Trigger spend clears the stacks");
			}
			{
				Fixture f{ report, application };
				const float baseline = GaugeHit(f, kProbeDamage);
				f.Install(kKinetic, 1, 1, kHairTrigger);
				const auto elite = Enemy(f);
				elite->SetControlTargetClass(ControlTargetClass::Elite);
				for (int stack = 0; stack < 3; ++stack) KineticHit(*elite, f.ship.get());
				Hit(*elite, f.ship.get(), 1.f, Plain());
				report.Check(StackCount(*elite, DamageStatusEffectIds::KineticEffectId) == 3, "Hair Trigger leaves an Elite target at three stacks", Json{ { "stacks", StackCount(*elite, DamageStatusEffectIds::KineticEffectId) } });
				KineticHit(*elite, f.ship.get());
				const float spend = Hit(*elite, f.ship.get(), 10.f, Plain());
				report.CheckValue("Hair Trigger: an Elite target still spends at four stacks", baseline * 1.10f, spend, "large size, Elite four stacks");
			}
			{
				Fixture f{ report, application };
				const float baseline = GaugeHit(f, kProbeDamage);
				f.Install(kKinetic, 0, 1, kHairTrigger);
				const auto target = Enemy(f);
				for (int stack = 0; stack < 3; ++stack) KineticHit(*target, f.ship.get());
				const float plain = Hit(*target, f.ship.get(), 10.f, Plain());
				report.CheckValue("Hair Trigger on the small size is absent: three stacks do not spend", 1.f, plain / baseline, "small size, three stacks");
				report.Check(StackCount(*target, DamageStatusEffectIds::KineticEffectId) == 3, "small size keeps three Kinetic stacks", Json{ { "stacks", StackCount(*target, DamageStatusEffectIds::KineticEffectId) } });
			}
		}

		// Giant Slayer: the ratio multiplier grows with ratioSlope; Toppler and Underdog are each active only on the large size.
		void GiantChecks(Report& report, Application& application)
		{
			const float ratios[] = { 1.5f, 3.f };
			for (int level = 1; level <= 3; ++level)
			{
				Fixture f{ report, application };
				f.Install(kGiant, 1, level);
				const float low = Dealt(f, *RatioTarget(f, 1.f));
				const float atThreshold = Dealt(f, *RatioTarget(f, ratios[0]));
				const float high = Dealt(f, *RatioTarget(f, ratios[1]));
				const float base = kGiantBase[static_cast<std::size_t>(level - 1)];
				report.CheckValue("Giant Slayer level " + std::to_string(level) + " at ratio 1.5 gives its base multiplier", base, atThreshold / low, "large size, ratio 1.5");
				report.CheckValue("Giant Slayer level " + std::to_string(level) + " at ratio 3 adds 0.05 per extra unit of ratio", base + 0.05f * (3.f - 1.5f), high / low, "large size, ratio 3");
			}

			{
				Fixture f{ report, application };
				f.Install(kGiant, 0, 1);
				const float low = Dealt(f, *RatioTarget(f, 1.f));
				const float high = Dealt(f, *RatioTarget(f, 3.f));
				report.CheckValue("Giant Slayer base multiplier is the same on the small size", 1.175f, high / low, "small size, ratio 3");
			}

			{
				Fixture f{ report, application };
				f.Install(kGiant, 1, 1, kToppler);
				const auto below = RatioTarget(f, 2.4f);
				Dealt(f, *below);
				report.Check(!HasSlow(*below), "Toppler: a ratio of 2.4 does not slow the target");
				const auto slowed = RatioTarget(f, 2.5f);
				Dealt(f, *slowed);
				report.Check(HasSlow(*slowed), "Toppler: a ratio of 2.5 slows the target");
				report.CheckValue("Toppler: a Normal target slows by 30 percent", 0.3f, SlowMagnitude(*slowed), "large size, Normal target");
				const auto boss = RatioTarget(f, 2.5f);
				boss->SetControlTargetClass(ControlTargetClass::Boss);
				Dealt(f, *boss);
				report.CheckValue("Toppler: a Boss slows by 15 percent", 0.15f, SlowMagnitude(*boss), "large size, Boss target");
			}
			{
				Fixture f{ report, application };
				f.Install(kGiant, 0, 1, kToppler);
				const auto slowed = RatioTarget(f, 2.5f);
				Dealt(f, *slowed);
				report.Check(!HasSlow(*slowed), "Toppler on the small size is absent: a ratio of 2.5 does not slow");
			}
			{
				Fixture f{ report, application };
				const float ownerMax = f.ship->GetHealthComponent().GetMaxHealth();
				const auto source = Enemy(f, 300.f);
				source->GetHealthComponent().SetMaxHealth(1.5f * ownerMax);
				source->GetHealthComponent().Regenerate(1.5f * ownerMax);
				const float baseline = Incoming(f, *source);
				f.Install(kGiant, 1, 1, kUnderdog);
				const float underdog = Incoming(f, *source);
				report.CheckValue("Underdog: a larger attacker deals x0.92 to the owner", 0.92f, underdog / baseline, "large size, attacker 1.5x MaxHealth");
				const auto target = RatioTarget(f, 2.5f);
				Dealt(f, *target);
				report.Check(!HasSlow(*target), "Underdog is not Toppler: the same ratio target is not slowed");
			}
		}

		// Escape Velocity: sonic after two seconds above 90% of the cap; the Primary multiplier is by level. G4 and R6 differ.
		void EscapeChecks(Report& report, Application& application)
		{
			for (int variant = 0; variant < 2; ++variant)
			{
				Fixture f{ report, application };
				f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
				const float cap = MeasureCap(f);
				report.Check(cap > 1.f, "the ship's live speed cap measures above zero", Json{ { "cap", cap } });
				const float idle = Shot(*Enemy(f), *f.ship, kProbeDamage, true);
				f.Install(kEscape, variant, 1);
				// MeasureCap and the idle shot leave the ship above 90% of cap; drop below it so the hold counts from the install.
				Drive(f, 0.5f * cap, 30);
				Drive(f, kFastSpeed, 19);
				report.CheckValue("Escape Velocity size " + std::to_string(variant + 1) + ": 1.9 seconds above the cap is not sonic yet",
					1.f, Shot(*Enemy(f), *f.ship, kProbeDamage, true) / idle, "before the hold");
				Drive(f, kFastSpeed, 1);
				report.CheckValue("Escape Velocity size " + std::to_string(variant + 1) + ": the sonic Primary shot is x1.10 at level 1",
					kEscapeMultiplier[0], Shot(*Enemy(f), *f.ship, kProbeDamage, true) / idle, "after two seconds");
				Drive(f, 0.5f * cap, 1);
				report.CheckValue("Escape Velocity size " + std::to_string(variant + 1) + ": slowing down ends sonic at once",
					1.f, Shot(*Enemy(f), *f.ship, kProbeDamage, true) / idle, "after slowing down");
			}

			for (int level = 2; level <= 3; ++level)
			{
				Fixture f{ report, application };
				f.ship->SetMovementMode(ShipMovementMode::ThrustDrift);
				const float cap = MeasureCap(f);
				const float idle = Shot(*Enemy(f), *f.ship, kProbeDamage, true);
				f.Install(kEscape, 1, level);
				Drive(f, kFastSpeed, 20);
				report.CheckValue("Escape Velocity level " + std::to_string(level) + " sonic Primary multiplier from its own table",
					kEscapeMultiplier[static_cast<std::size_t>(level - 1)], Shot(*Enemy(f), *f.ship, kProbeDamage, true) / idle, "level table");
				Drive(f, 0.5f * cap, 1);
			}

			{
				Fixture f{ report, application };
				const float before = f.Owner(G4Attribute());
				f.Install(kEscape, 1, 1, kSideG4);
				report.CheckValue("General G4 on the large size raises the horizontal movement rating by 1.4", 1.4f,
					f.Owner(G4Attribute()) - before, "large size with G4.Rare");
			}
			{
				Fixture f{ report, application };
				const float before = f.Owner(G4Attribute());
				f.Install(kEscape, 0, 1, kSideG4);
				report.CheckValue("General G4 on the small size is absent", 0.f, f.Owner(G4Attribute()) - before, "small size with G4.Rare");
			}
			{
				Fixture f{ report, application };
				const std::uint32_t instance = f.Install(kEscape, 1, 1, kSideR6);
				report.Check(ResolvedRuleCount(f, instance) == 1, "Fleet Guard (R6) on the large size registers one incoming damage rule",
					Json{ { "rules", ResolvedRuleCount(f, instance) } });
			}
			{
				Fixture f{ report, application };
				const std::uint32_t instance = f.Install(kEscape, 0, 1, kSideR6);
				report.Check(ResolvedRuleCount(f, instance) == 0, "Fleet Guard (R6) on the small size is absent from the resolved output",
					Json{ { "rules", ResolvedRuleCount(f, instance) } });
			}
		}

		// Install and remove of every ladder module with its large-size side feature: ten rounds keep the handle counts fixed.
		void HandleChecks(Report& report, Application& application)
		{
			Fixture f{ report, application };
			const std::size_t baselineHandles = f.ship->GetModuleApplier().GetOwnedHandleCount();
			const std::size_t baselineModifiers = f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount();
			struct Case { const char* id; const char* side; };
			const Case cases[] = { { kEmber, kSideO5 }, { kShatter, kSideR2 }, { kKinetic, kHairTrigger }, { kGiant, kToppler }, { kEscape, kSideG4 } };
			for (int cycle = 0; cycle < 10; ++cycle)
			{
				for (const Case& item : cases)
				{
					const std::uint32_t instance = f.Install(item.id, 1, 1 + cycle % 3, item.side);
					f.Remove(instance);
				}
				report.Check(f.ship->GetModuleApplier().GetOwnedHandleCount() == baselineHandles &&
					f.ship->GetModuleTriggerRuntime().GetActiveModifierHandleCount() == baselineModifiers,
					"ten install and remove rounds of the five ladder modules leak no handles", Json{ { "cycle", cycle } });
			}
		}
	}
}

int main(int argc, char** argv)
{
	using namespace rare_ladder_e2e;
	if (argc != 2) return 2;
	Report report;
	try
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Rare ladder content registration failed" };
		std::string title = "Rare Ladder Modules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		CatalogChecks(report);
		DescriptionChecks(report);
		EmberChecks(report, application);
		ShatterChecks(report, application);
		KineticChecks(report, application);
		GiantChecks(report, application);
		EscapeChecks(report, application);
		HandleChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& exception) { report.artifact["error"] = exception.what(); std::cerr << exception.what() << '\n'; }
	report.artifact["test"] = "RareLadderModulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
