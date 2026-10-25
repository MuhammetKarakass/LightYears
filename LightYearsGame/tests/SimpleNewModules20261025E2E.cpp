#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP4E2E.h"
#include "gameplay/content/ModuleLoader.h"

namespace
{
	using namespace vega;
	using namespace common_module_e2e;
	using uncommon_p4_e2e::Enemy;
	using uncommon_p4_e2e::Fixture;
	using uncommon_p4_e2e::Hit;
	using uncommon_p4_e2e::Plain;

	struct StartedCooldown
	{
		GameAbility* ability = nullptr;
		float remaining = 0.f;
	};

	List<StartedCooldown> StartAbilityCooldowns(Fixture& f)
	{
		List<StartedCooldown> started;
		for (const auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
		{
			GameAbility* ability = f.ship->GetAbilitySystemComponent().GetAbility(slot);
			if (!ability) continue;
			if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
			ability->Tick(1000.f);
			if (!ability->TryActivate()) continue;
			if (ability->IsActive()) ability->Cancel(sas::AbilityEndReason::Completed);
			if (ability->GetCooldownRemaining() > 0.f) started.push_back({ ability, ability->GetCooldownRemaining() });
		}
		return started;
	}

	DamagePayload PrimaryCritical(const std::shared_ptr<DamageHitGroup>& group, bool critical = true, DamageHitGroup::Kind kind = DamageHitGroup::Kind::PrimaryShot)
	{
		DamagePayload payload = Plain();
		payload.criticalPolicy = critical ? DamageCriticalPolicy::Guaranteed : DamageCriticalPolicy::Disabled;
		payload.hitGroup = group ? group : std::make_shared<DamageHitGroup>(kind);
		if (payload.hitGroup->kind != kind) payload.hitGroup = std::make_shared<DamageHitGroup>(kind);
		return payload;
	}

	float RuleAmountAt(const ModuleDefinition& module, int effectiveLevel)
	{
		const ModuleEventAction& rule = module.sizeVariants.front().eventActions.front();
		return rule.amountByLevel.ValueAtLevel(rule.amount, effectiveLevel);
	}

	void ContentChecks(Report& report)
	{
		const auto rejects = [&report](const char* label, const Json& action)
		{
			bool rejected = false;
			try { content::ModuleLoader::ParseRulePackage(Json{ { "eventActions", Json::array({ action }) } }, "Simple new module invalid fixture", 3, false); }
			catch (const std::exception&) { rejected = true; }
			report.Check(rejected, label, action);
		};
		const Json critical = { { "trigger", "CriticalHit" }, { "kind", "ReduceLongestCooldownSeconds" }, { "amount", { 1.f, 1.2f, 1.4f } }, { "everyCount", 3 } };
		const Json reflex = { { "trigger", "DamageTaken" }, { "kind", "RefundAllCooldownsFraction" }, { "amount", { .15f, .18f, .21f } }, { "cooldownSeconds", 3.f } };
		Json entry = critical;
		entry["trigger"] = "DamageTaken";
		rejects("Critical Sync rejects a non-critical trigger", entry);
		entry = critical; entry["amount"] = 0;
		rejects("Critical Sync rejects a non-positive reduction", entry);
		entry = reflex; entry["trigger"] = "CriticalHit";
		rejects("Reflex Circuit rejects a non-damage trigger", entry);
		entry = reflex; entry.erase("cooldownSeconds");
		rejects("Reflex Circuit requires an internal cooldown", entry);
		entry = reflex; entry["cooldownSeconds"] = 0;
		rejects("Reflex Circuit rejects a zero internal cooldown", entry);
		entry = reflex; entry["amount"] = { 1.01f, 1.2f, 1.4f };
		rejects("Reflex Circuit rejects a refund above one", entry);
		entry = reflex; entry["abilityFamily"] = "Ability.Defense";
		rejects("Reflex Circuit rejects a family-scoped refund", entry);
		entry = critical; entry["abilityFamily"] = "Ability.Primary";
		rejects("Critical Sync rejects a family-scoped reduction", entry);
		const auto criticalPackage = content::ModuleLoader::ParseRulePackage(Json{ { "eventActions", Json::array({ critical }) } }, "Critical Sync valid fixture", 3, false);
		const auto reflexPackage = content::ModuleLoader::ParseRulePackage(Json{ { "eventActions", Json::array({ reflex }) } }, "Reflex Circuit valid fixture", 3, false);
		report.Check(criticalPackage.eventActions.size() == 1 && criticalPackage.eventActions.front().trigger == ModuleTriggerKind::CriticalHit &&
			criticalPackage.eventActions.front().kind == ModuleEventActionKind::ReduceLongestCooldownSeconds && criticalPackage.eventActions.front().everyCount == 3,
			"loader accepts Critical Sync with its critical trigger and shot cadence");
		report.Check(reflexPackage.eventActions.size() == 1 && reflexPackage.eventActions.front().trigger == ModuleTriggerKind::DamageTaken &&
			reflexPackage.eventActions.front().kind == ModuleEventActionKind::RefundAllCooldownsFraction && NearlyEqual(reflexPackage.eventActions.front().cooldownSeconds, 3.f),
			"loader accepts Reflex Circuit with its damage trigger and internal cooldown");
	}

	void CriticalSyncChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto id = f.Install("Module.Primary.CriticalSync.Uncommon", 0, level);
			const auto started = StartAbilityCooldowns(f);
			report.Check(started.size() >= 2, "Critical Sync fixture starts at least two real ability cooldowns");
			if (started.size() < 2) continue;
			const int zoneBonus = ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			const float reduction = RuleAmountAt(*f.rules.lookup("Module.Primary.CriticalSync.Uncommon"), level + zoneBonus);
			auto longest = std::max_element(started.begin(), started.end(), [](const StartedCooldown& a, const StartedCooldown& b) { return a.remaining < b.remaining; });
			const float before = longest->remaining;
			const auto enemy = Enemy(f, 100.f);

			const auto first = std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot);
			Hit(*enemy, f.ship.get(), 1.f, PrimaryCritical(first));
			Hit(*enemy, f.ship.get(), 1.f, PrimaryCritical(first));
			report.CheckValue("Critical Sync counts one Primary shot once across repeated hit groups", before, longest->ability->GetCooldownRemaining(), "two hits sharing one PrimaryShot DamageHitGroup");
			Hit(*enemy, f.ship.get(), 1.f, PrimaryCritical(std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot), false));
			report.CheckValue("Critical Sync ignores non-critical Primary hits", before, longest->ability->GetCooldownRemaining(), "PrimaryShot with criticalPolicy Disabled");
			Hit(*enemy, f.ship.get(), 1.f, PrimaryCritical(std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::AbilityUse), true, DamageHitGroup::Kind::AbilityUse));
			report.CheckValue("Critical Sync ignores ability criticals", before, longest->ability->GetCooldownRemaining(), "AbilityUse DamageHitGroup critical hit");
			Hit(*enemy, f.ship.get(), 1.f, PrimaryCritical(std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot)));
			Hit(*enemy, f.ship.get(), 1.f, PrimaryCritical(std::make_shared<DamageHitGroup>(DamageHitGroup::Kind::PrimaryShot)));

			int changed = 0;
			for (const StartedCooldown& cooldown : started)
				if (std::abs(cooldown.ability->GetCooldownRemaining() - cooldown.remaining) > 0.001f) ++changed;
			report.Check(changed == 1, "Critical Sync reduces only the longest remaining cooldown", Json{ { "changed", changed } });
			report.CheckValue("Critical Sync reduces the longest cooldown by its level-table seconds", std::max(0.f, before - reduction),
				longest->ability->GetCooldownRemaining(), "three distinct critical PrimaryShot DamageHitGroups");
		}
	}

	void ReflexCircuitChecks(Report& report, Application& application)
	{
		for (int level = 1; level <= 3; ++level)
		{
			Fixture f{ report, application };
			const auto id = f.Install("Module.Defense.ReflexCircuit.Rare", 0, level);
			const auto started = StartAbilityCooldowns(f);
			report.Check(started.size() >= 2, "Reflex Circuit fixture starts at least two real ability cooldowns");
			if (started.size() < 2) continue;
			const int zoneBonus = ModuleResolver::FindZoneLevelBonus(f.ship->GetModuleApplier().GetResolvedEffects(), id);
			const float fraction = RuleAmountAt(*f.rules.lookup("Module.Defense.ReflexCircuit.Rare"), level + zoneBonus);
			const auto enemy = Enemy(f, 100.f);
			f.ship->GetShieldComponent().ChangeShield(-10000.f);

			Hit(*f.ship, enemy.get(), 1.f, Plain());
			for (const StartedCooldown& cooldown : started)
				report.CheckValue("Reflex Circuit refunds every running cooldown", cooldown.remaining * (1.f - fraction),
					cooldown.ability->GetCooldownRemaining(), "damage taken with all four loadout slots cooling down");
			for (const StartedCooldown& cooldown : started)
				report.Check(!cooldown.ability->IsActive(), "Reflex Circuit does not activate abilities", Json{ { "ability", cooldown.ability->GetDefinition().abilityId } });

			List<float> afterFirst;
			for (const StartedCooldown& cooldown : started) afterFirst.push_back(cooldown.ability->GetCooldownRemaining());
			Hit(*f.ship, enemy.get(), 1.f, Plain());
			for (std::size_t index = 0; index < started.size(); ++index)
				report.CheckValue("Reflex Circuit blocks a second trigger inside three seconds", afterFirst[index],
					started[index].ability->GetCooldownRemaining(), "immediate second damage event");
			f.ship->GetModuleApplier().Tick(2.99f);
			Hit(*f.ship, enemy.get(), 1.f, Plain());
			for (std::size_t index = 0; index < started.size(); ++index)
				report.CheckValue("Reflex Circuit remains blocked just before three seconds", afterFirst[index],
					started[index].ability->GetCooldownRemaining(), "damage at 2.99 seconds");
			f.ship->GetModuleApplier().Tick(.02f);
			Hit(*f.ship, enemy.get(), 1.f, Plain());
			for (std::size_t index = 0; index < started.size(); ++index)
				report.CheckValue("Reflex Circuit triggers again after its internal cooldown", afterFirst[index] * (1.f - fraction),
					started[index].ability->GetCooldownRemaining(), "damage at 3.01 seconds");
		}
	}

	void CatalogChecks(Report& report)
	{
		const auto* critical = content::ModuleContentCatalog::FindById("Module.Primary.CriticalSync.Uncommon");
		const auto* reflex = content::ModuleContentCatalog::FindById("Module.Defense.ReflexCircuit.Rare");
		report.Check(critical && critical->rarity == ModuleRarity::Uncommon && critical->family == GameplayTag{ "Ability.Primary" } &&
			critical->sizeLadderCellCounts == List<int>{ 1 } && critical->sizeVariants.size() == 1 &&
			critical->sizeVariants.front().shape.cells == List<sf::Vector2i>{ { 0, 0 } } && critical->sideFeatures.empty(),
			"Critical Sync ships as a one-cell Uncommon Primary module without a side feature");
		report.Check(reflex && reflex->rarity == ModuleRarity::Rare && reflex->family == GameplayTag{ "Ability.Defense" } &&
			reflex->sizeLadderCellCounts == List<int>{ 2 } && reflex->sizeVariants.size() == 1 &&
			reflex->sizeVariants.front().shape.cells == List<sf::Vector2i>{ { 0, 0 }, { 1, 0 } } && reflex->sideFeatures.empty(),
			"Reflex Circuit ships as a two-cell Rare Defense module without a side feature");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext testContext;
	vega::EngineContext::Scope testScope{ testContext };
	vega::InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		vega::EngineContext::Current().Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Simple new modules content registration failed" };
		std::string title = "Simple New Modules E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		CatalogChecks(report); ContentChecks(report); CriticalSyncChecks(report, application); ReflexCircuitChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "SimpleNewModules20261025E2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
