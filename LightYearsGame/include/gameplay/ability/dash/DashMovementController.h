#pragma once

#include "gameplay/movement/MovementBurstTypes.h"

namespace ly
{
	// Compatibility alias for the Dash ability contract. The actual movement
	// request is owned by the generic movement layer.
	using DashRequest = movement::MovementBurstRequest;

	// Bridge implemented by actors that support the Dash ability family.
	class DashMovementController
	{
	public:
		virtual ~DashMovementController() = default;

		virtual sf::Vector2f ResolveDashDirection() const = 0;
		virtual bool StartDash(const DashRequest& request) = 0;
		virtual void EndDash() = 0;
	};
}
