#pragma once

#include "framework/Core.h"

#include <SFML/System/Vector2.hpp>
#include <cstdint>

namespace ly
{
	class Actor;
	class World;

	class PortalTransferService final
	{
	public:
		using PairId = std::uint64_t;

		static PairId CreatePair(
			World& world,
			Actor& firstPortal,
			Actor& secondPortal,
			float portalRadius,
			float transferDuration,
			float reentryCooldown
		);
		static void ClosePair(World& world, PairId pairId);
		static void Tick(World& world, float deltaTime);
		static void EnsureRuntimeActor(World& world);
		static void ResetWorld(World& world);
	};
}
