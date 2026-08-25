#pragma once

#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace ly
{
	using ForcedMovementSourceId = std::uint64_t;

	// A source-owned movement request. The movement component consumes the
	// resolved velocity; the ability never teleports the target or edits its
	// position directly. A stable source ID keeps independent control sources
	// from removing one another's movement state.
	struct ForcedMovementRequest
	{
		ForcedMovementSourceId sourceId = 0;
		sf::Vector2f velocity{ 0.f, 0.f };
	};
}
