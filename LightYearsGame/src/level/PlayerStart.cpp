#include "level/PlayerStart.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/reflection/Reflection.h>
#include <stdexcept>

namespace vega
{
	VEGA_REFLECT(PlayerStart, Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_PROPERTY("Ship", GetShip, SetShip).Category("Player Start");
	}

	void PlayerStart::SetShip(const std::string& ship)
	{
		if (ship.rfind("Ship.Player.", 0) != 0 || !content::ShipContentCatalog::FindById(ship))
			throw std::runtime_error("Unknown player ship: " + ship);
		mShip = ship;
	}
}
