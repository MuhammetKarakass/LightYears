#include "level/PlayerStart.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <stdexcept>

namespace ly
{
	void PlayerStart::SetShip(const std::string& ship)
	{
		if (ship.rfind("Ship.Player.", 0) != 0 || !content::ShipContentCatalog::FindById(ship))
			throw std::runtime_error("Unknown player ship: " + ship);
		mShip = ship;
	}
}
