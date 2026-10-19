#pragma once

#include "level/authoring/ArenaSetupActor.h"
#include "level/authoring/EncounterSetupActor.h"
#include "level/authoring/CampaignSetupActor.h"
#include "framework/scene/Scene.h"

namespace vega
{
	// Resolved authoring data only. ArenaTestLevel and EncounterWaveRuntime own all rules.
	struct LevelLayout
	{
		ArenaDefinition arena;
		PlayerRespawnDefinition respawn;
		ArenaMusicSettings music;
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
		CampaignPlan campaign;

		static LevelLayout Defaults();
		static void RegisterTypesAndContent();
		static LevelLayout Parse(const nlohmann::json& scene, EngineContext& context, const std::string& rules = "ArenaTest");
		static LevelLayout ReadArenaTest(EngineContext& context);
		static LevelLayout ReadLevelOne(EngineContext& context);
		SceneLoadResult LoadEntities(World& world) const;
		void ConfigureDummies(World& world) const;
	};
}
