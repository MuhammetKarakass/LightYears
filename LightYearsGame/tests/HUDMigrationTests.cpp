#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "level/GameLevel.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/hud/ability/AbilityBarPresenter.h"
#include "presentation/hud/ability/AbilityBarView.h"
#include "presentation/hud/ability/AbilityBarHUDController.h"
#include "presentation/hud/GameplayWarningHUDController.h"
#include "presentation/hud/warning/GameplayWarningView.h"
#include "enemy/ChaosStage.h"
#include "presentation/hud/notification/NotificationHUDController.h"
#include "presentation/hud/notification/NotificationView.h"
#include "presentation/hud/encounter/EncounterHUDController.h"
#include "presentation/hud/encounter/EncounterHUDView.h"
#include "widget/GameHUD.h"
#include "widget/MainMenuHUD.h"
#include "widget/PauseMenuHUD.h"
#include "widget/GameOverHUD.h"
#include "widget/Button.h"
#include "widget/TextWidget.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ly
{
	struct HUDMigrationWarningTestAccess
	{
		static const std::shared_ptr<GameplayWarningViewModel>& Model(const GameplayWarningHUDController& controller)
		{
			return controller.mViewModel;
		}
		static std::shared_ptr<GameplayWarningView> View(const GameplayWarningHUDController& controller)
		{
			return controller.mView.lock();
		}
	};

	struct GameHUDDamageE2ETestAccess
	{
		static std::string Text(const TextWidget& widget) { return widget.mText.getString().toAnsiString(); }
	};
}

namespace
{
	using Json = nlohmann::json;

	class HeadlessAbilityLevel final : public ly::GameLevel
	{
	public:
		explicit HeadlessAbilityLevel(ly::Application* application) : GameLevel{ application } {}

	protected:
		void InitializeLevelSystems() override {}
		void CreateGameHUD() override {}
		void CreateHUDControllers() override {}
		void OnGameStart() override {}
	};

	struct PlayerManagerResetGuard
	{
		ly::PlayerManager& manager;
		~PlayerManagerResetGuard() { manager.Reset(); }
	};

	struct Report
	{
		Json artifact{ { "test", "HUDMigrationWarning" }, { "passed", false }, { "checks", Json::array() } };

		void Check(bool passed, const std::string& name, const Json& actual = nullptr)
		{
			Json check{ { "name", name }, { "passed", passed } };
			if (!actual.is_null()) check["actual"] = actual;
			artifact["checks"].push_back(std::move(check));
			if (!passed) throw std::runtime_error{ name };
		}
	};

	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.01f;
	}

	void RunWarningTest(Report& report)
	{
		using namespace ly;
		GameplayWarning warning;
		warning.title = "UNAUTHORIZED REGION";
		warning.message = "RETURN IN";
		warning.remainingTime = 2.5f;
		warning.hasCountdown = true;
		report.Check(FormatGameplayWarningText(warning) == "UNAUTHORIZED REGION\nRETURN IN 2.50", "countdown warning preserves legacy formatting");
		warning.hasCountdown = false;
		report.Check(FormatGameplayWarningText(warning) == "UNAUTHORIZED REGION\nRETURN IN", "static warning preserves legacy formatting");

		const GameplayWarningPulse atZero = ComputeGameplayWarningPulse(0.f);
		report.Check(atZero.color == sf::Color{ 255, 57, 57, 230 } && NearlyEqual(atZero.shake.x, 0.f) && NearlyEqual(atZero.shake.y, 0.f),
			"pulse at zero matches legacy color and position");
		const GameplayWarningPulse atPointOne = ComputeGameplayWarningPulse(0.1f);
		report.Check(atPointOne.color == sf::Color{ 255, 75, 75, 250 } && NearlyEqual(atPointOne.shake.x, -1.511f) &&
			NearlyEqual(atPointOne.shake.y, 0.260f), "pulse at 0.1s matches legacy formula",
			{ { "color", { atPointOne.color.r, atPointOne.color.g, atPointOne.color.b, atPointOne.color.a } },
				{ "shake", { atPointOne.shake.x, atPointOne.shake.y } } });

		std::string title = "HUD migration warning";
		Application application{ sf::Vector2u{ 1920u, 1080u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		auto hud = std::make_shared<GameHUD>();
		hud->NativeInit(application.GetRenderWindow());
		auto controller = std::make_unique<GameplayWarningHUDController>(hud);
		controller->ShowGameplayWarning(warning);
		const auto model = HUDMigrationWarningTestAccess::Model(*controller);
		const std::uint32_t activation = model->activation;
		report.Check(model->visible && model->text == "UNAUTHORIZED REGION\nRETURN IN" && activation == 1,
			"controller publishes visible warning and first activation");
		warning.title = "UNAUTHORIZED REGION";
		warning.message = "RETURN IN";
		warning.hasCountdown = true;
		warning.remainingTime = 1.25f;
		controller->ShowGameplayWarning(warning);
		report.Check(model->activation == activation && model->text == "UNAUTHORIZED REGION\nRETURN IN 1.25",
			"same type updates text without restarting activation");
		controller->HideGameplayWarning(static_cast<GameplayWarningType>(999));
		report.Check(model->visible, "different warning type cannot hide active warning");
		controller->HideGameplayWarning(GameplayWarningType::ArenaBoundary);
		report.Check(!model->visible, "matching warning type hides active warning");
		controller->ShowGameplayWarning(warning);
		report.Check(model->visible && model->activation == activation + 1, "show after hide starts new activation");

		controller->Tick(0.f);
		hud->Tick(0.f);
		auto view = HUDMigrationWarningTestAccess::View(*controller);
		report.Check(static_cast<bool>(view), "initialized HUD creates warning view in HUD layer");
		hud->Tick(0.f);
		const auto text = std::dynamic_pointer_cast<TextWidget>(view->GetChildren().front());
		const UIRect textRect = text->GetResolvedRect();
		report.Check(text->GetVisibility() && NearlyEqual(textRect.position.x + textRect.size.x * 0.5f, 960.f) &&
			std::abs(textRect.position.y + textRect.size.y * 0.5f - 54.f) <= 0.5f, "warning text layout centers at viewport top offset",
			{ { "viewport", { hud->GetViewportSize().x, hud->GetViewportSize().y } },
				{ "panelPosition", { view->GetResolvedRect().position.x, view->GetResolvedRect().position.y } },
				{ "panelSize", { view->GetResolvedRect().size.x, view->GetResolvedRect().size.y } },
				{ "textPosition", { textRect.position.x, textRect.position.y } },
				{ "textSize", { textRect.size.x, textRect.size.y } } });
		controller->HideGameplayWarning(GameplayWarningType::ArenaBoundary);
		hud->Tick(0.f);
		view->Tick(0.f);
		report.Check(!text->GetVisibility(), "view applies hidden state after revision changes");
		controller.reset();
		hud->Tick(0.f);
		report.Check(hud->GetLayer(UILayer::Hud).lock()->GetChildren().empty(), "controller teardown removes warning view from HUD layer");
		report.artifact["inputs"] = { { "viewport", { 1920, 1080 } } };
		report.artifact["passed"] = true;
	}

	void RunNotificationTest(Report& report)
	{
		using namespace ly;
		std::string title = "HUD migration notification";
		Application application{ sf::Vector2u{ 1920u, 1080u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		auto hud = std::make_shared<GameHUD>();
		hud->NativeInit(application.GetRenderWindow());
		auto controller = std::make_unique<NotificationHUDController>(hud);
		auto firstStage = std::make_shared<ChaosStage>(nullptr);
		controller->BindChaosStage(firstStage);
		for (int i = 0; i < 10; ++i)
			firstStage->onNotification.Broadcast("NOTICE " + std::to_string(i), 0.1f, 0.2f, 0.3f, { 20.f, 30.f }, 50.f, sf::Color::Red);
		auto model = controller->GetPresenter().GetViewModel();
		report.Check(model->pending.size() == 8 && model->pending.front().id == 3 && model->pending.back().id == 10,
			"notification queue retains only the latest eight monotonically identified requests",
			{ { "pending", model->pending.size() }, { "firstId", model->pending.front().id }, { "lastId", model->pending.back().id } });

		firstStage->onTotalChaosStarted.Broadcast(0.4f, 0.f, 0.f);
		firstStage->onChaosTimerUpdated.Broadcast(4.2f);
		const std::uint32_t timerRevision = model->revision.Get();
		firstStage->onChaosTimerUpdated.Broadcast(4.1f);
		report.Check(model->timerVisible && model->timerSeconds == 5 && model->revision.Get() == timerRevision,
			"timer ceil value coalesces updates within one displayed second");
		firstStage->onChaosTimerUpdated.Broadcast(3.9f);
		report.Check(model->timerSeconds == 4 && model->revision.Get() > timerRevision, "timer revision changes when displayed seconds change");

		controller->Tick(0.f);
		hud->Tick(0.f);
		auto layer = hud->GetLayer(UILayer::Hud).lock();
		shared_ptr<NotificationView> view;
		for (const shared_ptr<Widget>& child : layer->GetChildren())
			if (auto candidate = std::dynamic_pointer_cast<NotificationView>(child)) view = std::move(candidate);
		report.Check(static_cast<bool>(view) && view->GetChildren().size() == 9,
			"view consumes each pending notification once and retains its timer widget");
		if (view && !view->GetChildren().empty())
		{
			auto timer = std::dynamic_pointer_cast<TextWidget>(view->GetChildren().front());
			hud->Tick(0.f);
			const UIRect rect = timer->GetResolvedRect();
			report.Check(rect.position.y >= 0.f && std::abs(rect.position.y - 8.f) <= 1.f,
				"timer top anchor stays inside viewport", { { "top", rect.position.y } });
			TextWidget expectedTimer{ "TIME LEFT: 4", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 20 };
			report.Check(timer->GetVisibility() && NearlyEqual(timer->GetBound().size.x, expectedTimer.GetBound().size.x),
				"timer view displays the coalesced ceil value");
			firstStage->onTotalChaosEnded.Broadcast();
			hud->Tick(0.f);
			report.Check(!timer->GetVisibility(), "timer end hides the timer widget");
			hud->Tick(2.f);
			hud->Tick(0.f);
			report.Check(view->GetChildren().size() == 1, "SURVIVE notifications expire after fade and lifetime");
		}

		auto replacementStage = std::make_shared<ChaosStage>(nullptr);
		controller->BindChaosStage(replacementStage);
		const std::size_t pendingAfterRebind = model->pending.size();
		firstStage->onNotification.Broadcast("STALE", 0.f, 0.f, 0.f, {}, 50.f, sf::Color::Red);
		report.Check(model->pending.size() == pendingAfterRebind, "rebind clears the previous stage notification subscription");
		replacementStage->onNotification.Broadcast("CURRENT", 0.f, 0.f, 0.f, {}, 50.f, sf::Color::Red);
		report.Check(model->pending.back().text == "CURRENT", "replacement stage notifications remain connected");
		replacementStage.reset();
		firstStage.reset();
		controller.reset();
		hud->Tick(0.f);
		report.Check(layer->GetChildren().empty(), "controller teardown removes notification view from HUD layer");
		report.artifact["inputs"] = { { "viewport", { 1920, 1080 } } };
		report.artifact["passed"] = true;
	}

	void RunEncounterTest(Report& report)
	{
		using namespace ly;
		std::string title = "HUD migration encounter";
		Application application{ sf::Vector2u{ 1920u, 1080u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		auto hud = std::make_shared<GameHUD>();
		hud->NativeInit(application.GetRenderWindow());
		EncounterWaveSnapshot snapshot{};
		auto controller = std::make_unique<EncounterHUDController>(hud, [&snapshot]() { return snapshot; });
		controller->Tick(0.f);
		hud->Tick(0.f);
		auto layer = hud->GetLayer(UILayer::Hud).lock();
		shared_ptr<EncounterHUDView> view;
		for (const shared_ptr<Widget>& child : layer->GetChildren())
			if (auto candidate = std::dynamic_pointer_cast<EncounterHUDView>(child)) view = std::move(candidate);
		report.Check(static_cast<bool>(view) && !view->GetVisibility(), "idle encounter creates a hidden layered view");

		snapshot.state = EncounterWaveState::Spawning;
		snapshot.currentWaveNumber = 2;
		snapshot.totalWaveCount = 3;
		snapshot.aliveEnemyCount = 3;
		snapshot.remainingSpawnCount = 1;
		snapshot.enemyLevel = 2;
		controller->Tick(0.f);
		hud->Tick(0.f);
		const auto presentation = controller->GetPresentation();
		report.Check(view->GetVisibility() && view->GetChildren().size() == 1 &&
			view->GetChildren().front()->GetChildren().size() == 2 && view->GetPresentedTitle() == "WAVE 2 / 3" &&
			presentation->detail == "ENEMIES 4  \xE2\x80\xA2  LEVEL 2",
			"spawning encounter displays two correctly projected lines",
			{ { "title", view->GetPresentedTitle() }, { "detail", presentation->detail } });
		auto lines = std::dynamic_pointer_cast<StackPanel>(view->GetChildren().front());
		auto titleWidget = std::dynamic_pointer_cast<TextWidget>(lines->GetChildren().front());
		const UIRect titleRect = titleWidget->GetResolvedRect();
		report.Check(std::abs(titleRect.position.x + titleRect.size.x * 0.5f - 960.f) <= 1.f &&
			std::abs(titleRect.position.y - 24.f) <= 1.f, "encounter stack anchors at viewport top center offset 24",
			{ { "titleRect", { titleRect.position.x, titleRect.position.y, titleRect.size.x, titleRect.size.y } } });

		snapshot.state = EncounterWaveState::InterWaveDelay;
		snapshot.currentWaveNumber = 2;
		snapshot.interWaveRemainingTime = 1.2f;
		controller->Tick(0.f);
		hud->Tick(0.f);
		report.Check(view->GetVisibility() && view->GetPresentedTitle() == "WAVE 2 CLEARED" &&
			presentation->detail == "NEXT WAVE IN 2", "inter-wave view preserves ceil countdown wording");

		snapshot.state = EncounterWaveState::Completed;
		controller->Tick(0.f);
		hud->Tick(0.f);
		report.Check(view->GetVisibility() && view->GetPresentedTitle() == "ENCOUNTER COMPLETE" &&
			view->GetPresentedTitleColor() == sf::Color{ 140, 255, 190, 255 } && presentation->detail.empty(),
			"completed encounter displays one line in the legacy completed color");

		snapshot.state = EncounterWaveState::Failed;
		controller->Tick(0.f);
		hud->Tick(0.f);
		report.Check(!view->GetVisibility(), "failed encounter view is hidden");

		snapshot.state = EncounterWaveState::Spawning;
		controller->Tick(0.f);
		hud->Tick(0.f);
		const std::uint32_t stableRevision = presentation->revision.Get();
		const unsigned int stableRefreshCount = view->GetRefreshCount();
		for (int frame = 0; frame < 16; ++frame)
		{
			controller->Tick(1.f / 60.f);
			hud->Tick(1.f / 60.f);
		}
		report.Check(presentation->revision.Get() == stableRevision && view->GetRefreshCount() == stableRefreshCount,
			"unchanged encounter snapshots preserve revision and view refresh count");

		controller.reset();
		hud->Tick(0.f);
		report.Check(layer->GetChildren().empty(), "controller destruction removes encounter view from HUD layer");

		auto weakSnapshotOwner = std::make_shared<EncounterWaveSnapshot>();
		weakSnapshotOwner->state = EncounterWaveState::Spawning;
		weak_ptr<EncounterWaveSnapshot> weakSnapshot = weakSnapshotOwner;
		auto detachedController = std::make_unique<EncounterHUDController>(hud, [weakSnapshot]()
		{
			if (const auto liveSnapshot = weakSnapshot.lock()) return *liveSnapshot;
			return EncounterWaveSnapshot{};
		});
		detachedController->Tick(0.f);
		hud->Tick(0.f);
		weakSnapshotOwner.reset();
		detachedController->Tick(0.f);
		hud->Tick(0.f);
		view.reset();
		for (const shared_ptr<Widget>& child : layer->GetChildren())
			if (auto candidate = std::dynamic_pointer_cast<EncounterHUDView>(child)) view = std::move(candidate);
		report.Check(weakSnapshot.expired() && view && !view->GetVisibility(),
			"expired weak snapshot provider degrades to an empty hidden encounter");
		detachedController.reset();
		hud->Tick(0.f);
		report.Check(layer->GetChildren().empty(), "detached encounter controller teardown removes its view");
		report.artifact["inputs"] = { { "viewport", { 1920, 1080 } } };
		report.artifact["passed"] = true;
	}

	void RunAbilityTest(Report& report)
	{
		using namespace ly;
		std::string title = "HUD migration ability";
		Application application{ sf::Vector2u{ 1920u, 1080u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		PlayerManager& manager = PlayerManager::GetPlayerManager();
		manager.Reset();
		PlayerManagerResetGuard resetGuard{ manager };
		auto level = std::make_shared<HeadlessAbilityLevel>(&application);
		Player& player = manager.CreateNewPlayer();
		auto ship = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(ship), "shipped player ship spawned");
		level->BeginPlayInternal();
		level->TickInternal(0.f);

		AbilityBarPresenter presenter;
		presenter.Tick();
		auto model = presenter.GetViewModel();
		const std::uint32_t firstGeneration = model->shipGeneration;
		std::size_t visibleCount = 0;
		for (std::size_t index = 0; index < model->slots.size(); ++index)
		{
			const AbilitySlotViewData& data = model->slots[index];
			if (!data.visible) continue;
			++visibleCount;
			const sas::AbilitySlot slot = static_cast<sas::AbilitySlot>(
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index));
			report.Check(!data.iconPath.empty() && data.inputLabel == AbilityInputSchema::GetLabel(slot),
				"visible shipped ability has its icon and schema key label",
				{ { "slot", index }, { "icon", data.iconPath }, { "label", data.inputLabel } });
		}
		report.Check(visibleCount > 0, "shipped player abilities populate the bar model",
			{ { "visibleSlots", visibleCount }, { "generation", firstGeneration } });

		const UIRevision stableRevision = model->revision;
		presenter.Tick();
		presenter.Tick();
		report.Check(model->revision.Get() == stableRevision.Get(), "two unchanged presenter ticks preserve revision");

		LightYearsAbilitySystemComponent& abilitySystem = ship->GetAbilitySystemComponent();
		GameAbility* activatedAbility = nullptr;
		sas::AbilitySlot activatedSlot = sas::AbilitySlot::Ability1;
		for (std::size_t index = 0; index < model->slots.size(); ++index)
		{
			if (!model->slots[index].visible) continue;
			const auto slot = static_cast<sas::AbilitySlot>(
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index));
			auto* ability = abilitySystem.GetAbility(slot);
			if (!ability || ability->GetCooldownDuration() <= 0.f) continue;
			if (ability->TryActivate())
			{
				activatedAbility = ability;
				activatedSlot = slot;
				break;
			}
		}
		report.Check(activatedAbility != nullptr, "a shipped cooldown ability activates through its runtime instance");
		presenter.Tick();
		const std::size_t activeIndex = static_cast<std::size_t>(static_cast<int>(activatedSlot) -
			static_cast<int>(sas::AbilitySlot::Ability1));
		report.Check(activatedAbility && model->slots[activeIndex].state != AbilitySlotState::Ready,
			"activation publishes an active or cooldown state");
		for (int tick = 0; activatedAbility && activatedAbility->IsActive() && tick < 600; ++tick)
		{
			abilitySystem.Tick(0.1f);
		}
		presenter.Tick();
		if (activatedAbility && activatedAbility->IsOnCooldown())
		{
			const int before = model->slots[activeIndex].cooldownTenths;
			abilitySystem.Tick(0.25f);
			presenter.Tick();
			const int after = model->slots[activeIndex].cooldownTenths;
			report.Check(model->slots[activeIndex].state == AbilitySlotState::Cooldown && after < before,
				"cooldown projection counts down in tenths",
				{ { "beforeTenths", before }, { "afterTenths", after } });
		}
		else
		{
			report.Check(false, "activated ability enters cooldown after its active duration",
				{ { "active", activatedAbility && activatedAbility->IsActive() },
					{ "state", static_cast<int>(model->slots[activeIndex].state) } });
		}

		bool leveled = false;
		std::string previousStats;
		std::string updatedStats;
		int newLevel = 0;
		for (std::size_t index = 0; index < model->slots.size(); ++index)
		{
			if (!model->slots[index].visible) continue;
			const auto slot = static_cast<sas::AbilitySlot>(
				static_cast<int>(sas::AbilitySlot::Ability1) + static_cast<int>(index));
			auto* ability = abilitySystem.GetAbility(slot);
			if (!ability) continue;
			previousStats = model->slots[index].statsText;
			newLevel = ability->GetLevel() + 1;
			if (!abilitySystem.SetAbilityLevel(slot, newLevel)) continue;
			presenter.Tick();
			updatedStats = model->slots[index].statsText;
			leveled = updatedStats != previousStats && updatedStats.find("Lv " + std::to_string(newLevel)) != std::string::npos;
			if (leveled) break;
		}
		report.Check(leveled, "ability level event rebuilds temporary stat text",
			{ { "before", previousStats }, { "after", updatedStats }, { "level", newLevel } });

		auto oldShip = ship;
		const unsigned int oldShipId = oldShip->GetUniqueID();
		oldShip->Destroy();
		level->TickInternal(0.f);
		presenter.Tick();
		report.Check(std::none_of(model->slots.begin(), model->slots.end(), [](const AbilitySlotViewData& slot) { return slot.visible; }),
			"destroyed ship hides all bar slots");
		auto replacementShip = player.SpawnSpaceShip(level.get()).lock();
		report.Check(static_cast<bool>(replacementShip), "player respawns a replacement ship");
		level->TickInternal(0.f);
		presenter.Tick();
		const std::uint32_t respawnGeneration = model->shipGeneration;
		report.Check(respawnGeneration > firstGeneration && respawnGeneration > 1 &&
			std::any_of(model->slots.begin(), model->slots.end(), [](const AbilitySlotViewData& slot) { return slot.visible; }),
			"respawn advances ship generation and binds replacement slots",
			{ { "oldShipId", oldShipId }, { "generation", respawnGeneration } });
		const std::uint32_t reboundRevision = model->revision.Get();
		oldShip->GetAbilitySystemComponent().onAbilityLevelChanged.Broadcast({}, 999);
		presenter.Tick();
		report.Check(model->revision.Get() == reboundRevision, "old ship delegate cannot change the rebound view model");

		auto geometryModel = std::make_shared<AbilityBarViewModel>(*model);
		geometryModel->slots[1] = geometryModel->slots[0];
		geometryModel->slots[1].inputLabel = "E";
		geometryModel->slots[0].statsText = "Dash\nLv 1";
		geometryModel->slots[1].statsText = "Pulse\nLv 3\nDamage 20\nRange 30\nDuration 1";
		for (std::size_t index = 2; index < geometryModel->slots.size(); ++index)
			geometryModel->slots[index].visible = false;
		geometryModel->slots[0].state = AbilitySlotState::Cooldown;
		geometryModel->slots[0].cooldownTenths = 15;
		geometryModel->slots[1].state = AbilitySlotState::Ready;
		geometryModel->revision.Bump();
		AbilityBarView view{ geometryModel };
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		view.NativeTick(0.f);
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		const UIRect firstIcon = view.GetIcon(0)->GetResolvedRect();
		const UIRect secondIcon = view.GetIcon(1)->GetResolvedRect();
		const auto firstStats = std::dynamic_pointer_cast<TextWidget>(view.GetColumn(0)->GetChildren().front());
		const auto secondStats = std::dynamic_pointer_cast<TextWidget>(view.GetColumn(1)->GetChildren().front());
		const auto firstInput = std::dynamic_pointer_cast<TextWidget>(view.GetColumn(0)->GetChildren()[1]);
		const auto secondInput = std::dynamic_pointer_cast<TextWidget>(view.GetColumn(1)->GetChildren()[1]);
		const auto firstState = std::dynamic_pointer_cast<TextWidget>(view.GetColumn(0)->GetChildren().back());
		const auto secondState = std::dynamic_pointer_cast<TextWidget>(view.GetColumn(1)->GetChildren().back());
		const float groupCenter = (firstIcon.position.x + firstIcon.size.x * .5f + secondIcon.position.x + secondIcon.size.x * .5f) * .5f;
		report.Check(view.GetColumn(0)->GetVisibility() && view.GetColumn(1)->GetVisibility() &&
			!view.GetColumn(2)->GetVisibility() && NearlyEqual(firstIcon.position.y, 968.f) &&
			NearlyEqual(secondIcon.position.y, 968.f) && firstStats && secondStats &&
			secondStats->GetResolvedRect().size.y > firstStats->GetResolvedRect().size.y && firstState && secondState &&
			firstInput && secondInput && NearlyEqual(firstInput->GetWidgetLocation().x, firstIcon.position.x + 2.f) &&
			NearlyEqual(secondInput->GetWidgetLocation().x, secondIcon.position.x + 2.f) &&
			NearlyEqual(firstInput->GetWidgetLocation().y, 948.f) && NearlyEqual(secondInput->GetWidgetLocation().y, 948.f) &&
			NearlyEqual(firstStats->GetWidgetLocation().x, firstIcon.position.x) &&
			NearlyEqual(secondStats->GetWidgetLocation().x, secondIcon.position.x) &&
			NearlyEqual(firstStats->GetWidgetLocation().y, 942.f - firstStats->GetResolvedRect().size.y) &&
			NearlyEqual(secondStats->GetWidgetLocation().y, 942.f - secondStats->GetResolvedRect().size.y) &&
			NearlyEqual(firstState->GetWidgetLocation().x, firstIcon.position.x) &&
			NearlyEqual(secondState->GetWidgetLocation().x, secondIcon.position.x) &&
			NearlyEqual(firstState->GetWidgetLocation().y, 1020.f) && NearlyEqual(secondState->GetWidgetLocation().y, 1020.f),
			"unequal stat blocks preserve legacy raw text origins and icon/state placement",
			{ { "icons", { { firstIcon.position.x, firstIcon.position.y, firstIcon.size.x, firstIcon.size.y },
				{ secondIcon.position.x, secondIcon.position.y, secondIcon.size.x, secondIcon.size.y } } } });
		report.Check(NearlyEqual(secondIcon.position.x - firstIcon.position.x, 178.f) &&
			NearlyEqual(groupCenter, 960.f - 24.f + firstIcon.size.x * .5f),
			"actual icon bounds retain the legacy 48px logical-cell center and 178px pitch",
			{ { "groupCenter", groupCenter }, { "logicalCellCenter", 960.f - 24.f + firstIcon.size.x * .5f },
				{ "pitch", secondIcon.position.x - firstIcon.position.x } });
		report.Check(NearlyEqual(view.GetIcon(0)->GetEffectiveAlpha(), .35f) &&
			NearlyEqual(view.GetIcon(1)->GetEffectiveAlpha(), .9f), "cooldown and ready icons apply legacy alpha");
		const unsigned int initialViewRefreshes = view.GetRefreshCount();
		view.NativeTick(0.f);
		report.Check(view.GetRefreshCount() == initialViewRefreshes, "unchanged view model does not refresh the bar");
		geometryModel->slots[2] = geometryModel->slots[0];
		geometryModel->slots[2].inputLabel = "F";
		geometryModel->slots[3] = geometryModel->slots[0];
		geometryModel->slots[3].inputLabel = "R";
		geometryModel->slots[2].state = AbilitySlotState::Active;
		geometryModel->slots[3].state = AbilitySlotState::Ready;
		geometryModel->revision.Bump();
		view.NativeTick(0.f);
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		const auto firstFourIcon = view.GetIcon(0)->GetResolvedRect();
		const auto secondFourIcon = view.GetIcon(1)->GetResolvedRect();
		const auto thirdFourIcon = view.GetIcon(2)->GetResolvedRect();
		const auto fourthFourIcon = view.GetIcon(3)->GetResolvedRect();
		const float fourIconCenter = (firstFourIcon.position.x + fourthFourIcon.position.x) * .5f +
			(firstFourIcon.size.x + fourthFourIcon.size.x) * .25f;
		report.Check(view.GetColumn(0)->GetVisibility() && view.GetColumn(1)->GetVisibility() &&
			view.GetColumn(2)->GetVisibility() && view.GetColumn(3)->GetVisibility() &&
			NearlyEqual(fourIconCenter, 960.f - 24.f + firstFourIcon.size.x * .5f) &&
			NearlyEqual(firstFourIcon.size.x, fourthFourIcon.size.x) &&
			NearlyEqual(secondFourIcon.position.x - firstFourIcon.position.x, 178.f) &&
			NearlyEqual(thirdFourIcon.position.x - secondFourIcon.position.x, 178.f) &&
			NearlyEqual(fourthFourIcon.position.x - thirdFourIcon.position.x, 178.f) &&
			NearlyEqual(firstFourIcon.position.y, 968.f) && NearlyEqual(fourthFourIcon.position.y, 968.f),
			"four visible icon bounds preserve the legacy 48px logical-cell center, 178px pitch, and top",
			{ { "iconXs", { firstFourIcon.position.x, secondFourIcon.position.x, thirdFourIcon.position.x, fourthFourIcon.position.x } },
				{ "center", fourIconCenter } });
		geometryModel->slots[1].visible = false;
		geometryModel->slots[2].visible = false;
		geometryModel->slots[3].visible = false;
		geometryModel->revision.Bump();
		view.NativeTick(0.f);
		view.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		const UIRect remainingIcon = view.GetIcon(0)->GetResolvedRect();
		report.Check(!view.GetColumn(1)->GetVisibility() && NearlyEqual(remainingIcon.position.x + remainingIcon.size.x * .5f,
			960.f - 24.f + remainingIcon.size.x * .5f), "hidden slots collapse and recenter the remaining 48px logical cell");

		auto lateModel = std::make_shared<AbilityBarViewModel>();
		AbilityBarView lateBoundView{ lateModel };
		lateBoundView.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		lateBoundView.NativeTick(0.f);
		const bool initiallyEmpty = !lateBoundView.GetIcon(0);
		lateModel->slots[0] = geometryModel->slots[0];
		lateModel->shipGeneration = geometryModel->shipGeneration + 1;
		lateModel->revision.Bump();
		lateBoundView.NativeTick(0.f);
		lateBoundView.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		const sf::Vector2f reboundIconSize = lateBoundView.GetIcon(0)->GetBound().size;
		const sf::Vector2f sourceIconSize = view.GetIcon(0)->GetBound().size;
		report.Check(initiallyEmpty && lateBoundView.GetColumn(0)->GetVisibility() &&
			NearlyEqual(reboundIconSize.x, sourceIconSize.x) && NearlyEqual(reboundIconSize.y, sourceIconSize.y),
			"empty model binds a correctly sized ability icon when its first ship arrives",
			{ { "reboundIconSize", { reboundIconSize.x, reboundIconSize.y } },
				{ "sourceIconSize", { sourceIconSize.x, sourceIconSize.y } } });

		auto textureModel = std::make_shared<AbilityBarViewModel>();
		textureModel->shipGeneration = 1;
		textureModel->slots[0].visible = true;
		textureModel->slots[0].iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
		textureModel->slots[1].visible = true;
		textureModel->slots[1].iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		textureModel->revision.Bump();
		AbilityBarView textureReloadView{ textureModel };
		textureReloadView.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		textureReloadView.NativeTick(0.f);
		auto oldIcon = textureReloadView.GetIcon(0);
		const sf::Vector2f oldIconSize = oldIcon->GetBound().size;
		textureModel->slots[0].iconPath = textureModel->slots[1].iconPath;
		textureModel->revision.Bump();
		textureReloadView.NativeTick(0.f);
		textureReloadView.ResolveLayoutTree({ { 0.f, 0.f }, { 1920.f, 1080.f } }, true);
		auto newIcon = textureReloadView.GetIcon(0);
		const sf::Vector2f newIconSize = newIcon->GetBound().size;
		const auto& reloadedColumnChildren = textureReloadView.GetColumn(0)->GetChildren();
		report.Check(oldIcon->IsExpired() && newIcon != oldIcon && oldIconSize != newIconSize &&
			reloadedColumnChildren.size() == 4 && std::none_of(reloadedColumnChildren.begin(), reloadedColumnChildren.end(),
			[&oldIcon](const shared_ptr<Widget>& child) { return child == oldIcon; }),
			"different-size icon path change replaces the old sprite child and uses the new asset bounds",
			{ { "oldIconSize", { oldIconSize.x, oldIconSize.y } }, { "newIconSize", { newIconSize.x, newIconSize.y } } });

		auto hud = std::make_shared<GameHUD>();
		hud->NativeInit(application.GetRenderWindow());
		auto barController = std::make_unique<AbilityBarHUDController>(hud);
		barController->Tick(0.f);
		hud->Tick(0.f);
		auto hudLayer = hud->GetLayer(UILayer::Hud).lock();
		report.Check(hudLayer && !hudLayer->GetChildren().empty(), "ability controller adds its view after HUD initialization");
		barController.reset();
		hud->Tick(0.f);
		report.Check(hudLayer && hudLayer->GetChildren().empty(), "ability controller teardown removes its view");
		report.artifact["inputs"] = { { "initialGeneration", firstGeneration }, { "respawnGeneration", respawnGeneration },
			{ "visibleSlots", visibleCount }, { "oldShipId", oldShipId }, { "viewChecks", 10 } };
		report.artifact["passed"] = true;
	}

	template<typename T>
	void CollectWidgets(const std::shared_ptr<ly::Widget>& parent, std::vector<std::shared_ptr<T>>& result)
	{
		for (const auto& child : parent->GetChildren())
		{
			if (auto typed = std::dynamic_pointer_cast<T>(child)) result.push_back(typed);
			CollectWidgets<T>(child, result);
		}
	}

	struct MenuActionCounts
	{
		int mainStart{ 0 }, mainQuit{ 0 };
		int resume{ 0 }, pauseRestart{ 0 }, pauseQuit{ 0 }, pauseMainMenu{ 0 };
		int gameRestart{ 0 }, gameQuit{ 0 }, gameMainMenu{ 0 };
		void MainStart() { ++mainStart; }
		void MainQuit() { ++mainQuit; }
		void Resume() { ++resume; }
		void PauseRestart() { ++pauseRestart; }
		void PauseQuit() { ++pauseQuit; }
		void PauseMainMenu() { ++pauseMainMenu; }
		void GameRestart() { ++gameRestart; }
		void GameQuit() { ++gameQuit; }
		void GameMainMenu() { ++gameMainMenu; }
	};

	void SortButtonsByTop(std::vector<std::shared_ptr<ly::Button>>& buttons)
	{
		std::sort(buttons.begin(), buttons.end(), [](const auto& left, const auto& right)
		{
			return left->GetBound().position.y < right->GetBound().position.y;
		});
	}

	bool ButtonCenterNear(const ly::Button& button, float x, float y)
	{
		const sf::FloatRect bounds = button.GetBound();
		return std::abs(bounds.position.x + bounds.size.x * .5f - x) <= 2.f &&
			std::abs(bounds.position.y + bounds.size.y * .5f - y) <= 2.f;
	}

	bool ClickButton(ly::HUD& hud, ly::Button& button)
	{
		const sf::FloatRect bounds = button.GetBound();
		const sf::Vector2i position{
			static_cast<int>(std::lround(bounds.position.x + bounds.size.x * .5f)),
			static_cast<int>(std::lround(bounds.position.y + bounds.size.y * .5f)) };
		const bool pressed = hud.HandleEvent(sf::Event{ sf::Event::MouseButtonPressed{ sf::Mouse::Button::Left, position } });
		const bool released = hud.HandleEvent(sf::Event{ sf::Event::MouseButtonReleased{ sf::Mouse::Button::Left, position } });
		return pressed && released;
	}

	bool LegacyTextCenterNear(ly::TextWidget& actual, const std::string& text, unsigned int size, float authoredCenterY, float viewportWidth)
	{
		ly::TextWidget legacy{ text };
		legacy.SetTextSize(size);
		const sf::FloatRect legacyGlyphBounds = legacy.GetBound();
		const sf::FloatRect actualBounds = actual.GetBound();
		return std::abs(actualBounds.position.x + actualBounds.size.x * .5f - (viewportWidth * .5f + legacyGlyphBounds.position.x)) <= .5f &&
			std::abs(actualBounds.position.y + actualBounds.size.y * .5f - (authoredCenterY + legacyGlyphBounds.position.y)) <= .5f;
	}

	void RunMenusTest(Report& report)
	{
		using namespace ly;
		std::string title = "HUD migration menus";
		Application application{ sf::Vector2u{ 1920u, 1080u }, 32u, title, sf::Style::None };
		application.GetRenderWindow().setVisible(false);
		MenuActionCounts mainActions;
		auto mainMenu = std::make_shared<MainMenuHUD>();
		mainMenu->NativeInit(application.GetRenderWindow());
		mainMenu->Tick(0.f);
		mainMenu->onStartButtonClicked.BindAction(&mainActions, &MenuActionCounts::MainStart);
		mainMenu->onQuitButtonClicked.BindAction(&mainActions, &MenuActionCounts::MainQuit);
		auto mainLayer = mainMenu->GetLayer(UILayer::Menu).lock();
		std::vector<std::shared_ptr<Button>> mainButtons;
		std::vector<std::shared_ptr<TextWidget>> mainTexts;
		CollectWidgets<Button>(mainLayer, mainButtons);
		CollectWidgets<TextWidget>(mainLayer, mainTexts);
		SortButtonsByTop(mainButtons);
		report.Check(mainButtons.size() == 2 && mainTexts.size() == 1, "main menu builds its title and two button children");
		if (mainButtons.size() != 2 || mainTexts.size() != 1) return;
		report.Check(ButtonCenterNear(*mainButtons[0], 960.f, 540.f) && ButtonCenterNear(*mainButtons[1], 960.f, 640.f),
			"main menu centers Start at viewport midpoint and retains its 100px pitch");
		report.Check(LegacyTextCenterNear(*mainTexts.front(), "Light Years", 40, 100.f, 1920.f),
			"main menu title preserves its legacy glyph-bound center");

		const sf::FloatRect mainFirstBounds = mainButtons[0]->GetBound();
		const sf::FloatRect mainSecondBounds = mainButtons[1]->GetBound();
		mainMenu->HandleEvent(sf::Event{ sf::Event::MouseMoved{ { static_cast<int>(mainFirstBounds.position.x + 10.f), static_cast<int>(mainFirstBounds.position.y + 10.f) } } });
		const bool firstOnlyHovered = mainButtons[0]->IsHovered() && !mainButtons[1]->IsHovered();
		mainMenu->HandleEvent(sf::Event{ sf::Event::MouseMoved{ { static_cast<int>(mainSecondBounds.position.x + 10.f), static_cast<int>(mainSecondBounds.position.y + 10.f) } } });
		report.Check(firstOnlyHovered && !mainButtons[0]->IsHovered() && mainButtons[1]->IsHovered(),
			"moving hover from Start to Quit clears the first button and hovers the second");
		const bool mainClicksHandled = ClickButton(*mainMenu, *mainButtons[0]) && ClickButton(*mainMenu, *mainButtons[1]);
		report.Check(mainClicksHandled && mainActions.mainStart == 1 && mainActions.mainQuit == 1,
			"main menu press and release broadcast each public action once");
		mainMenu->SetViewportSize({ 1280u, 720u });
		mainLayer->ResolveLayoutTree({ { 0.f, 0.f }, { 1280.f, 720.f } }, true);
		report.Check(ButtonCenterNear(*mainButtons[0], 640.f, 360.f) && ButtonCenterNear(*mainButtons[1], 640.f, 460.f),
			"main menu remains centered at a 1280x720 viewport");

		MenuActionCounts pauseActions;
		auto pauseMenu = std::make_shared<PauseMenuHUD>();
		pauseMenu->NativeInit(application.GetRenderWindow());
		pauseMenu->Tick(0.f);
		pauseMenu->onResumeButtonClicked.BindAction(&pauseActions, &MenuActionCounts::Resume);
		pauseMenu->onRestartButtonClicked.BindAction(&pauseActions, &MenuActionCounts::PauseRestart);
		pauseMenu->onQuitButtonClicked.BindAction(&pauseActions, &MenuActionCounts::PauseQuit);
		pauseMenu->onMainMenuButtonClicked.BindAction(&pauseActions, &MenuActionCounts::PauseMainMenu);
		auto modalLayer = pauseMenu->GetLayer(UILayer::Modal).lock();
		std::vector<std::shared_ptr<Button>> pauseButtons;
		std::vector<std::shared_ptr<TextWidget>> pauseTexts;
		std::vector<std::shared_ptr<Panel>> pausePanels;
		CollectWidgets<Button>(modalLayer, pauseButtons);
		CollectWidgets<TextWidget>(modalLayer, pauseTexts);
		CollectWidgets<Panel>(modalLayer, pausePanels);
		SortButtonsByTop(pauseButtons);
		report.Check(pauseButtons.size() == 4 && pauseTexts.size() == 1 && !pausePanels.empty(), "pause menu builds overlay, title, and four button children");
		if (pauseButtons.size() != 4 || pauseTexts.size() != 1 || pausePanels.empty()) return;
		const UIRect pauseOverlay = pausePanels.front()->GetResolvedRect();
		report.Check(NearlyEqual(pauseOverlay.position.x, 50.f) && NearlyEqual(pauseOverlay.position.y, 50.f) &&
			NearlyEqual(pauseOverlay.size.x, 1820.f) && NearlyEqual(pauseOverlay.size.y, 980.f), "pause overlay keeps 50px viewport insets");
		report.Check(ButtonCenterNear(*pauseButtons[0], 960.f, 200.f) && ButtonCenterNear(*pauseButtons[1], 960.f, 275.f) &&
			ButtonCenterNear(*pauseButtons[2], 960.f, 350.f) && ButtonCenterNear(*pauseButtons[3], 960.f, 425.f),
			"pause buttons keep Resume, Restart, Quit, Main Menu order and 75px pitch");
		report.Check(LegacyTextCenterNear(*pauseTexts.front(), "Paused", 40, 100.f, 1920.f), "pause title preserves its legacy glyph-bound center");
		const sf::Event outsideMove{ sf::Event::MouseMoved{ { 5, 5 } } };
		report.Check(pauseMenu->HandleEvent(outsideMove), "visible modal HUD captures outside events after layer dispatch");
		const sf::Event escapeEvent{ sf::Event::KeyPressed{ sf::Keyboard::Key::Escape } };
		report.Check(pauseMenu->HandleEvent(escapeEvent) && pauseActions.resume == 1,
			"Escape broadcasts Resume even when HUD base captures the modal event");
		bool pauseClicksHandled = true;
		for (const auto& button : pauseButtons) pauseClicksHandled = ClickButton(*pauseMenu, *button) && pauseClicksHandled;
		report.Check(pauseClicksHandled && pauseActions.resume == 2 && pauseActions.pauseRestart == 1 &&
			pauseActions.pauseQuit == 1 && pauseActions.pauseMainMenu == 1,
			"each pause button broadcasts its existing public delegate exactly once");

		MenuActionCounts gameActions;
		auto gameOver = std::make_shared<GameOverHUD>();
		gameOver->SetTitleText("You Win!");
		gameOver->SetScoreText(42);
		gameOver->NativeInit(application.GetRenderWindow());
		gameOver->Tick(0.f);
		gameOver->onRestartButtonClicked.BindAction(&gameActions, &MenuActionCounts::GameRestart);
		gameOver->onQuitButtonClicked.BindAction(&gameActions, &MenuActionCounts::GameQuit);
		gameOver->onMainMenuButtonClicked.BindAction(&gameActions, &MenuActionCounts::GameMainMenu);
		auto gameOverLayer = gameOver->GetLayer(UILayer::Modal).lock();
		std::vector<std::shared_ptr<Button>> gameButtons;
		std::vector<std::shared_ptr<TextWidget>> gameTexts;
		std::vector<std::shared_ptr<Panel>> gamePanels;
		CollectWidgets<Button>(gameOverLayer, gameButtons);
		CollectWidgets<TextWidget>(gameOverLayer, gameTexts);
		CollectWidgets<Panel>(gameOverLayer, gamePanels);
		SortButtonsByTop(gameButtons);
		report.Check(gameButtons.size() == 3 && gameTexts.size() == 2 && !gamePanels.empty(), "game over builds overlay, title, score, and three buttons");
		if (gameButtons.size() != 3 || gameTexts.size() != 2 || gamePanels.empty()) return;
		const UIRect gameOverlay = gamePanels.front()->GetResolvedRect();
		report.Check(NearlyEqual(gameOverlay.position.x, 50.f) && NearlyEqual(gameOverlay.position.y, 50.f) &&
			NearlyEqual(gameOverlay.size.x, 1820.f) && NearlyEqual(gameOverlay.size.y, 980.f), "game over overlay keeps 50px viewport insets");
		report.Check(ButtonCenterNear(*gameButtons[0], 960.f, 300.f) && ButtonCenterNear(*gameButtons[1], 960.f, 375.f) &&
			ButtonCenterNear(*gameButtons[2], 960.f, 450.f), "game over buttons keep Restart, Main Menu, Quit order and 75px pitch");
		TextWidget expectedTitle{ "You Win!" };
		expectedTitle.SetTextSize(40);
		TextWidget expectedScore{ "Score: 42" };
		expectedScore.SetTextSize(30);
		auto titleText = *std::min_element(gameTexts.begin(), gameTexts.end(), [](const auto& left, const auto& right)
		{
			return left->GetBound().position.y < right->GetBound().position.y;
		});
		auto scoreText = *std::max_element(gameTexts.begin(), gameTexts.end(), [](const auto& left, const auto& right)
		{
			return left->GetBound().position.y < right->GetBound().position.y;
		});
		report.Check(GameHUDDamageE2ETestAccess::Text(*titleText) == "You Win!" &&
			GameHUDDamageE2ETestAccess::Text(*scoreText) == "Score: 42" &&
			LegacyTextCenterNear(*titleText, "You Win!", 40, 150.f, 1920.f) &&
			LegacyTextCenterNear(*scoreText, "Score: 42", 30, 200.f, 1920.f) &&
			NearlyEqual(titleText->GetBound().size.x, expectedTitle.GetBound().size.x) &&
			NearlyEqual(scoreText->GetBound().size.x, expectedScore.GetBound().size.x),
			"pre-init game over title and score keep their requested strings and legacy rendered centers");
		bool gameClicksHandled = true;
		for (const auto& button : gameButtons) gameClicksHandled = ClickButton(*gameOver, *button) && gameClicksHandled;
		report.Check(gameClicksHandled && gameActions.gameRestart == 1 && gameActions.gameMainMenu == 1 && gameActions.gameQuit == 1,
			"each game over button broadcasts its existing public delegate exactly once");
		report.artifact["inputs"] = { { "viewport", { 1920, 1080 } }, { "responsiveViewport", { 1280, 720 } },
			{ "mainButtons", mainButtons.size() }, { "pauseButtons", pauseButtons.size() }, { "gameOverButtons", gameButtons.size() } };
		report.artifact["passed"] = true;
	}

	void WriteArtifact(const std::filesystem::path& path, const Json& artifact)
	{
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
		std::ofstream output{ path, std::ios::out | std::ios::trunc };
		if (!output) throw std::runtime_error{ "Could not open HUD migration artifact path" };
		output << artifact.dump(2) << '\n';
	}
}

int main(int argc, char** argv)
{
	if (argc != 3 || (std::string{ argv[1] } != "--warning" && std::string{ argv[1] } != "--notification" &&
		std::string{ argv[1] } != "--encounter" && std::string{ argv[1] } != "--ability" && std::string{ argv[1] } != "--menus"))
	{
		std::cerr << "Usage: LightYearsHUDMigrationTests --warning|--notification|--encounter|--ability|--menus <artifact-path>\n";
		return 2;
	}

	Report report;
	const std::string mode = argc == 3 ? argv[1] : "";
	report.artifact["test"] = mode == "--notification" ? "HUDMigrationNotification" :
		mode == "--encounter" ? "HUDMigrationEncounter" : mode == "--ability" ? "HUDMigrationAbility" :
		mode == "--menus" ? "HUDMigrationMenus" : "HUDMigrationWarning";
	try
	{
		ly::AssetManager::GetAssetManager().SetAssetRootDirectory(
			(std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets").generic_string() + "/"
		);
		if (mode == "--ability" && !ly::GameContentBootstrap::Register())
			throw std::runtime_error{ "GameContentBootstrap::Register failed" };
		if (std::string{ argv[1] } == "--warning") RunWarningTest(report);
		else if (std::string{ argv[1] } == "--notification") RunNotificationTest(report);
		else if (std::string{ argv[1] } == "--encounter") RunEncounterTest(report);
		else if (std::string{ argv[1] } == "--ability") RunAbilityTest(report);
		else RunMenusTest(report);
	}
	catch (const std::exception& exception)
	{
		report.artifact["error"] = exception.what();
		const std::string label = mode == "--notification" ? "notification" : mode == "--encounter" ? "encounter" :
			mode == "--ability" ? "ability" : mode == "--menus" ? "menus" : "warning";
		std::cerr << "HUD migration " << label << " test failed: " << exception.what() << '\n';
	}
	try
	{
		WriteArtifact(argv[2], report.artifact);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Could not write HUD migration artifact: " << exception.what() << '\n';
		return 1;
	}
	std::cout << "E2E artifact: " << std::filesystem::absolute(argv[2]).string() << '\n';
	return report.artifact.value("passed", false) ? 0 : 1;
}
