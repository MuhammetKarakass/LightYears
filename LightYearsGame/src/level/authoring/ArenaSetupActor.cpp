#include "level/authoring/ArenaSetupActor.h"

namespace vega
{
	VEGA_REFLECT_STRUCT(ArenaBoundarySettings)
	{
		VEGA_FIELD("enabled", enabled);
		VEGA_FIELD("visualType", visualType).Labels({ {0,"DebugRectangle"}, {1,"EnergyWall"}, {2,"WarningField"}, {3,"None"} });
		VEGA_FIELD("outOfBoundsMargin", outOfBoundsMargin);
		VEGA_FIELD("outOfBoundsTime", outOfBoundsTime);
		VEGA_FIELD("outlineColor", outlineColor);
		VEGA_FIELD("warningColor", warningColor);
		VEGA_FIELD("outlineThickness", outlineThickness);
	}
	VEGA_REFLECT_STRUCT(ArenaRespawnSettings)
	{
		VEGA_FIELD("respawnDelay", respawnDelay);
		VEGA_FIELD("playerIndex", playerIndex);
		VEGA_FIELD("createPlayerIfMissing", createPlayerIfMissing);
		VEGA_FIELD("useScreenClamp", useScreenClamp);
		VEGA_FIELD("respawnWhenDestroyed", respawnWhenDestroyed);
		VEGA_FIELD("gameOverWhenRespawnFails", gameOverWhenRespawnFails);
		VEGA_FIELD("playerShipId", playerShipId);
	}
	VEGA_REFLECT_STRUCT(ArenaMusicSettings)
	{
		VEGA_FIELD("path", path).Asset(reflect::AssetKind::Sound);
		VEGA_FIELD("volume", volume);
		VEGA_FIELD("pitch", pitch);
		VEGA_FIELD("loop", loop);
		VEGA_FIELD("fadeDuration", fadeDuration);
	}
	VEGA_REFLECT(ArenaSetupActor, Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_CATEGORY("Arena configuration");
		VEGA_FIELD("Boundary", boundary);
		VEGA_FIELD("Respawn", respawn);
		VEGA_FIELD("Music", music);
		VEGA_FIELD("DummyArmor", dummyArmor).Tooltip("Base Armor applied to scene-placed dummy prefab instances that do not author their own Armor before BeginPlay.");
	}
}
