#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/tags/GameplayTags.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace uncommon_p6_e2e;
using namespace vega;

namespace
{
	constexpr const char* kPrism = "Module.Offense.PrismCore.Epic";
	constexpr const char* kBlood = "Module.Offense.BloodContract.Epic";
	constexpr const char* kStormSide = "Module.SideFeature.PrismCore.StormCapacitor";
	constexpr const char* kAlternatingSide = "Module.SideFeature.PrismCore.AlternatingCurrent";
	constexpr const char* kClotSide = "Module.SideFeature.BloodContract.ClotArmor";
	constexpr const char* kDebtSide = "Module.SideFeature.BloodContract.DebtCollection";
	constexpr float kTolerance = 0.01f;

	// Scaling is already resolved, so the owner's AttackPower does not change the ratios these checks compare.
	DamagePayload Scaled()
	{
		DamagePayload payload = Plain();
		payload.sourceScalingPolicy = DamageSourceScalingPolicy::AlreadyResolved;
		return payload;
	}

	// Level at which a placement resolves (placement level plus its zone bonus, as the resolver does).
	int ResolvedLevel(Fixture& f, std::uint32_t instanceId, int placementLevel)
	{
		return placementLevel + ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), instanceId);
	}

	// Linear table read with extrapolation past the last authored entry, as ModuleLevelTable does.
	float TableAt(const float* values, int count, int level)
	{
		if (level <= count) return values[level - 1];
		return values[count - 1] + (values[count - 1] - values[count - 2]) * static_cast<float>(level - count);
	}

	float PrismMultiplier(int level)
	{
		const float values[] = { 1.f, 1.15f, 1.3f };
		return TableAt(values, 3, level);
	}

	template<typename Owner, typename Id>
	int Stacks(Owner& owner, const Id& id)
	{
		const auto* effect = owner.GetAbilitySystemComponent().FindGameplayEffectById(id);
		return effect ? effect->stackCount : 0;
	}

	template<typename Owner, typename Id>
	float Duration(Owner& owner, const Id& id)
	{
		const auto* effect = owner.GetAbilitySystemComponent().FindGameplayEffectById(id);
		return effect ? effect->remainingDuration : 0.f;
	}

	float OwnerArmor(Fixture& f)
	{
		return f.ship->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::Armor);
	}

	// Offense hit with the ability's own tags; damage type tags decide the Alternating Current comparison.
	float OffenseHitOf(Fixture& f, Actor& target, GameAbility& ability, float damage, const GameplayTag& damageType)
	{
		return Hit(target, f.ship.get(), damage, Scaled(), ability.GetDefinition().abilityId, ability.GetDefinition().abilityTags, DamageDeliveryType::Direct, { damageType });
	}

	void PrismChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const std::uint32_t id = f.Install(kPrism, 0, level);
			const float multiplier = PrismMultiplier(ResolvedLevel(f, id, level));
			const auto enemy = Enemy(f);

			// A Primary hit is not an Offense use and must not apply a cycle status.
			Hit(*enemy, f.ship.get(), 50.f, Scaled(), {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
			report.Check(Stacks(*enemy, DamageStatusEffectIds::IgniteEffectId) == 0, "Prism: Primary hit applies no cycle status");

			for (int use = 0; use < 5; ++use)
			{
				auto& ability = CastOffense(f);
				const int before[] = { Stacks(*enemy, DamageStatusEffectIds::IgniteEffectId), Stacks(*enemy, DamageStatusEffectIds::CryoSlowedEffectId),
					Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId), Stacks(*enemy, DamageStatusEffectIds::KineticEffectId) };
				// The hit uses Energy so that only the cycle status can move the damage-status stacks.
				OffenseHitOf(f, *enemy, ability, 10.f, DamageTypeSchema::Energy);
				const int after[] = { Stacks(*enemy, DamageStatusEffectIds::IgniteEffectId), Stacks(*enemy, DamageStatusEffectIds::CryoSlowedEffectId),
					Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId), Stacks(*enemy, DamageStatusEffectIds::KineticEffectId) };
				const int expectedIndex = use % 4;
				bool onlyExpectedMoved = true;
				for (int index = 0; index < 4; ++index) onlyExpectedMoved = onlyExpectedMoved && (after[index] == before[index] + (index == expectedIndex ? 1 : 0));
				report.Check(onlyExpectedMoved, "Prism: use " + std::to_string(use + 1) + " applies only the cycle status (level " + std::to_string(level) + ")");
				if (use == 0)
				{
					const float expected = vega::content::DamageStatusBalanceCatalog::Get().thermal.duration * multiplier;
					report.Check(std::fabs(Duration(*enemy, DamageStatusEffectIds::IgniteEffectId) - expected) < kTolerance, "Prism: thermal cycle duration x" + std::to_string(multiplier));
				}
				if (use == 1)
				{
					const float expected = vega::content::DamageStatusBalanceCatalog::Get().cryo.duration * multiplier;
					report.Check(std::fabs(Duration(*enemy, DamageStatusEffectIds::CryoSlowedEffectId) - expected) < kTolerance, "Prism: cryo cycle duration x" + std::to_string(multiplier));
				}
			}
		}
	}

	void StormChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(kPrism, 1, 1, kStormSide);
		std::vector<decltype(Enemy(f))> enemies;
		for (int index = 0; index < 5; ++index) enemies.push_back(Enemy(f, 100.f + 20.f * static_cast<float>(index)));

		for (int use = 0; use < 3; ++use) CastOffense(f);
		report.Check(f.ship->GetCombatRuntime().GetEpicOffenseState().stormCharges == 3, "Storm: three uses hold three charges");
		Hit(*enemies[0], f.ship.get(), 10.f, Scaled(), {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
		int electrified = 0;
		for (const auto& enemy : enemies) electrified += Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId) > 0 ? 1 : 0;
		report.Check(electrified == 0 && f.ship->GetCombatRuntime().GetEpicOffenseState().stormCharges == 3, "Storm: a Primary hit below four charges discharges nothing");

		CastOffense(f);
		report.Check(f.ship->GetCombatRuntime().GetEpicOffenseState().stormCharges == 4, "Storm: fourth use reaches four charges");
		Hit(*enemies[0], f.ship.get(), 10.f, Scaled(), {}, { GameplayTags::Ability::Primary }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
		electrified = 0;
		int twoStacks = 0;
		for (const auto& enemy : enemies)
		{
			const int stacks = Stacks(*enemy, DamageStatusEffectIds::ElectricEffectId);
			electrified += stacks > 0 ? 1 : 0;
			twoStacks += stacks == 2 ? 1 : 0;
		}
		report.Check(electrified == 3 && twoStacks == 3, "Storm: discharge strikes three enemies with two Electric stacks each");
		report.Check(f.ship->GetCombatRuntime().GetEpicOffenseState().stormCharges == 0, "Storm: discharge spends every charge");
	}

	void AlternatingChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		f.Install(kPrism, 1, 1, kAlternatingSide);
		const auto enemy = Enemy(f);
		auto& ability = CastOffense(f);
		const float first = OffenseHitOf(f, *enemy, ability, 100.f, DamageTypeSchema::Thermal);
		const float second = OffenseHitOf(f, *enemy, ability, 100.f, DamageTypeSchema::Kinetic);
		const float third = OffenseHitOf(f, *enemy, ability, 100.f, DamageTypeSchema::Kinetic);
		const float fourth = OffenseHitOf(f, *enemy, ability, 100.f, DamageTypeSchema::Thermal);
		report.Check(first > 0.f, "Alternating: baseline hit deals damage");
		report.Check(std::fabs(second / first - 1.2f) < 0.001f, "Alternating: a different damage type after the previous direct hit deals x1.20");
		report.Check(std::fabs(third / first - 1.f) < 0.001f, "Alternating: the same damage type again gets no bonus");
		report.Check(std::fabs(fourth / first - 1.2f) < 0.001f, "Alternating: switching back to Thermal gets x1.20");
	}

	void BloodChecks(Report& report, Application& application)
	{
		const float fullValues[] = { 1.2f, 1.25f, 1.3f };
		const float emptyValues[] = { 1.7f, 1.75f, 1.8f };
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const std::uint32_t id = f.Install(kBlood, 0, level);
			const int resolved = ResolvedLevel(f, id, level);
			const float full = TableAt(fullValues, 3, resolved);
			const float empty = TableAt(emptyValues, 3, resolved);
			auto& health = f.ship->GetHealthComponent();
			const float maximum = health.GetMaxHealth();
			const std::string tag = "Blood L" + std::to_string(level) + ": ";

			// Damage scales with missing health: compare a full-health hit with a hit at 10% health.
			health.Regenerate(maximum);
			const auto enemy = Enemy(f);
			const float atFull = Hit(*enemy, f.ship.get(), 100.f, Scaled(), {}, { GameplayTags::Ability::Offense }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
			health.ChangeHealth(-maximum * 0.9f);
			const float atLow = Hit(*enemy, f.ship.get(), 100.f, Scaled(), {}, { GameplayTags::Ability::Offense }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
			const float expectedRatio = (full + (empty - full) * 0.9f) / full;
			report.Check(std::fabs(atLow / atFull - expectedRatio) < 0.001f, tag + "Offense damage scales linearly with missing health");

			// Cost: 6% of current health, shield untouched, never lethal.
			health.Regenerate(maximum);
			const float beforeCost = health.GetHealth();
			const float shieldBefore = f.ship->GetShieldComponent().GetShield();
			CastOffense(f);
			report.Check(std::fabs((beforeCost - health.GetHealth()) - beforeCost * 0.06f) < kTolerance, tag + "Offense use spends 6% of current health");
			report.Check(std::fabs(f.ship->GetShieldComponent().GetShield() - shieldBefore) < kTolerance, tag + "health cost bypasses the shield");
			health.ChangeHealth(-(health.GetHealth() - 0.5f));
			CastOffense(f);
			report.Check(std::fabs(health.GetHealth() - 0.5f) < kTolerance, tag + "health cost never kills");
		}

		// Heal window: an enemy hit by an Offense ability that dies within 4 s heals 1% of MaxHealth; a later death does not.
		{
			Fixture f{ report, application };
			f.Install(kBlood, 0, 1);
			auto& health = f.ship->GetHealthComponent();
			const float maximum = health.GetMaxHealth();
			const auto inWindow = Enemy(f);
			health.Regenerate(maximum);
			Hit(*inWindow, f.ship.get(), 100.f, Scaled(), {}, { GameplayTags::Ability::Offense }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
			// Healing never overheals, so the owner starts below MaxHealth; otherwise the 1% heal is clamped to nothing.
			health.ChangeHealth(-maximum * 0.5f);
			ApplyCombatDamage(*inWindow, 100000.f, nullptr, { DamageTypeSchema::Energy }, Scaled());
			const float beforeHeal = health.GetHealth();
			f.ship->GetModuleApplier().Tick(0.1f);
			report.Check(std::fabs((health.GetHealth() - beforeHeal) - maximum * 0.01f) < kTolerance, "Blood: an enemy killed by anyone within 4 s heals 1% MaxHealth");

			const auto lateKill = Enemy(f);
			Hit(*lateKill, f.ship.get(), 100.f, Scaled(), {}, { GameplayTags::Ability::Offense }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
			f.ship->GetModuleApplier().Tick(4.5f);
			ApplyCombatDamage(*lateKill, 100000.f, nullptr, { DamageTypeSchema::Energy }, Scaled());
			const float beforeLate = health.GetHealth();
			f.ship->GetModuleApplier().Tick(0.1f);
			report.Check(std::fabs(health.GetHealth() - beforeLate) < kTolerance, "Blood: a death after 4 s heals nothing");
		}

		// Debt Collection widens the window to 7 s.
		{
			Fixture f{ report, application };
			f.Install(kBlood, 1, 1, kDebtSide);
			auto& health = f.ship->GetHealthComponent();
			const float maximum = health.GetMaxHealth();
			const auto enemy = Enemy(f);
			health.Regenerate(maximum);
			Hit(*enemy, f.ship.get(), 100.f, Scaled(), {}, { GameplayTags::Ability::Offense }, DamageDeliveryType::Direct, { DamageTypeSchema::Energy });
			health.ChangeHealth(-maximum * 0.5f);
			f.ship->GetModuleApplier().Tick(5.f);
			ApplyCombatDamage(*enemy, 100000.f, nullptr, { DamageTypeSchema::Energy }, Scaled());
			const float beforeHeal = health.GetHealth();
			f.ship->GetModuleApplier().Tick(0.1f);
			report.Check(std::fabs((health.GetHealth() - beforeHeal) - maximum * 0.01f) < kTolerance, "Blood: Debt Collection keeps a 5 s kill inside the window and heals 1% MaxHealth");
		}

		// Clot Armor: Armor equal to 3 x the health spent, for 3 s, refreshed by the next grant.
		{
			Fixture f{ report, application };
			f.Install(kBlood, 1, 1, kClotSide);
			auto& health = f.ship->GetHealthComponent();
			health.Regenerate(health.GetMaxHealth());
			const float armorBefore = OwnerArmor(f);
			const float spent = health.GetHealth() * 0.06f;
			CastOffense(f);
			report.Check(std::fabs((OwnerArmor(f) - armorBefore) - spent * 3.f) < kTolerance, "Blood: Clot Armor grants 3 x the health spent");
			f.ship->GetModuleApplier().Tick(2.f);
			report.Check(std::fabs((OwnerArmor(f) - armorBefore) - spent * 3.f) < kTolerance, "Blood: Clot Armor still lasts at 2 s");
			f.ship->GetModuleApplier().Tick(1.5f);
			report.Check(std::fabs(OwnerArmor(f) - armorBefore) < kTolerance, "Blood: Clot Armor expires after 3 s");
		}
	}
}

int main(int argc, char** argv)
{
	// Setup below runs before any Application exists; applications adopt this binding.
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Epic Offense content failed to register" };
		const auto* prism = content::ModuleContentCatalog::FindById(kPrism);
		report.Check(prism && prism->rarity == ModuleRarity::Epic && prism->sizeVariants.size() == 2 && prism->sideFeatures.size() == 2, "Prism Core is an Epic two-size module with two private sides");
		const auto* blood = content::ModuleContentCatalog::FindById(kBlood);
		report.Check(blood && blood->rarity == ModuleRarity::Epic && blood->sizeVariants.size() == 2 && blood->sideFeatures.size() == 2, "Blood Contract is an Epic two-size module with two private sides");
		std::string title = "Epic Offense Modules E2E";
		Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		PrismChecks(report, application);
		StormChecks(report, application);
		AlternatingChecks(report, application);
		BloodChecks(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "EpicOffenseModulesE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
