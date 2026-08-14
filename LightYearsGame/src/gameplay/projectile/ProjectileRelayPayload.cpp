#include "gameplay/projectile/ProjectileRelayPayload.h"

#include <algorithm>

namespace ly
{
	bool ProjectileRelayLineage::HasVisited(std::uint64_t relayId) const
	{
		return std::find(visitedRelayIds.begin(), visitedRelayIds.end(), relayId) !=
			visitedRelayIds.end();
	}

	ProjectileRelayLineage ProjectileRelayLineage::Appended(
		std::uint64_t relayId
	) const
	{
		ProjectileRelayLineage result = *this;
		if (!result.HasVisited(relayId))
		{
			result.visitedRelayIds.push_back(relayId);
		}
		++result.generation;
		return result;
	}
}

