#pragma once

#include <string>
#include <SFML/System/Vector2.hpp>

namespace ly
{
	struct PlayerRespawnDefinition
	{
		sf::Vector2f spawnLocation{};
		float respawnDelay = 1.f;

		int playerIndex = 0;
		bool createPlayerIfMissing = true;

		bool useScreenClamp = true;
		bool respawnWhenDestroyed = true;
		bool gameOverWhenRespawnFails = true;

		std::string playerShipId = "Default";
	};
}


