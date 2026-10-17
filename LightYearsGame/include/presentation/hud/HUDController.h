#pragma once

#include "gameplay/GameplayWarning.h"
#include <SFML/Window/Event.hpp>

namespace vega
{
	class HUDController
	{
	public:
		virtual ~HUDController() = default;
		virtual void Tick(float deltaTime) {}
		// Offered by the level before the HUD sees an event while the game is running; true consumes it.
		virtual bool HandleEvent(const sf::Event& event) { return false; }
		virtual void ShowGameplayWarning(const GameplayWarning& warning) {}
		virtual void HideGameplayWarning(GameplayWarningType warningType) {}
	};
}


