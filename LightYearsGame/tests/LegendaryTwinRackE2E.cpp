#include "framework/EngineContext.h"
#include "gameFramework/LightYearsAssets.h"
#include "CommonModuleE2ETestSupport.h"
#include "gameplay/ability/loadout/DefaultAbilityLoadout.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "presentation/hud/ability/AbilityBarPresenter.h"
#include "presentation/hud/ability/AbilityBarView.h"

namespace
{
	using namespace vega;
	using namespace common_module_e2e;
	constexpr sas::AbilitySlot Slots[] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };

	GameAbilityDefinition Probe(int index, bool second, int charges = 1)
	{
		GameAbilityDefinition definition;
		definition.abilityId = std::string{ second ? "Ability.Defense.TwinRackSecond" : "Ability.Offense.TwinRackFirst" } + std::to_string(index) + ".Basic";
		definition.slot = Slots[index];
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldownStartPolicy = sas::AbilityCooldownStartPolicy::OnActivation;
		definition.cooldown = second ? 20.f + index : 10.f + index;
		definition.maxCharges = charges;
		definition.scrapCost = { 10, 3 };
		definition.abilityTags = { second ? GameplayTags::Ability::Defense : GameplayTags::Ability::Offense };
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/shield_gold.png";
		return definition;
	}

	struct Fixture
	{
		Report& report;
		shared_ptr<HeadlessModuleLevel> level;
		Player& player;
		shared_ptr<PlayerSpaceShip> ship;
		ShipSchematic schematic;
		ModuleRulesContext rules;
		ModuleLoadout loadout;
		std::uint32_t installed = 0;

		Fixture(Report& result, Application& application) : report{ result }, level{ std::make_shared<HeadlessModuleLevel>(&application) }, player{ PlayerManager::GetPlayerManager().CreateNewPlayer() }, schematic{ *content::ShipContentCatalog::GetPlayerFighterDefinition().schematic }, rules{ schematic, [](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); }, content::ModuleContentCatalog::GetZoneTable(), [](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); } }
		{
			ship = player.SpawnSpaceShip(level.get()).lock();
			report.Check(bool(ship), "spawn real player ship");
			level->BeginPlayInternal();
			level->TickInternal(0.f);
			ship->SetUseScreenClamp(false);
			// Preserve every authored system cell, adding a controlled Legendary
			// test region so all five effective levels exercise the real resolver.
			const auto original = schematic;
			schematic.width = 40; schematic.height = 40;
			schematic.cellZones.assign(1600, GameplayTags::Module::Zone::Hull);
			schematic.cellLocked.clear();
			for (int y = 0; y < original.height; ++y)
				for (int x = 0; x < original.width; ++x) schematic.cellZones[y * 40 + x] = original.GetZoneAt(x, y);
			report.Check(loadout.SeedDefaults(rules), "seed unchanged authored module defaults");
			// Player already applied identical defaults; force one revision to
			// pass the controlled schematic through the runtime applier as well.
			const auto* module = rules.lookup("Module.Utility.TwinRack.Legendary");
			report.Check(module != nullptr, "shipped Twin Rack exists");
			loadout.AddToInventory(rules, *module, 0, module->rarity, 1);
			ship->GetModuleApplier().Apply(loadout, rules);
		}

		~Fixture() { ship->Destroy(); PlayerManager::GetPlayerManager().Reset(); }
		LightYearsAbilitySystemComponent& Abilities() { return ship->GetAbilitySystemComponent(); }
		void Install(int effectiveLevel)
		{
			const int natural = std::min(effectiveLevel, 3);
			const auto zone = effectiveLevel <= 3 ? GameplayTags::Module::Zone::Nose : effectiveLevel == 4 ? GameplayTags::Module::Zone::Sensor : GameplayTags::Module::Zone::Hull;
			for (int y = 20; y < 22; ++y) for (int x = 20; x < 22; ++x) schematic.cellZones[y * 40 + x] = zone;
			const auto* module = rules.lookup("Module.Utility.TwinRack.Legendary");
			installed = loadout.AddToInventory(rules, *module, 0, module->rarity, natural);
			report.Check(installed && loadout.Install(rules, installed, { 20, 20 }, 0) == ModulePlacementFailure::None, "install shipped Twin Rack");
			report.Check(ship->GetModuleApplier().Apply(loadout, rules), "apply installed module through its owner");
			report.CheckValue("effective-level refund reaches runtime", .10f + .02f * (effectiveLevel - 1), ship->GetModuleApplier().GetLegendaryTwinRackRuntime().GetRules().front().pairedRefundFraction, std::to_string(effectiveLevel));
		}
		void Remove()
		{
			report.Check(loadout.Uninstall(installed) && ship->GetModuleApplier().Apply(loadout, rules), "uninstall through loadout and applier");
		}
		void Probes(int charges = 1)
		{
			for (int index = 0; index < 4; ++index)
			{
				std::string reason;
				report.Check(Abilities().GrantAbility(Probe(index, false), Slots[index], &reason).IsValid(), "grant first-bank probe", reason);
				report.Check(Abilities().AssignSecondAbility(Slots[index], Probe(index, true, index == 0 ? charges : 1), &reason), "assign independent second-bank probe", reason);
			}
		}
		void Tap(sas::AbilitySlot slot, bool control)
		{
			Abilities().SetLoadoutKeyInput(slot, true, control);
			Abilities().Tick(0.f);
			Abilities().SetLoadoutKeyInput(slot, false, control);
			Abilities().Tick(0.f);
		}
	};

	void MathAndInput(Report& report, Application& application)
	{
		for (int effective = 1; effective <= 5; ++effective)
		{
			Fixture f{ report, application };
			for (const auto& entry : GetDefaultAbilityLoadout())
			{
				report.Check(f.Abilities().GetAbility(entry.slot)->GetDefinition().abilityId == entry.abilityId, "default loadout matches its sole canonical owner");
				report.Check(f.Abilities().GetSecondAbility(entry.slot) == nullptr, "module never seeds a second default loadout");
			}
			f.Probes();
			std::array<float, 8> base;
			for (int i = 0; i < 4; ++i) { base[i] = f.Abilities().GetAbility(Slots[i])->GetCooldownDuration(); base[i + 4] = f.Abilities().GetSecondAbility(Slots[i])->GetCooldownDuration(); }
			f.Install(effective);
			const float refund = .10f + .02f * (effective - 1);
			for (int i = 0; i < 4; ++i)
			{
				auto* first = f.Abilities().GetAbility(Slots[i]);
				auto* second = f.Abilities().GetSecondAbility(Slots[i]);
				report.CheckValue("first cooldown receives fixed 1.30", base[i] * 1.30f, first->GetCooldownDuration(), std::to_string(i));
				report.CheckValue("second cooldown receives fixed 1.30", base[i + 4] * 1.30f, second->GetCooldownDuration(), std::to_string(i));
				f.Tap(Slots[i], true);
				report.Check(second->IsOnCooldown() && !first->IsOnCooldown(), "Ctrl+Q/E/F/R routes only to second ability");
				report.Check(!f.Abilities().DebugActivateAbility(second->GetHandle()), "failed cooldown activation gives no refund");
				const float remaining = second->GetCooldownRemaining();
				f.Tap(Slots[i], false);
				report.CheckValue("real use refunds pair remaining cooldown at effective level", remaining * (1.f - refund), second->GetCooldownRemaining(), std::to_string(effective));
				report.CheckValue("refund never chains back to caster", base[i] * 1.30f, first->GetCooldownRemaining(), "same slot");
			}
			f.Abilities().Tick(2.f);
			auto* first = f.Abilities().GetAbility(Slots[0]);
			auto* second = f.Abilities().GetSecondAbility(Slots[0]);
			const auto secondHandle = second->GetHandle();
			const float before = second->GetCooldownRemaining();
			f.Remove();
			report.Check(!f.Abilities().IsTwinRackEnabled() && f.Abilities().GetSecondAbility(Slots[0])->GetHandle() == secondHandle, "uninstall stores the same second instance");
			report.CheckValue("uninstall removes penalty from running recharge", before / 1.30f, second->GetCooldownRemaining(), "remaining timer");
			report.CheckValue("uninstall removes duration penalty", base[0], first->GetCooldownDuration(), "first bank");
			report.Check(!f.Abilities().DebugActivateAbility(secondHandle), "stored second set cannot activate");
			f.Install(effective);
			report.Check(f.Abilities().GetSecondAbility(Slots[0])->GetHandle() == secondHandle, "reinstall restores stored second set");
			report.CheckValue("reinstall restores remaining penalty once", before, second->GetCooldownRemaining(), "remaining timer");
			f.Abilities().Tick(100.f);
			f.Tap(Slots[0], false);
			report.CheckValue("ready pair stores no refund credit", 0.f, second->GetCooldownRemaining(), "ready pair");
			f.Tap(Slots[0], true);
			report.CheckValue("later use pays full cooldown", base[4] * 1.30f, second->GetCooldownRemaining(), "no stored refund");
			f.Remove();
			f.Abilities().Tick(100.f);
			report.Check(!second->IsOnCooldown() && !f.Abilities().DebugActivateAbility(secondHandle), "even a ready stored ability is unusable without Twin Rack");
			f.Install(effective);
			report.Check(f.Abilities().DebugActivateAbility(secondHandle), "reinstall restores actual second-bank usability");
		}
	}

	void ChargesAndExclusions(Report& report, Application& application)
	{
		Fixture f{ report, application }; f.Probes(3); f.Install(1);
		auto* first = f.Abilities().GetAbility(Slots[0]);
		auto* second = f.Abilities().GetSecondAbility(Slots[0]);
		f.Tap(Slots[0], true);
		f.Abilities().Tick(second->GetDefinition().chargeUseInterval);
		f.Tap(Slots[0], true);
		report.Check(second->BuildSnapshot().charges == 1, "two real uses spend two independent charges", second->BuildSnapshot().charges);
		f.Abilities().Tick(5.f);
		const float before = second->GetCooldownRemaining();
		f.Tap(Slots[0], false);
		report.CheckValue("charged pair refunds only actively recharging remainder", before * .90f, second->GetCooldownRemaining(), "partially elapsed recharge");
		report.Check(second->BuildSnapshot().charges == 1, "refund does not grant or touch another charge");
		f.Abilities().Tick(second->GetCooldownRemaining());
		report.Check(second->BuildSnapshot().charges == 2, "only the active charge completes");
		report.CheckValue("queued charge starts at full penalized duration", second->GetCooldownDuration(), second->GetCooldownRemaining(), "no refund on queued charge");

		// Retain a genuine channel in the secondary set, observing its actual
		// lifecycle rather than injecting fake activation events.
		auto channel = Probe(1, true);
		channel.abilityId = "Ability.Defense.TwinRackChannel.Basic";
		channel.activationPolicy = sas::AbilityActivationPolicy::WhileHeld;
		channel.lifetimePolicy = sas::AbilityLifetimePolicy::WhileInputHeld;
		std::string reason;
		report.Check(f.Abilities().AssignSecondAbility(Slots[1], channel, &reason), "replace idle second with a held channel", reason);
		f.Tap(Slots[1], false);
		auto* channelPair = f.Abilities().GetAbility(Slots[1]);
		f.Abilities().SetLoadoutKeyInput(Slots[1], true, true);
		f.Abilities().Tick(0.f);
		const float channelBefore = channelPair->GetCooldownRemaining();
		f.Abilities().SetLoadoutKeyInput(Slots[1], true, false);
		report.Check(!channelPair->IsInputHeld() && f.Abilities().GetSecondAbility(Slots[1])->IsInputHeld(), "Ctrl change during a held key never transfers the cast");
		f.Abilities().Tick(1.f);
		report.CheckValue("channel ticks do not refund", channelBefore - 1.f, channelPair->GetCooldownRemaining(), "held channel");
		f.Abilities().GetSecondAbility(Slots[1])->Cancel(sas::AbilityEndReason::Cancelled);
		report.CheckValue("cancel does not refund", channelBefore - 1.f, channelPair->GetCooldownRemaining(), "cancel");
		f.Abilities().SetLoadoutKeyInput(Slots[1], false, true);

		f.Abilities().Tick(100.f);
		f.Tap(Slots[0], false);
		const float firstBefore = first->GetCooldownRemaining();
		second->SetActivationOrigin(sas::AbilityActivationOrigin::EchoInvocation);
		report.Check(f.Abilities().DebugActivateAbility(second->GetHandle()), "cost-free replay executes through real lifecycle");
		report.CheckValue("cost-free replay produces no refund", firstBefore, first->GetCooldownRemaining(), "Echo origin");
		second->SetActivationOrigin(sas::AbilityActivationOrigin::NormalInput);
	}

	void ScrapAndHUD(Report& report, Application& application)
	{
		Fixture f{ report, application };
		std::string reason;
		report.Check(f.ship->GetAbilityLoadout().AssignSecondAbility("Ability.Utility.NullPulse.Basic", Slots[0], &reason), "public loadout API assigns shipped second ability", reason);
		report.Check(f.ship->GetAbilityLoadout().GetLoadout().FindSecondAbility(Slots[0]) != nullptr, "read-only loadout exposes stored second binding");
		f.Install(1);
		auto* first = f.Abilities().GetAbility(Slots[0]);
		auto* second = f.Abilities().GetSecondAbility(Slots[0]);
		const int firstLevel = first->GetLevel(), secondLevel = second->GetLevel();
		f.player.AwardScrap(100000);
		const auto scrap = f.player.GetScrap();
		const auto cost = second->GetDefinition().GetScrapCostToReachLevel(secondLevel + 1);
		report.Check(f.player.TryPurchaseAbilityLevel(Slots[0], true, &reason), "buy second ability through real Scrap path", reason);
		report.Check(second->GetLevel() == secondLevel + 1 && first->GetLevel() == firstLevel && f.player.GetScrap() == scrap - cost, "second Scrap purchase changes only its level and charges its own cost");
		report.Check(f.player.TryPurchaseAbilityLevel(Slots[0], &reason), "first Scrap path remains available", reason);
		report.Check(first->GetLevel() == firstLevel + 1 && second->GetLevel() == secondLevel + 1, "first purchase never raises its pair");
		f.Tap(Slots[0], true);
		report.Check(second->IsOnCooldown(), "shipped second executes with Ctrl input");
		const auto* recorded = f.Abilities().GetAbilityUseHistory().FindLatestUnconsumed();
		report.Check(recorded && recorded->abilityId.ToString() == second->GetDefinition().abilityId, "second-bank real use enters normal history");
		const auto replay = *recorded;
		f.Tap(Slots[0], false);
		report.Check(first->IsOnCooldown(), "real first ability enters cooldown before replay");
		const float firstBeforeReplay = first->GetCooldownRemaining(), secondBeforeReplay = second->GetCooldownRemaining();
		const int chargesBeforeReplay = second->BuildSnapshot().charges;
		report.Check(f.Abilities().InvokeRecordedAbility(replay, second->GetDefinition().scalingRules, 1.f, Slots[0], false, &reason), "execute actual cost-free recorded invocation", reason);
		report.CheckValue("recorded replay never refunds the first ability", firstBeforeReplay, first->GetCooldownRemaining(), "actual invocation runtime");
		report.CheckValue("recorded replay never refunds the second ability", secondBeforeReplay, second->GetCooldownRemaining(), "actual invocation runtime");
		report.Check(second->BuildSnapshot().charges == chargesBeforeReplay, "recorded replay spends no source charge");
		AbilityBarPresenter presenter; presenter.Tick();
		const auto model = presenter.GetViewModel();
		report.Check(model->slots[0].visible && model->slots[4].visible && model->slots[4].inputLabel == "Ctrl+Q", "HUD shows paired abilities and Ctrl binding");
		report.Check(model->slots[4].state == AbilitySlotState::Cooldown && model->slots[4].cooldownTenths > 0, "HUD reports second cooldown independently");
		AbilityBarView view{ EngineContext::Current(), model }; view.Tick(0.f);
		report.Check(view.GetColumn(0) && view.GetColumn(4), "HUD builds both paired rows");
		const auto handle = second->GetHandle();
		f.Remove(); presenter.Tick();
		report.Check(!model->slots[4].visible && model->slots[0].visible, "uninstall hides only second HUD row");
		f.Install(1); presenter.Tick();
		report.Check(model->slots[4].visible && second->GetHandle() == handle && second->GetLevel() == secondLevel + 1, "reinstall restores second binding and purchased level");
		for (const auto& binding : AbilityInputSchema::GetBindings())
			report.Check(binding.key != sf::Keyboard::Key::LShift && binding.key != sf::Keyboard::Key::RShift, "Shift remains reserved for Afterburner");
		report.Check(f.ship->GetModuleApplier().IsSystemEnabled(GameplayTags::Module::System::Afterburner), "Twin Rack preserves Afterburner system");
	}
}

int main(int argc, char** argv)
{
	EngineContext context; EngineContext::Scope scope{ context }; InstallVegaEngineAssets();
	if (argc != 2) return 2;
	Report report;
	try
	{
		context.Assets().SetAssetRootDirectory((std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/");
		report.Check(GameContentBootstrap::Register(), "register shipped content");
		PlayerManager::GetPlayerManager().Reset();
		std::string title = "Twin Rack E2E";
		Application application{ sf::Vector2u{ 128u, 128u }, 32u, title, sf::Style::None }; application.GetRenderWindow().setVisible(false);
		MathAndInput(report, application);
		ChargesAndExclusions(report, application);
		ScrapAndHUD(report, application);
		report.artifact["passed"] = true;
	}
	catch (const std::exception& error) { report.artifact["error"] = error.what(); std::cerr << error.what() << '\n'; }
	report.artifact["test"] = "LightYearsLegendaryTwinRackE2E";
	WriteArtifact(argv[1], report.artifact);
	return report.artifact.value("passed", false) ? 0 : 1;
}
