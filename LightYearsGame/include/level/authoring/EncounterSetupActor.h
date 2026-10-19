#pragma once

#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"
#include "gameplay/encounter/EncounterWaveRuntime.h"

namespace vega
{
	struct ArenaEncounterProgression
	{
		VEGA_STRUCT(ArenaEncounterProgression)
		int additionalEnemyEveryWaves = 1;
		int enemyLevelEveryWaves = 3;
		int maximumAdditionalEnemies = 0;
		int maximumEnemyLevel = 15;
		int seed = 1337;
	};
	struct ArenaWaveTemplate
	{
		VEGA_STRUCT(ArenaWaveTemplate)
		float interWaveDelay = 1.5f;
	};
	struct ArenaWaveEntry
	{
		VEGA_STRUCT(ArenaWaveEntry)
		int wave = 0;
		std::string enemyId = "Enemy.ApproachGunner.Basic";
		int count = 1;
		float spawnInterval = 0.f;
	};
	class EncounterSetupActor : public Actor
	{
		VEGA_CLASS(EncounterSetupActor)
	public:
		explicit EncounterSetupActor(World* world) : Actor{ world } {}
		bool enabled = false;
		EncounterSequenceMode sequenceMode = EncounterSequenceMode::EndlessCycle;
		ArenaEncounterProgression progression;
		int maximumConcurrentEnemies = 24;
		std::vector<sf::Vector2f> spawnPoints{ {900.f,600.f}, {5100.f,600.f}, {900.f,2400.f}, {5100.f,2400.f} };
		float spawnOffsetSpacing = 50.f;
		std::vector<ArenaWaveTemplate> waves{ {}, {}, {} };
		std::vector<ArenaWaveEntry> entries{
			{0,"Enemy.ApproachGunner.Basic",2,0.5f}, {1,"Enemy.ApproachGunner.Basic",1,0.4f}, {1,"Enemy.StrafeSkirmisher.Basic",1,0.f},
			{2,"Enemy.ApproachGunner.Basic",1,0.3f}, {2,"Enemy.StrafeSkirmisher.Basic",1,0.3f}, {2,"Enemy.RangeKeeper.Basic",1,0.3f}
		};
	};
}
