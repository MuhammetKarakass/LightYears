#pragma once

#include "gameplay/movement/MovementInfluenceTypes.h"

namespace ly
{
	class Actor;
}

namespace ly::movement
{
	// The only gameplay-facing entrance for external movement. It routes ship
	// targets into MovementComponent and preserves a velocity fallback for older
	// movable combatants that do not yet own that component.
	class MovementInfluenceService final
	{
	public:
		static bool ApplyImpulse(Actor& target, const ImpulseRequest& request);
		static bool ApplyInstantAcceleration(
			Actor& target,
			const sf::Vector2f& acceleration,
			float deltaTime
		);
		static bool SetAccelerationSource(
			Actor& target,
			const AccelerationSourceRequest& request
		);
		static bool SetForcedMovement(
			Actor& target,
			const ForcedMovementRequest& request
		);
		static void RemoveSource(Actor& target, MovementInfluenceSourceId sourceId);

	private:
		static bool CanReceiveExternalMovement(const Actor& target);
	};
}
