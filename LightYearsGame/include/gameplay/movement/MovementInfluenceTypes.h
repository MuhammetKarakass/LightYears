#pragma once

#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace ly::movement
{
	// A stable source identity lets a persistent field remove only the movement
	// it owns from a target. Source IDs are local to each target controller.
	using MovementInfluenceSourceId = std::uint64_t;

	// A one-shot velocity-like displacement. It is kept separate from voluntary
	// ship velocity so knockback is not accidentally limited by thrust speed.
	struct ImpulseRequest
	{
		sf::Vector2f velocity{ 0.f, 0.f };
		float retentionPerSecond = 0.01f;
	};

	// A persistent world-space acceleration. The source updates this while it is
	// active and removes it when its field, effect, or traversal ends.
	struct AccelerationSourceRequest
	{
		MovementInfluenceSourceId sourceId = 0;
		sf::Vector2f acceleration{ 0.f, 0.f };
	};

	// Forced movement owns translation for its lifetime. It intentionally does
	// not share the additive rules used by impulses and accelerations.
	struct ForcedMovementRequest
	{
		MovementInfluenceSourceId sourceId = 0;
		sf::Vector2f velocity{ 0.f, 0.f };
		int priority = 0;
	};
}
