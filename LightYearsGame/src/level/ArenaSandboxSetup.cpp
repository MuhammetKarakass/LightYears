#include "level/ArenaSandboxSetup.h"

namespace vega
{
	ArenaDefinition CreateSandboxArenaDefinition()
	{
		ArenaDefinition arenaDefinition;
		arenaDefinition.size = sf::Vector2f{ 6000.f, 3000.f };
		arenaDefinition.legalBounds = sf::FloatRect{ sf::Vector2f{ 0.f, 0.f }, arenaDefinition.size };
		arenaDefinition.outOfBoundsMargin = 20.f;
		arenaDefinition.outOfBoundsTime = 5.f;
		arenaDefinition.boundaryVisual.enabled = true;
		arenaDefinition.boundaryVisual.visualType = ArenaBoundaryVisualType::DebugRectangle;
		arenaDefinition.boundaryVisual.outlineColor = sf::Color{ 90, 230, 255, 180 };
		arenaDefinition.boundaryVisual.warningColor = sf::Color{ 255, 80, 80, 220 };
		arenaDefinition.boundaryVisual.outlineThickness = 4.f;
		return arenaDefinition;
	}

	PlayerRespawnDefinition ConfigureSandboxPlayerRespawn(PlayerRespawnDefinition definition)
	{
		definition.respawnDelay = 1.f;
		definition.playerIndex = 0;
		definition.createPlayerIfMissing = true;
		definition.useScreenClamp = false;
		definition.respawnWhenDestroyed = true;
		definition.gameOverWhenRespawnFails = true;
		definition.playerShipId = "Default";
		return definition;
	}
}
