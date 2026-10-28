#include "gameFramework/LightYearsAssets.h"
#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/AbilityLoader.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreProjectileActor.h"
#include "gameplay/ability/loadout/AbilityLoadoutManager.h"
#include "gameplay/ability/runtime/AbilityInvocationRuntime.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "level/GameLevel.h"
#include "presentation/hud/ability/AbilityBarPresenter.h"
#include "presentation/hud/ability/AbilityBarView.h"
#include "presentation/hud/evolveChoice/EvolveChoiceHUDController.h"
#include "presentation/hud/evolveChoice/EvolveChoiceView.h"
#include "widget/GameHUD.h"
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>

namespace
{
	using namespace vega;
	using Json = nlohmann::json;
	constexpr const char* RocketId = "Ability.Offense.Rocket.Basic";
	constexpr const char* ValueId = "Ability.Evolve.Rocket.Payload";
	constexpr const char* ScalingId = "Ability.Evolve.Rocket.Energy";
	constexpr const char* ShapeId = "Ability.Evolve.Rocket.Salvo";
	constexpr const char* SecondaryId = "Ability.Offense.Rocket.SecondaryTest";
	constexpr auto Q = sas::AbilitySlot::Ability1;
	constexpr auto E = sas::AbilitySlot::Ability2;

	struct Report
	{
		Json data{ { "test", "AbilityEvolveE2E" }, { "passed", false }, { "checks", Json::array() } };
		void Check(bool passed, const std::string& name)
		{
			data["checks"].push_back({ { "name", name }, { "passed", passed } });
			std::cout << (passed ? "PASS " : "FAIL ") << name << std::endl;
			if (!passed) throw std::runtime_error(name);
		}
		void Equal(float actual, float expected, const std::string& name) { Check(std::abs(actual - expected) < .002f, name + " (" + std::to_string(actual) + " / " + std::to_string(expected) + ")"); }
	};

	class EvolveLevel final : public GameLevel
	{
	public:
		explicit EvolveLevel(Application* app) : GameLevel{ app } {}
		using GameLevel::AddHUDController;
		using GameLevel::DispatchEvent;
	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	Json& Record(Json& root, const std::string& id = RocketId)
	{
		for (auto& record : root["abilities"]) if (record.at("id") == id) return record;
		throw std::runtime_error("Fixture record missing: " + id);
	}

	const Json& Record(const Json& root, const std::string& id = RocketId)
	{
		for (const auto& record : root.at("abilities")) if (record.at("id") == id) return record;
		throw std::runtime_error("Fixture record missing: " + id);
	}

	content::AbilityLoader::Result Load(const Json& root)
	{
		return content::AbilityLoader::LoadFromJson(root, AbilityData::GetBuiltinShippedAbilityDefinitions(), AbilityData::GetBuiltinAbilityActorDefinitions());
	}

	Json Fingerprint(const GameAbilityDefinition& definition)
	{
		Json result{ { "id", definition.abilityId }, { "slot", static_cast<int>(definition.slot) }, { "cooldown", definition.cooldown }, { "duration", definition.duration }, { "charges", definition.maxCharges }, { "behavior", ToString(definition.behaviorType) }, { "icon", definition.iconPath }, { "actors", Json::array() }, { "scaling", Json::array() } };
		for (const auto& action : definition.actions) if (auto* spawn = std::get_if<SpawnActorAction>(&action.action)) result["actors"].push_back(spawn->actorDefinitionId.ToString());
		for (const auto& rule : definition.scalingRules) result["scaling"].push_back({ std::string{ rule.targetAttributeId.GetName() }, std::string{ rule.sourceAttributeId.GetName() }, rule.coefficient });
		return result;
	}

	Json MakeFixture(Json root)
	{
		auto& rocket = Record(root);
		Json actor = rocket.at("actors").at(0);
		for (auto& attribute : actor["attributes"]) if (attribute.at("id") == "Common.Damage") attribute["baseValue"] = 100.f;
		const auto otherActors = Record(root, "Ability.Offense.OverdriveCore.Basic").at("actors");
		rocket["evolves"] = Json::array({
			{ { "id", ValueId }, { "name", "Heavy payload" }, { "description", "Raises projectile damage and changes cooldown." }, { "patch", { { "cooldown", 12.f }, { "actors", Json::array({ actor }) } } } },
			{ { "id", ScalingId }, { "name", "Energy converter" }, { "description", "Uses Energy Power instead of Attack Power and doubles level damage growth." }, { "patch", {
				{ "scalingRules", Json::array({ { { "targetAttributeId", "Common.Damage" }, { "sourceAttributeId", std::string{ OwnerAttributeIds::EnergyPower.GetName() } }, { "operation", "Add" }, { "coefficient", 2.f } } }) },
				{ "progression", { { "steps", { { "attributeModifiers", Json::array({ { { "attributeId", "Common.Damage" }, { "operation", "Add" }, { "magnitude", 10.f } } }) } } } } }
			} } },
			{ { "id", ShapeId }, { "name", "Alternate projectile" }, { "description", "Uses the registered Overdrive projectile with configured delivery." }, { "patch", { { "behaviorId", "Configured" }, { "actors", otherActors } } } }
		});
		return root;
	}

	void LoaderChecks(Report& report, const Json& shipped, const Json& fixture)
	{
		report.Check(Load(shipped).Succeeded(), "absent evolves preserves shipped loader");
		auto empty = shipped; Record(empty)["evolves"] = Json::array();
		report.Check(Load(empty).Succeeded(), "empty evolves accepted");
		const auto loaded = Load(fixture);
		report.Check(loaded.Succeeded(), "three evolves accepted: " + loaded.error);
		for (const auto& definition : loaded.definitions) if (definition.id == RocketId) report.Check(definition.definition.evolveContent->combinations.size() == 10, "all ten combinations stored inside one ability");
		report.Check(loaded.definitions.size() == shipped.at("abilities").size(), "combinations do not grant extra catalog abilities");
		List<const GameAbilityDefinition*> previouslyLoaded;
		for (const auto& definition : loaded.definitions) previouslyLoaded.push_back(&definition.definition);
		const auto noInheritedEvolves = content::AbilityLoader::LoadFromJson(shipped, previouslyLoaded, AbilityData::GetBuiltinAbilityActorDefinitions());
		report.Check(noInheritedEvolves.Succeeded() && std::all_of(noInheritedEvolves.definitions.begin(), noInheritedEvolves.definitions.end(), [&](const auto& definition) { return definition.definition.activeEvolveIds.empty() && (Record(shipped, definition.id).value("evolves", Json::array()).empty() == !definition.definition.evolveContent); }), "absent evolves never inherit previous runtime or fallback selections; authored evolves remain available");
		auto variant = shipped;
		variant["abilities"].push_back({ { "id", "Ability.Offense.Rocket.TestVariant" }, { "baseId", RocketId }, { "evolves", Record(fixture).at("evolves") } });
		const auto loadedVariant = Load(variant);
		report.Check(loadedVariant.Succeeded(), "baseId variant materializes private inherited actor combinations: " + loadedVariant.error);
		for (int count : { 1, 2, 4 })
		{
			auto invalid = fixture;
			auto& entries = Record(invalid)["evolves"];
			if (count == 4) entries.push_back(entries.front()); else entries.erase(entries.begin() + count, entries.end());
			report.Check(!Load(invalid).Succeeded(), "reject evolve count " + std::to_string(count));
		}
		for (const std::string id : { "bad", "Ability.Evolve.Rocket.", "Ability.Evolve.Rocket.Bad.Extra", "Ability.Evolve.Other.Wrong", ValueId })
		{
			auto invalid = fixture; Record(invalid)["evolves"][1]["id"] = id;
			report.Check(!Load(invalid).Succeeded(), "reject malformed, wrong-family or duplicate ID " + id);
		}
		for (const auto* field : { "id", "baseId", "slot", "abilityClass", "class", "category", "abilityTags", "evolves" })
		{
			auto invalid = fixture; Record(invalid)["evolves"][0]["patch"][field] = "forbidden";
			report.Check(!Load(invalid).Succeeded(), std::string{ "reject immutable patch " } + field);
		}
		for (const auto* field : { "name", "description" })
		{
			auto invalid = fixture; Record(invalid)["evolves"][0][field] = "";
			report.Check(!Load(invalid).Succeeded(), std::string{ "reject empty evolve " } + field);
		}
		for (const auto* selector : { "MissingBehavior", "Unknown" })
		{
			auto invalid = fixture; Record(invalid)["evolves"][2]["patch"]["behaviorId"] = selector;
			report.Check(!Load(invalid).Succeeded(), "reject unregistered behavior");
		}
		auto invalidPresentation = fixture;
		Record(invalidPresentation)["evolves"][2]["patch"]["actors"][0]["presentationProfileId"] = "Presentation.Ability.OverdriveCore.Projectile.Missing";
		report.Check(!Load(invalidPresentation).Succeeded(), "reject unregistered presentation");
		auto invalidActor = fixture;
		Record(invalidActor)["evolves"][2]["patch"]["actors"][0]["id"] = "Actor.Ability.OverdriveCore.Projectile.Unregistered";
		Record(invalidActor)["evolves"][2]["patch"]["actors"][0]["baseId"] = "Actor.Ability.OverdriveCore.Projectile.Basic";
		report.Check(!Load(invalidActor).Succeeded(), "reject actor ID without C++ registration even when baseId is registered");
		auto duplicateActor = fixture;
		auto& actors = Record(duplicateActor)["evolves"][2]["patch"]["actors"];
		actors.push_back(actors.front());
		report.Check(!Load(duplicateActor).Succeeded(), "reject duplicate actor IDs inside a combination");

		// Every single patch is valid, but removing a profile breaks the ordered pair.
		auto pairOnly = fixture;
		auto& base = Record(pairOnly);
		const std::string profileId = "AttributeProfile.Rocket.Projectile.Test";
		base["attributeProfiles"] = Json::array({ { { "id", profileId }, { "attributes", base["actors"][0]["attributes"] } } });
		auto inheritedActor = base["actors"][0];
		inheritedActor["attributes"] = Json::array(); inheritedActor["attributeProfileIds"] = Json::array({ profileId });
		base["evolves"][0]["patch"] = { { "actors", Json::array({ inheritedActor }) } };
		base["evolves"][1]["patch"] = { { "attributeProfiles", Json::array() } };
		const auto badPair = Load(pairOnly);
		report.Check(!badPair.Succeeded() && badPair.error.find(ValueId) != std::string::npos && badPair.error.find(ScalingId) != std::string::npos, "pair-only validation failure identifies ability and ordered evolve IDs");
		for (int keep : { 0, 1 })
		{
			auto isolated = pairOnly;
			for (int index = 0; index < 3; ++index) if (index != keep) Record(isolated)["evolves"][index]["patch"] = Json::object();
			report.Check(Load(isolated).Succeeded(), "each patch causing the pair-only failure is independently valid");
		}
	}

	template<class T> std::shared_ptr<T> Latest(EvolveLevel& level)
	{
		std::shared_ptr<T> result;
		for (const auto& weak : level.GetActorsByTypeIncludingPending<T>()) if (auto cast = weak.lock(); cast && !cast->GetIsPendingDestroy() && (!result || result->GetUniqueID() < cast->GetUniqueID())) result = cast;
		return result;
	}

	GameAbility& Grant(PlayerSpaceShip& ship, Report& report)
	{
		auto& asc = ship.GetAbilitySystemComponent();
		if (auto* current = asc.GetAbility(Q)) report.Check(asc.RemoveAbility(current->GetHandle()), "remove current slot ability");
		if (auto* current = asc.GetAbilityById(RocketId)) report.Check(asc.RemoveAbility(current->GetHandle()), "remove prior Rocket binding");
		report.Check(asc.GrantAbility(*content::AbilityContentCatalog::FindById(RocketId), Q).IsValid(), "grant evolved-capable ability through real ASC");
		return *asc.GetAbility(Q);
	}

	void Ready(GameAbility& ability)
	{
		if (ability.IsActive()) ability.Cancel(sas::AbilityEndReason::Completed);
		ability.Tick(100.f);
	}

	void RuntimeChecks(Report& report, const Json& fixture, const std::filesystem::path& fixturePath)
	{
		auto runtimeFixture = fixture;
		auto secondaryOptions = Record(fixture).at("evolves");
		for (auto& option : secondaryOptions) option["id"] = "Ability.Evolve.Rocket.Secondary" + option.at("id").get<std::string>().substr(std::string{ "Ability.Evolve.Rocket." }.size());
		runtimeFixture["abilities"].push_back({ { "id", SecondaryId }, { "baseId", RocketId }, { "evolves", secondaryOptions } });
		{ std::ofstream output{ fixturePath }; output << runtimeFixture.dump(2); }
		std::string reason;
		report.Check(content::AbilityContentCatalog::LoadFromFile(fixturePath, AbilityData::GetBuiltinShippedAbilityDefinitions(), AbilityData::GetBuiltinAbilityActorDefinitions(), &reason), "real catalog loads test-only content: " + reason);
		std::string title = "Ability evolve E2E";
		Application application{ { 1280u, 900u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		auto& manager = PlayerManager::GetPlayerManager(); manager.Reset();
		struct Reset { ~Reset() { PlayerManager::GetPlayerManager().Reset(); } } reset;
		Player& player = manager.CreateNewPlayer();
		auto level = std::make_shared<EvolveLevel>(&application);
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(ship != nullptr, "real player ship spawned");
		level->BeginPlayInternal(); level->TickInternal(0.f);
		auto& ability = Grant(*ship, report);
		auto& asc = ship->GetAbilitySystemComponent();
		asc.GetAttributes().SetBaseValue(OwnerAttributeIds::AttackPower, 20.f);
		asc.GetAttributes().SetBaseValue(OwnerAttributeIds::EnergyPower, 30.f);
		asc.GetAttributes().SetBaseValue(OwnerAttributeIds::AbilityHaste, 0.f);
		const auto unchangedSlot = sas::AbilitySlot::Ability3;
		report.Check(asc.GetAbility(unchangedSlot) != nullptr, "unrelated ability remains granted");
		const auto unchanged = Fingerprint(asc.GetAbility(unchangedSlot)->GetDefinition());
		AbilityBarPresenter bar;
		auto overlay = std::make_shared<EvolveChoiceHUDController>(weak_ptr<GameHUD>{});
		level->AddHUDController(overlay);
		const auto presenter = overlay->GetPresenter();
		auto press = [&](sf::Keyboard::Key key) { const bool handled = overlay->HandleEvent(sf::Event::KeyPressed{ key }); overlay->HandleEvent(sf::Event::KeyReleased{ key }); return handled; };
		bar.Tick(); presenter->Tick(0.f);
		AbilityBarView barView{ application.GetEngineContext(), bar.GetViewModel() };
		EvolveChoiceView evolveView{ application.GetEngineContext(), presenter->GetViewModel() };
		auto refreshViews = [&]()
		{
			barView.NativeTick(0.f); evolveView.NativeTick(0.f);
			barView.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 900.f } }, true);
			evolveView.ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 900.f } }, true);
		};
		refreshViews();
		report.Check(!evolveView.GetVisibility(), "view hidden before pending choice");
		report.Check(!ability.GetPendingEvolve().tierLevel && !bar.GetViewModel()->slots[0].evolvePending && !press(sf::Keyboard::Key::V) && !press(sf::Keyboard::Key::Num1), "no pending tier means no marker, overlay or captured input");
		report.Check(!player.TryChooseAbilityEvolve(Q, ValueId, &reason) && !reason.empty(), "closed tier rejects choose with reason");
		report.Check(asc.SetAbilityLevel(ability.GetHandle(), AbilityEvolveTierLevels[0] - 1), "level below first threshold");
		report.Check(!ability.GetPendingEvolve().tierLevel, "no pending below first tier");
		report.Check(asc.SetAbilityLevel(ability.GetHandle(), AbilityEvolveTierLevels[0]), "reach first evolve threshold");
		bar.Tick(); presenter->Tick(0.f);
		report.Check(ability.GetPendingEvolve().options.size() == 3 && bar.GetViewModel()->slots[0].evolvePending && !presenter->GetViewModel()->open, "tier earned without forcing overlay");
		report.Check(press(sf::Keyboard::Key::V) && presenter->GetViewModel()->open && presenter->GetViewModel()->tierLevel == AbilityEvolveTierLevels[0], "V opens pending ability at first tier");
		refreshViews();
		const auto& children = barView.GetColumn(0)->GetChildren();
		const auto badge = std::dynamic_pointer_cast<TextWidget>(children.back());
		const auto state = std::dynamic_pointer_cast<TextWidget>(children[children.size() - 2]);
		report.Check(evolveView.GetVisibility() && badge && badge->GetVisibility() && state && badge->GetResolvedRect().position.y >= state->GetResolvedRect().position.y + state->GetResolvedRect().size.y, "real view displays separate pending badge below state text");
		sf::RenderTexture image{ { 1280u, 900u } };
		image.clear({ 15, 20, 30 }); barView.NativeDraw(image); image.display();
		report.Check(image.getTexture().copyToImage().saveToFile(fixturePath.parent_path() / "ability-evolve-hud.png"), "save actual ability bar render");
		image.clear({ 15, 20, 30 }); barView.NativeDraw(image); evolveView.NativeDraw(image); image.display();
		report.Check(image.getTexture().copyToImage().saveToFile(fixturePath.parent_path() / "ability-evolve-overlay.png"), "save actual choice overlay render");
		report.Check(level->DispatchEvent(sf::Event::KeyPressed{ sf::Keyboard::Key::Escape }) && !presenter->GetViewModel()->open && !level->IsPaused(), "Escape through GameLevel defers choice without pausing");
		report.Check(!player.TryChooseAbilityEvolve(Q, "Ability.Evolve.Rocket.Unknown", &reason), "unknown evolve rejected");
		Ready(ability); report.Check(ability.TryActivate(), "base Rocket activates before pick");
		level->TickInternal(0.f);
		const auto inFlight = Latest<RocketProjectileActor>(*level);
		report.Check(inFlight != nullptr, "base activation spawns Rocket actor");
		report.Equal(inFlight->GetDamage(), 115.f, "base level-eight damage uses AP");
		const auto historyBeforePick = asc.GetAbilityUseHistory().GetRecords().front();
		press(sf::Keyboard::Key::V);
		report.Check(press(sf::Keyboard::Key::Num1) && ability.HasEvolve(ValueId), "number key chooses through controller and presenter");
		bar.Tick();
		report.Check(!bar.GetViewModel()->slots[0].evolvePending && !presenter->GetViewModel()->open && !ability.GetPendingEvolve().tierLevel, "first pick clears marker until second tier");
		refreshViews(); report.Check(!badge->GetVisibility() && !evolveView.GetVisibility(), "actual views hide badge and overlay after first pick");
		report.Equal(inFlight->GetDamage(), 115.f, "in-flight projectile retains captured damage after pick");
		report.Equal(ability.GetCooldownDuration(), 8.975f, "value evolve cooldown includes current-level global progression");
		Ready(ability); report.Check(ability.TryActivate(), "value evolved Rocket activates"); level->TickInternal(0.f);
		report.Equal(Latest<RocketProjectileActor>(*level)->GetDamage(), 160.f, "value patch changes activation damage");
		report.Check(!player.TryChooseAbilityEvolve(Q, ValueId, &reason), "already chosen evolve cannot fill closed tier");
		report.Check(asc.SetAbilityLevel(ability.GetHandle(), AbilityEvolveTierLevels[1]), "leveling continues after first pick");
		report.Check(ability.GetPendingEvolve().options.size() == 2 && ability.GetPendingEvolve().tierLevel == AbilityEvolveTierLevels[1], "second tier opens exactly two remaining options");
		report.Check(!player.TryChooseAbilityEvolve(Q, ValueId, &reason), "already picked option rejected while tier open");
		report.Check(player.TryChooseAbilityEvolve(RocketId, ScalingId, &reason), "choose scaling evolve by same ability ID");
		report.Check(ability.GetEvolveSelection() == List<std::string>{ ValueId, ScalingId } && !ability.GetPendingEvolve().tierLevel, "both ordered picks active and third closed");
		report.Check(!player.TryChooseAbilityEvolve(Q, ShapeId, &reason), "third evolve permanently closed for run");
		Ready(ability); report.Check(ability.TryActivate(), "scaling evolved activation"); level->TickInternal(0.f);
		report.Equal(Latest<RocketProjectileActor>(*level)->GetDamage(), 300.f, "patched progression and EP scaling change damage at current level");
		player.AwardScrap(100000);
		report.Check(player.TryPurchaseAbilityLevel(Q, &reason) && ability.GetLevel() == AbilityEvolveTierLevels[1] + 1 && ability.HasEvolve(ScalingId), "Player level purchase keeps selected combination");
		Ready(ability); report.Check(ability.TryActivate(), "post-pick level-up activation"); level->TickInternal(0.f);
		report.Equal(Latest<RocketProjectileActor>(*level)->GetDamage(), 310.f, "patched ladder continues on subsequent level");
		report.Check(Fingerprint(asc.GetAbility(unchangedSlot)->GetDefinition()) == unchanged && !asc.GetAbility(unchangedSlot)->GetDefinition().evolveContent, "unrelated ability definition unchanged");

		// History recorded before the evolve still replays the old actor values and AP channel.
		AbilityInvocationRuntime replay{ asc };
		List<sas::AttributeScalingRule> originalScaling;
		for (const auto& channel : historyBeforePick.scalingChannels) originalScaling.push_back({ channel.targetAttributeId, channel.sourceAttributeId, channel.operation, channel.sourceCoefficient });
		report.Check(replay.Invoke(historyBeforePick, originalScaling, 1.f, E, false, &reason), "recorded invocation survives subsequent evolves"); level->TickInternal(0.f);
		report.Equal(Latest<RocketProjectileActor>(*level)->GetDamage(), 115.f, "recorded invocation retains pre-evolve combination");

		report.Check(ship->GetAbilityLoadout().UnequipAbility(Q, &reason), "slot removal through loadout path");
		report.Check(ship->GetAbilityLoadout().EquipAbility(RocketId, Q, &reason), "re-grant through loadout path");
		report.Check(asc.GetAbility(Q)->GetEvolveSelection() == List<std::string>{ ValueId, ScalingId } && asc.GetAbility(Q)->GetLevel() == AbilityEvolveTierLevels[1] + 1, "slot removal and re-grant preserve Player run progression");
		ship->Destroy(); level->TickInternal(0.f); bar.Tick(); presenter->Tick(0.f);
		report.Check(!presenter->GetViewModel()->open, "destroyed ship closes overlay");
		ship = player.SpawnSpaceShip(level.get()).lock(); report.Check(ship != nullptr, "respawn real player ship"); level->TickInternal(0.f);
		auto& restored = Grant(*ship, report);
		report.Check(restored.GetEvolveSelection() == List<std::string>{ ValueId, ScalingId } && restored.GetLevel() == AbilityEvolveTierLevels[1] + 1, "respawn reapplies evolves with purchased levels");

		// Fresh Player: pass both tiers without choosing, then fill them in order.
		manager.Reset(); Player& latePlayer = manager.CreateNewPlayer();
		auto lateShip = latePlayer.SpawnSpaceShip(level.get()).lock(); report.Check(lateShip != nullptr, "fresh run for delayed picks"); level->TickInternal(0.f);
		auto& late = Grant(*lateShip, report); auto& lateASC = lateShip->GetAbilitySystemComponent();
		report.Check(lateASC.SetAbilityLevel(late.GetHandle(), AbilityEvolveTierLevels[1]), "reach second threshold without choosing first");
		report.Check(lateASC.AssignSecondAbility(Q, *content::AbilityContentCatalog::FindById(SecondaryId), &reason), "grant another evolving ability in retained secondary bank");
		report.Check(lateASC.SetAbilityLevel(lateASC.GetSecondAbility(Q)->GetHandle(), AbilityEvolveTierLevels[0]) && !lateASC.IsTwinRackEnabled(), "secondary pending choice exists while its bank is inactive");
		presenter->Tick(0.f); press(sf::Keyboard::Key::V);
		report.Check(presenter->GetViewModel()->tierLevel == AbilityEvolveTierLevels[0] && presenter->GetViewModel()->options.size() == 3, "late first pick still fills first tier");
		report.Check(press(sf::Keyboard::Key::Num3) && late.HasEvolve(ShapeId), "choose registered structural evolve");
		bar.Tick();
		report.Check(presenter->GetViewModel()->open && presenter->GetViewModel()->tierLevel == AbilityEvolveTierLevels[1] && presenter->GetViewModel()->options.size() == 2 && bar.GetViewModel()->slots[0].evolvePending, "after delayed first pick overlay and badge advance to second tier");
		Ready(late); report.Check(late.TryActivate(), "structural evolved ability activates"); level->TickInternal(0.f);
		report.Check(late.GetDefinition().behaviorType == AbilityBehaviorType::Configured && Latest<OverdriveCoreProjectileActor>(*level) != nullptr, "replacement behavior and actor really spawn through registry");
		report.Check(press(sf::Keyboard::Key::Num1) && late.GetEvolveSelection() == List<std::string>{ ShapeId, ValueId }, "second delayed pick preserves ordered combination");
		bar.Tick(); report.Check(!bar.GetViewModel()->slots[0].evolvePending && presenter->GetViewModel()->open && presenter->GetViewModel()->abilityId == SecondaryId, "overlay advances from first bank to next pending ability without reopening");
		report.Check(late.GetDefinition().configurationContent->actors.front().actorType == AbilityActorType::RocketProjectile, "later actor array replaces earlier structural patch");
		report.Check(press(sf::Keyboard::Key::Num1) && !lateASC.GetSecondAbility(Q)->GetEvolveSelection().empty(), "presenter chooses pending secondary ability by ID even while bank inactive");
		report.Check(!presenter->GetViewModel()->open && !press(sf::Keyboard::Key::V), "all granted choices filled closes overlay and releases its key");
	}
}

int main(int argc, char** argv)
{
	vega::EngineContext context; vega::EngineContext::Scope scope{ context }; vega::InstallVegaEngineAssets();
	const auto artifact = std::filesystem::absolute(argc > 1 ? argv[1] : "outputs/ability-evolve-e2e.json");
	std::filesystem::create_directories(artifact.parent_path());
	Report report;
	try
	{
		const auto assets = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
		context.Assets().SetAssetRootDirectory(assets.generic_string() + "/");
		report.Check(vega::GameContentBootstrap::Register(), "shipped bootstrap");
		std::ifstream input{ assets / "content/data/abilities.json" }; Json shipped; input >> shipped;
		const auto fixture = MakeFixture(shipped);
		LoaderChecks(report, shipped, fixture);
		RuntimeChecks(report, fixture, artifact.parent_path() / "ability-evolve-fixture.json");
		report.data["passed"] = true;
	}
	catch (const std::exception& error) { report.data["error"] = error.what(); std::cerr << error.what() << '\n'; }
	std::ofstream output{ artifact }; output << report.data.dump(2) << '\n';
	return report.data.value("passed", false) ? 0 : 1;
}
