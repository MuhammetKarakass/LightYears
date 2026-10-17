#pragma once

#include "gameConfigs/CollisionLayers.h"

#include "framework/Actor.h"

namespace ly::targeting
{
	// Combat target selection should derive allegiance from the source actor in
	// exactly one place. Ability-specific code may still add its own shape,
	// health, or gameplay-state filters on top of this policy.
	inline CollisionLayer ResolveOpposingLayer(const Actor& source)
	{
		switch (source.GetCollisionLayer())
		{
		case GameCollision::Player:
		case GameCollision::FriendlySummon:
			return GameCollision::Enemy;
		case GameCollision::Enemy:
			return GameCollision::Player | GameCollision::FriendlySummon;
		default:
			return CollisionLayer::None;
		}
	}

	inline bool IsOpposingTarget(const Actor& source, const Actor& candidate)
	{
		const CollisionLayer opposingLayer = ResolveOpposingLayer(source);
		return opposingLayer != CollisionLayer::None &&
			HasCollisionLayer(candidate.GetCollisionLayer(), opposingLayer);
	}
}
