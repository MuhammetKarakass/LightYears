#include "presentation/hud/encounter/EncounterHUDController.h"

#include "widget/GameHUD.h"
#include "widget/TextWidget.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly
{
	namespace
	{
		const char* const EncounterFontPath = "SpaceShooterRedux/Bonus/OrbitronBlack.ttf";
		const unsigned int WaveTextSize = 24u;
		const unsigned int DetailTextSize = 17u;
		const float WaveTextTopOffset = 38.f;
		const float DetailTextTopOffset = 72.f;

		// U+2022 bullet written as explicit UTF-8 bytes so the separator does not depend on source encoding.
		const char* const DetailSeparator = "  \xE2\x80\xA2  ";

		const sf::Color WaveTextColor{ 190, 230, 255, 255 };
		const sf::Color DetailTextColor{ 150, 190, 220, 255 };
		const sf::Color CompletedTextColor{ 140, 255, 190, 255 };

		const int NoCountdown = -1;

		// Presentation rounding only; the runtime inter-wave timer is never touched.
		int ResolveCountdownSeconds(float remainingTime)
		{
			return static_cast<int>(std::ceil(std::max(0.f, remainingTime)));
		}

		sf::Color ResolveWaveTextColor(EncounterWaveState state)
		{
			return state == EncounterWaveState::Completed ? CompletedTextColor : WaveTextColor;
		}

		void SetWidgetText(const weak_ptr<TextWidget>& widget, const std::string& text, const sf::Color& color)
		{
			const shared_ptr<TextWidget> textWidget = widget.lock();
			if (!textWidget)
			{
				return;
			}

			textWidget->SetString(text);
			textWidget->SetFillColor(color);
			// The origin is derived from the current bounds, so it must follow every string change.
			textWidget->CenterOrigin();
			textWidget->SetVisibility(!text.empty());
		}
	}

	EncounterHUDViewModel BuildEncounterHUDViewModel(const EncounterWaveSnapshot& snapshot)
	{
		EncounterHUDViewModel viewModel;
		switch (snapshot.state)
		{
		case EncounterWaveState::Spawning:
		case EncounterWaveState::WaitingForClear:
			viewModel.visible = true;
			viewModel.title = "WAVE " + std::to_string(snapshot.currentWaveNumber) + " / " +
				std::to_string(snapshot.totalWaveCount);
			viewModel.detail = "ENEMIES " + std::to_string(snapshot.aliveEnemyCount + snapshot.remainingSpawnCount) +
				DetailSeparator + "LEVEL " + std::to_string(snapshot.enemyLevel);
			return viewModel;
		case EncounterWaveState::InterWaveDelay:
			viewModel.visible = true;
			viewModel.title = "WAVE " + std::to_string(snapshot.currentWaveNumber) + " CLEARED";
			viewModel.detail = "NEXT WAVE IN " + std::to_string(ResolveCountdownSeconds(snapshot.interWaveRemainingTime));
			return viewModel;
		case EncounterWaveState::Completed:
			viewModel.visible = true;
			viewModel.title = "ENCOUNTER COMPLETE";
			return viewModel;
		default:
			// Idle and Failed both present nothing; a failed encounter is reported by the game-over flow.
			return viewModel;
		}
	}

	EncounterHUDController::EncounterHUDController(weak_ptr<GameHUD> gameHUD, SnapshotProvider snapshotProvider)
		: mGameHUD{ gameHUD },
		mSnapshotProvider{ std::move(snapshotProvider) },
		mWaveText{},
		mDetailText{},
		mWidgetsCreated{ false },
		mWidgetsVisible{ false },
		mLastWindowSize{ 0u, 0u }
	{
	}

	EncounterHUDController::~EncounterHUDController()
	{
		RemoveWidgets();
	}

	void EncounterHUDController::Tick(float deltaTime)
	{
		(void)deltaTime;

		const shared_ptr<GameHUD> hud = mGameHUD.lock();
		if (!hud || !hud->HasInit())
		{
			return;
		}

		EnsureWidgets();

		const sf::Vector2u windowSize = hud->GetWindowSize();
		if (windowSize != mLastWindowSize)
		{
			mLastWindowSize = windowSize;
			PositionWidgets();
		}

		UpdateWidgets(mSnapshotProvider ? mSnapshotProvider() : EncounterWaveSnapshot{});
	}

	void EncounterHUDController::EnsureWidgets()
	{
		if (mWidgetsCreated)
		{
			return;
		}

		const shared_ptr<GameHUD> hud = mGameHUD.lock();
		if (!hud)
		{
			return;
		}

		mWaveText = hud->AddWidget<TextWidget>("", EncounterFontPath);
		mDetailText = hud->AddWidget<TextWidget>("", EncounterFontPath);
		if (const shared_ptr<TextWidget> waveText = mWaveText.lock())
		{
			waveText->SetTextSize(WaveTextSize);
			waveText->SetVisibility(false);
		}
		if (const shared_ptr<TextWidget> detailText = mDetailText.lock())
		{
			detailText->SetTextSize(DetailTextSize);
			detailText->SetVisibility(false);
		}

		mWidgetsCreated = true;
		mWidgetsVisible = false;
		mLastWindowSize = hud->GetWindowSize();
		PositionWidgets();
	}

	void EncounterHUDController::UpdateWidgets(const EncounterWaveSnapshot& snapshot)
	{
		const EncounterHUDViewModel viewModel = BuildEncounterHUDViewModel(snapshot);
		if (!viewModel.visible)
		{
			HideWidgets();
			return;
		}

		const bool wasVisible = mWidgetsVisible;
		if (!wasVisible)
		{
			mWidgetsVisible = true;
			PositionWidgets();
		}

		const int threatCount = snapshot.aliveEnemyCount + snapshot.remainingSpawnCount;
		const int countdownSecond = snapshot.state == EncounterWaveState::InterWaveDelay
			? ResolveCountdownSeconds(snapshot.interWaveRemainingTime)
			: NoCountdown;

		// The state and the wave number shape both lines; the rest only shapes the detail line.
		const bool stateChanged = snapshot.state != mLastState || snapshot.currentWaveNumber != mLastWaveNumber;
		const bool detailChanged = threatCount != mLastThreatCount || countdownSecond != mLastCountdownSecond ||
			snapshot.enemyLevel != mLastEnemyLevel;

		mLastState = snapshot.state;
		mLastWaveNumber = snapshot.currentWaveNumber;
		mLastThreatCount = threatCount;
		mLastCountdownSecond = countdownSecond;
		mLastEnemyLevel = snapshot.enemyLevel;

		if (!wasVisible || stateChanged) SetWidgetText(mWaveText, viewModel.title, ResolveWaveTextColor(snapshot.state));
		if (!wasVisible || stateChanged || detailChanged) SetWidgetText(mDetailText, viewModel.detail, DetailTextColor);
	}

	void EncounterHUDController::HideWidgets()
	{
		if (!mWidgetsVisible)
		{
			return;
		}

		if (const shared_ptr<TextWidget> waveText = mWaveText.lock()) waveText->SetVisibility(false);
		if (const shared_ptr<TextWidget> detailText = mDetailText.lock()) detailText->SetVisibility(false);
		mWidgetsVisible = false;
	}

	void EncounterHUDController::RemoveWidgets()
	{
		if (const shared_ptr<GameHUD> hud = mGameHUD.lock())
		{
			hud->RemoveWidget(mWaveText);
			hud->RemoveWidget(mDetailText);
		}

		mWaveText.reset();
		mDetailText.reset();
		mWidgetsCreated = false;
		mWidgetsVisible = false;
		mLastWindowSize = sf::Vector2u{ 0u, 0u };
		mLastState = EncounterWaveState::Idle;
		mLastWaveNumber = 0;
		mLastThreatCount = -1;
		mLastCountdownSecond = NoCountdown;
		mLastEnemyLevel = -1;
	}

	void EncounterHUDController::PositionWidgets()
	{
		const float centerX = static_cast<float>(mLastWindowSize.x) * 0.5f;
		if (const shared_ptr<TextWidget> waveText = mWaveText.lock())
			waveText->SetWidgetLocation(sf::Vector2f{ centerX, WaveTextTopOffset });
		if (const shared_ptr<TextWidget> detailText = mDetailText.lock())
			detailText->SetWidgetLocation(sf::Vector2f{ centerX, DetailTextTopOffset });
	}
}
