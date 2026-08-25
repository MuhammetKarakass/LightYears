#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	class Actor;

	// Gameplay entities opt into Void Gate through this small capability contract.
	// The portal service never needs to know whether the participant is a ship or
	// a concrete projectile family; each participant pauses and restores its own
	// runtime state.
	class PortalTransferParticipant
	{
	public:
		virtual ~PortalTransferParticipant() = default;

		virtual Actor& GetPortalTransferActor() = 0;
		virtual bool CanEnterPortalTransfer() const = 0;
		virtual float GetPortalTransferRadius() const = 0;
		virtual bool IsInPortalTransit() const = 0;
		virtual void BeginPortalTransit() = 0;
		virtual void CompletePortalTransit(const sf::Vector2f& exitLocation) = 0;
	};
}
