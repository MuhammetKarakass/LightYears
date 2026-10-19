#include "level/authoring/EncounterSetupActor.h"

namespace vega
{
	VEGA_REFLECT_STRUCT(ArenaEncounterProgression)
	{
		VEGA_FIELD("additionalEnemyEveryWaves", additionalEnemyEveryWaves);
		VEGA_FIELD("enemyLevelEveryWaves", enemyLevelEveryWaves);
		VEGA_FIELD("maximumAdditionalEnemies", maximumAdditionalEnemies);
		VEGA_FIELD("maximumEnemyLevel", maximumEnemyLevel);
		VEGA_FIELD("seed", seed);
	}
	VEGA_REFLECT_STRUCT(ArenaWaveTemplate)
	{
		VEGA_FIELD("interWaveDelay", interWaveDelay);
	}
	VEGA_REFLECT_STRUCT(ArenaWaveEntry)
	{
		VEGA_FIELD("wave", wave).Tooltip("Zero-based index into Waves. Entry order within each wave is significant.");
		VEGA_FIELD("enemyId", enemyId);
		VEGA_FIELD("count", count);
		VEGA_FIELD("spawnInterval", spawnInterval);
	}
	VEGA_REFLECT(EncounterSetupActor, Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_CATEGORY("Encounter configuration");
		VEGA_FIELD("Enabled", enabled);
		VEGA_FIELD("SequenceMode", sequenceMode).Labels({ {static_cast<int>(EncounterSequenceMode::EndlessCycle),"EndlessCycle"} });
		VEGA_FIELD("Progression", progression);
		VEGA_FIELD("MaximumConcurrentEnemies", maximumConcurrentEnemies);
		VEGA_FIELD("SpawnPoints", spawnPoints);
		VEGA_FIELD("SpawnOffsetSpacing", spawnOffsetSpacing);
		VEGA_FIELD("Waves", waves);
		VEGA_FIELD("Entries", entries);
	}
}
