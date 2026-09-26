#pragma once

#include "framework/Actor.h"
#include "framework/Core.h"

#include <cstdint>

namespace ly
{
	struct PortalTransferRuntimeState final
	{
		struct Endpoint
		{
			weak_ptr<Actor> actor;
			sf::Vector2f location{};
		};

		struct Transit
		{
			weak_ptr<Actor> actor;
			bool enteredFirst = true;
			float remaining = 0.f;
		};

		struct ReentryLock
		{
			weak_ptr<Actor> actor;
			bool waitingForExit = true;
			float cooldownRemaining = 0.f;
		};

		struct Pair
		{
			std::uint64_t id = 0;
			Endpoint first;
			Endpoint second;
			float radius = 1.f;
			float transferDuration = 0.25f;
			float reentryCooldown = 1.f;
			bool acceptingEntries = true;
			List<Transit> transits;
			Dictionary<unsigned int, ReentryLock> reentryLocks;
		};

		std::uint64_t nextPairId = 1;
		List<Pair> pairs;
	};
}
