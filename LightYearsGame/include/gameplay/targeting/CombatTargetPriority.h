#pragma once

#include "framework/Actor.h"
#include "gameplay/targeting/TargetingTypes.h"

namespace ly::targeting
{
	// Target relation answers who *can* be targeted. This policy only ranks valid
	// candidates: enemy-controlled mechanics protect the player by preferring it
	// over a friendly summon, then retain the normal nearest-target behavior.
	inline int GetCombatTargetPriority(
		CollisionLayer sourceLayer,
		const Actor& candidate
	)
	{
		if (sourceLayer == CollisionLayer::Enemy)
		{
			if (HasCollisionLayer(candidate.GetCollisionLayer(), CollisionLayer::Player))
			{
				return 0;
			}
			if (HasCollisionLayer(candidate.GetCollisionLayer(), CollisionLayer::FriendlySummon))
			{
				return 1;
			}
		}
		return 0;
	}

	inline CandidateComparator MakeCombatTargetPriorityComparator(
		CollisionLayer sourceLayer
	)
	{
		return [sourceLayer](
			const TargetingCandidate& left,
			const TargetingCandidate& right
		)
		{
			const int leftPriority = GetCombatTargetPriority(sourceLayer, *left.actor);
			const int rightPriority = GetCombatTargetPriority(sourceLayer, *right.actor);
			return leftPriority != rightPriority
				? leftPriority < rightPriority
				: left.distanceSquared < right.distanceSquared;
		};
	}
}
