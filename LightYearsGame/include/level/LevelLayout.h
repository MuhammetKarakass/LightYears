#pragma once

#include "level/authoring/ArenaSetupActor.h"
#include "level/authoring/EncounterSetupActor.h"
#include "framework/scene/Scene.h"

namespace vega
{
	// Resolved authoring data only. ArenaTestLevel and EncounterWaveRuntime own all rules.
	struct LevelLayout
	{
		ArenaDefinition arena;
		PlayerRespawnDefinition respawn;
		ArenaMusicSettings music;
		ArenaHeavyDummySettings heavyDummy;
		std::vector<sf::Vector2f> heavyDummyPositions;
		float dummyArmor = 40.f;
		bool encounterEnabled = false;
		EncounterSequenceMode sequenceMode = EncounterSequenceMode::EndlessCycle;
		EncounterProgression progression;
		List<EnemyWaveDefinition> waves;
		std::vector<sf::Vector2f> spawnPoints;
		float spawnOffsetSpacing = 50.f;
		std::string playerShip;
		nlohmann::json scene;
		bool fromScene = false;

		static LevelLayout Defaults();
		static void RegisterTypesAndContent();
		static LevelLayout Parse(const nlohmann::json& scene, EngineContext& context);
		static LevelLayout ReadArenaTest(EngineContext& context);
		SceneLoadResult LoadEntities(World& world) const;
		void ConfigureDummies(World& world) const;
	};
}
