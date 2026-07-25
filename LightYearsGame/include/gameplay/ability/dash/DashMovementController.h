#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	struct DashRequest
	{
		sf::Vector2f direction{ 0.f, 0.f };
		float baseDistance = 0.f;
		float duration = 0.f;
	};

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
