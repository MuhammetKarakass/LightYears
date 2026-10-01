#include "presentation/hud/encounter/EncounterHUDController.h"

#include "presentation/hud/encounter/EncounterHUDView.h"
#include "widget/GameHUD.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly
{
	namespace
	{
		// U+2022 bullet written as explicit UTF-8 bytes so the separator does not depend on source encoding.
		const char* const DetailSeparator = "  \xE2\x80\xA2  ";

		const sf::Color WaveTextColor{ 190, 230, 255, 255 };
		const sf::Color DetailTextColor{ 150, 190, 220, 255 };
		const sf::Color CompletedTextColor{ 140, 255, 190, 255 };

		// Presentation rounding only; the runtime inter-wave timer is never touched.
		int ResolveCountdownSeconds(float remainingTime)
		{
			return static_cast<int>(std::ceil(std::max(0.f, remainingTime)));
		}

		sf::Color ResolveWaveTextColor(EncounterWaveState state)
		{
			return state == EncounterWaveState::Completed ? CompletedTextColor : WaveTextColor;
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
			// A finite encounter prints its authored total; an endless one has none.
			viewModel.title = "WAVE " + std::to_string(snapshot.currentWaveNumber) +
				(snapshot.totalWaveCount.has_value()
					? " / " + std::to_string(*snapshot.totalWaveCount)
					: " | ENDLESS");
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
		mPresentation{ std::make_shared<EncounterHUDPresentation>() }
	{
	}

	EncounterHUDController::~EncounterHUDController()
	{
		if (const shared_ptr<EncounterHUDView> view = mView.lock()) view->DestroyWidget();
	}

	void EncounterHUDController::Tick(float deltaTime)
	{
		(void)deltaTime;

		const shared_ptr<GameHUD> hud = mGameHUD.lock();
		if (!hud || !hud->HasInit())
		{
			return;
		}

		const EncounterWaveSnapshot snapshot = mSnapshotProvider ? mSnapshotProvider() : EncounterWaveSnapshot{};
		const EncounterHUDViewModel projected = BuildEncounterHUDViewModel(snapshot);
		SetIfChanged(mPresentation->visible, projected.visible, mPresentation->revision);
		SetIfChanged(mPresentation->title, projected.title, mPresentation->revision);
		SetIfChanged(mPresentation->detail, projected.detail, mPresentation->revision);
		SetIfChanged(mPresentation->titleColor, ResolveWaveTextColor(snapshot.state), mPresentation->revision);
		if (mView.expired()) mView = hud->AddToLayer<EncounterHUDView>(UILayer::Hud, mPresentation);
	}
}
