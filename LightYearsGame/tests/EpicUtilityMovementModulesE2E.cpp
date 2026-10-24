#include "framework/EngineContext.h"

#include <algorithm>
#include "gameConfigs/CollisionLayers.h"
#include "gameFramework/LightYearsAssets.h"
#include "UncommonModuleP6E2E.h"
#include "gameplay/module/rules/EpicUtilityMovementRuntime.h"

namespace
{
	using namespace uncommon_p6_e2e;
	const char* const kCryoBola = "Ability.Utility.CryoBola.Basic";

	ModuleEpicUtilityMovementRuntime& Epic(Fixture& f) { return f.ship->GetModuleApplier().GetEpicUtilityMovementRuntime(); }

	void CascadeChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto id = f.Install("Module.Utility.CascadeReactor.Epic", 0, 1);
		CastOffense(f);
		f.Equal("the first use starts the chain without a bonus", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastMovement(f);
		f.Equal("a different family takes step one", 1.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastOffense(f);
		f.Equal("a different family again takes step two", 2.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastOffense(f);
		f.Equal("repeating the same family resets the chain", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		CastMovement(f);
		f.Equal("the chain restarts from the reset family", 1.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		f.ship->GetModuleApplier().Tick(5.1f);
		f.Equal("a lapsed five second window resets the chain", 0.f, static_cast<float>(Epic(f).GetCascadeStep(id)));
		f.Remove(id);
		report.Check(Epic(f).GetCascadeStep(id) == 0, "uninstall removes the chain state");
	}

	void FieldMasteryChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto id = f.Install("Module.Utility.FieldMastery.Epic", 0, 1);
		const List<GameplayTag> utility{ GameplayTags::Ability::Utility };
		f.Equal("Utility abilities cannot be upgraded with Scrap while installed", 1.f, Epic(f).IsScrapUpgradeLocked(utility) ? 1.f : 0.f);
		// Cryo Bola is a shipped Utility ability (default loadout Ability1) and the real source of Utility uses here.
		auto& abilities = f.ship->GetAbilitySystemComponent();
		if (!abilities.GetAbilityById(kCryoBola))
		{
			if (auto* current = abilities.GetAbility(sas::AbilitySlot::Ability1)) report.Check(abilities.RemoveAbility(current->GetHandle()), "Field Mastery test replaces only the fixture's Q ability");
			const auto* authored = content::AbilityContentCatalog::FindById(kCryoBola);
			report.Check(authored != nullptr, "Field Mastery test uses shipped Cryo Bola content");
			auto definition = *authored; definition.slot = sas::AbilitySlot::Ability1;
			report.Check(abilities.GrantAbility(definition).IsValid(), "Field Mastery test grants the shipped Cryo Bola Utility ability");
		}
		const auto* ability = abilities.GetAbilityById(kCryoBola);
		report.Check(ability != nullptr, "Field Mastery test resolves the Cryo Bola Utility ability on the ship");
		report.Check(std::any_of(ability->GetDefinition().abilityTags.begin(), ability->GetDefinition().abilityTags.end(), [](const GameplayTag& tag) { return tag.MatchesTag(GameplayTags::Ability::Utility); }), "Field Mastery test ability carries the Utility tag");
		f.Equal("the granted Utility ability starts at run level one", 1.f, static_cast<float>(ability->GetLevel()));
		for (int use = 0; use < 14; ++use) Epic(f).OnAbilityUse(kCryoBola, utility);
		f.Equal("uses accumulate per ability", 14.f, Epic(f).GetUseCount(kCryoBola));
		f.ship->GetModuleApplier().Tick(0.01f);
		f.Equal("fourteen uses do not grant a run level yet", 1.f, static_cast<float>(ability->GetLevel()));
		Epic(f).OnAbilityUse(kCryoBola, utility);
		f.ship->GetModuleApplier().Tick(0.01f);
		f.Equal("the fifteenth use grants one run level", 2.f, static_cast<float>(ability->GetLevel()));
		f.Equal("the threshold is spent", 0.f, Epic(f).GetUseCount(kCryoBola));
		f.Remove(id);
		report.Check(!Epic(f).IsScrapUpgradeLocked(utility), "uninstall lifts the Scrap lock");
	}

	void AssaultChecks(Report& report, Application& application)
	{
		Fixture f{ report, application };
		const auto id = f.Install("Module.Movement.AssaultVector.Epic", 0, 1);
		const auto enemy = Enemy(f, 500.f);
		enemy->SetActorLocation(f.ship->GetActorLocation() + sf::Vector2f{ 500.f, 0.f });
		f.ship->GetModuleApplier().Tick(0.01f);
		Move(f, 100.f, .25f);
		f.Equal("moving toward the nearest enemy adds the Level 1 damage bonus", 0.15f, Epic(f).GetAssaultDamageBonus());
		f.Equal("moving toward the enemy adds no Armor", 0.f, Epic(f).GetAssaultArmorBonus());
		Move(f, -100.f, .25f);
		f.Equal("moving away from the enemy clears the damage bonus", 0.f, Epic(f).GetAssaultDamageBonus());
		f.Equal("moving away from the enemy adds the Level 1 Armor bonus", 20.f, Epic(f).GetAssaultArmorBonus());
		for (float x : { 10.f, 0.f, -10.f })
		{
			const sf::Vector2f velocity{ x, 150.f };
			enemy->SetActorLocation(f.ship->GetActorLocation() + velocity * .1f + sf::Vector2f{ 500.f, 0.f });
			const auto source = f.ship->GetUniqueID();
			report.Check(movement::MovementInfluenceService::SetForcedMovement(*f.ship, { source, velocity, 1 }), "Assault direction test moves through the real component");
			f.ship->GetMovementComponent().Tick(.1f); f.ship->GetModuleApplier().Tick(.1f);
			movement::MovementInfluenceService::RemoveSource(*f.ship, source);
			f.Equal("the entire forward half-plane including perpendicular grants damage", x >= 0.f ? .15f : 0.f, Epic(f).GetAssaultDamageBonus());
			f.Equal("only the reverse half-plane grants Armor", x < 0.f ? 20.f : 0.f, Epic(f).GetAssaultArmorBonus());
		}
		Move(f, 0.f, .25f);
		f.Equal("standing still gives neither bonus (damage)", 0.f, Epic(f).GetAssaultDamageBonus());
		f.Equal("standing still gives neither bonus (Armor)", 0.f, Epic(f).GetAssaultArmorBonus());
		f.Remove(id);
		f.Equal("uninstall clears the Armor bonus", 0.f, Epic(f).GetAssaultArmorBonus());
	}

	void SideChecks(Report& report, Application& application)
	{
		Fixture side{ report, application };
		const auto id = side.Install("Module.Utility.FieldMastery.Epic", 1, 1, "Module.SideFeature.FieldMastery.KnowledgeTransfer");
		const auto& fields = side.ship->GetModuleApplier().GetResolvedEffects().epicRules.fieldMasteries;
		const bool transfer = std::any_of(fields.begin(), fields.end(), [](const ResolvedModuleFieldMasteryRule& rule) { return rule.transferUses == 2.f; });
		report.Check(transfer, "Knowledge Transfer adds two transfer uses at the three-cell size");
		side.Remove(id);

		Fixture small{ report, application };
		small.Install("Module.Utility.FieldMastery.Epic", 0, 1, "Module.SideFeature.FieldMastery.KnowledgeTransfer");
		const auto& smallFields = small.ship->GetModuleApplier().GetResolvedEffects().epicRules.fieldMasteries;
		report.Check(std::none_of(smallFields.begin(), smallFields.end(), [](const ResolvedModuleFieldMasteryRule& rule) { return rule.transferUses > 0.f; }), "two-cell Field Mastery keeps its side locked");
	}

	void ContentChecks(Report& report)
	{
		const auto* cascade = content::ModuleContentCatalog::FindById("Module.Utility.CascadeReactor.Epic");
		const auto* field = content::ModuleContentCatalog::FindById("Module.Utility.FieldMastery.Epic");
		const auto* assault = content::ModuleContentCatalog::FindById("Module.Movement.AssaultVector.Epic");
		const auto* phase = content::ModuleContentCatalog::FindById("Module.Movement.PhaseSlip.Epic");
		report.Check(cascade && cascade->rarity == vega::ModuleRarity::Epic && cascade->sideFeatures.empty(), "Cascade Reactor is a sideless Epic Utility");
		report.Check(field && field->rarity == vega::ModuleRarity::Epic && field->sizeLadderCellCounts == List<int>{ 2, 3 } && field->sideFeatures.size() == 1, "Field Mastery is a 2/3 Epic ladder with one side");
		report.Check(assault && assault->rarity == vega::ModuleRarity::Epic && assault->sideFeatures.empty(), "Assault Vector is a sideless Epic Movement");
		report.Check(phase && phase->rarity == vega::ModuleRarity::Epic && phase->sideFeatures.empty(), "Phase Slip is a sideless Epic Movement");
		report.Check(content::ModuleSideFeatureCatalog::FindById("Module.SideFeature.FieldMastery.KnowledgeTransfer") != nullptr, "Knowledge Transfer side is registered");
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
		if (!GameContentBootstrap::Register()) throw std::runtime_error{ "Epic Utility/Movement content registration failed" };
		std::string title = "Epic Utility Movement E2E"; Application application{ sf::Vector2u{ 64u, 64u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		ContentChecks(report); CascadeChecks(report, application); FieldMasteryChecks(report, application); AssaultChecks(report, application); SideChecks(report, application); report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "EpicUtilityMovementModulesE2E"; WriteArtifact(argv[1], report.artifact); return report.artifact.value("passed", false) ? 0 : 1;
}
