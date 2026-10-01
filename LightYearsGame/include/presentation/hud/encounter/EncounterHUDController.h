#pragma once

#include "framework/Core.h"
#include "gameplay/encounter/EncounterWaveRuntime.h"
#include "presentation/hud/HUDController.h"
#include "presentation/hud/encounter/EncounterHUDPresentation.h"

#include <SFML/System/Vector2.hpp>
#include <functional>
#include <string>

namespace ly
{
	class GameHUD;
	class EncounterHUDView;

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
		shared_ptr<const EncounterHUDPresentation> GetPresentation() const { return mPresentation; }

	private:
		weak_ptr<GameHUD> mGameHUD;
		SnapshotProvider mSnapshotProvider;
		shared_ptr<EncounterHUDPresentation> mPresentation;
		weak_ptr<EncounterHUDView> mView;
	};
}
