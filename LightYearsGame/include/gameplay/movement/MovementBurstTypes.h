#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly::movement
{
	// Generic movement burst request. The movement layer does not know whether a
	// caller uses this for Dash or another authored burst-like movement action.
	struct MovementBurstRequest
	{
		sf::Vector2f direction{ 0.f, 0.f };
		float baseDistance = 0.f;
		float duration = 0.f;
	};
}
