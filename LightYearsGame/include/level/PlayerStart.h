#pragma once

#include <framework/Actor.h>
#include "gameConfigs/ship/ShipStructs.h"

namespace vega
{
	// An authored spawn point, not a player. The editor can draw a marker using
	// this query without creating a gameplay sprite or collision body.
	class PlayerStart : public Actor
	{
		VEGA_CLASS(PlayerStart)
	public:
		explicit PlayerStart(World* world) : Actor{ world } {}
		bool IsEditorOnlyMarker() const { return true; }
		const std::string& GetShip() const { return mShip; }
		void SetShip(const std::string& ship);
	private:
		std::string mShip = DefaultPlayerShipId;
	};
}
