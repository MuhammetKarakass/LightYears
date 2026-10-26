#pragma once

#include "framework/Core.h"
#include "framework/GameplayTag.h"

#include <SFML/System/Vector2.hpp>

#include <cstdint>

namespace vega
{
	class Actor;

	// A burst that splits into fragments. Each fragment hits one of the source's opposing combatants within radius of the
	// origin, nearest first, in rounds, so one target takes at most maxHitsPerTarget fragments; the center target (when
	// given and alive) takes the first fragment of every round. Fragments left over when every target is capped are lost.
	struct FragmentBurstRequest
	{
		sf::Vector2f origin{};
		float radius = 0.f;
		int fragmentCount = 0;
		float damagePerFragment = 0.f;
		int maxHitsPerTarget = 1;
		List<GameplayTag> damageTags;
	};

	// Generated damage: no critical hits, no module triggers and no new status stacks. Returns the hits per target id.
	List<std::pair<std::uint64_t, int>> ApplyFragmentBurst(Actor& source, Actor* center, const FragmentBurstRequest& request);
}
