#pragma once

#include "framework/Core.h"
#include "gameplay/encounter/EncounterWaveRuntime.h"
#include "presentation/hud/HUDController.h"

#include <SFML/System/Vector2.hpp>
#include <functional>
#include <string>

namespace ly
{
	class GameHUD;
	class TextWidget;

	// Presentation-only projection of an encounter snapshot. Kept next to its controller so the
	// encounter runtime stays unaware of HUD wording.
	struct EncounterHUDViewModel
	{
		bool visible = false;
		std::string title;
		std::string detail;
	};

	EncounterHUDViewModel BuildEncounterHUDViewModel(const EncounterWaveSnapshot& snapshot);

	// Renders the encounter snapshot supplied by its provider. Owns no encounter state.
	class EncounterHUDController final : public HUDController
	{
	public:
		using SnapshotProvider = std::function<EncounterWaveSnapshot()>;

		EncounterHUDController(weak_ptr<GameHUD> gameHUD, SnapshotProvider snapshotProvider);
		~EncounterHUDController() override;

		void Tick(float deltaTime) override;

	private:
		void EnsureWidgets();
		void UpdateWidgets(const EncounterWaveSnapshot& snapshot);
		void HideWidgets();
		void RemoveWidgets();
		void PositionWidgets();

		weak_ptr<GameHUD> mGameHUD;
		SnapshotProvider mSnapshotProvider;
		weak_ptr<TextWidget> mWaveText;
		weak_ptr<TextWidget> mDetailText;
		bool mWidgetsCreated = false;
		bool mWidgetsVisible = false;
		sf::Vector2u mLastWindowSize{ 0u, 0u };

		// Last presented values so unchanged snapshots do not rewrite widget text.
		EncounterWaveState mLastState = EncounterWaveState::Idle;
		size_t mLastWaveNumber = 0;
		EncounterSequenceMode mLastSequenceMode = EncounterSequenceMode::Finite;
		std::optional<size_t> mLastTotalWaveCount;
		int mLastThreatCount = -1;
		int mLastCountdownSecond = -1;
		int mLastEnemyLevel = -1;
	};
}
