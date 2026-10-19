#pragma once

#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"

namespace vega
{
	class GameStage;
	enum class CampaignStageKind { Wait, Vanguard, TwinBlade, Hexagon, Chaos };
	struct CampaignStageEntry
	{
		VEGA_STRUCT(CampaignStageEntry)
		CampaignStageKind kind = CampaignStageKind::Wait;
		float duration = 5.f;
		int configuration = -1;
	};
	struct VanguardStageSettings
	{
		VEGA_STRUCT(VanguardStageSettings)
		float minSpawnInterval = 1.25f;
		float maxSpawnInterval = 2.f;
		float spawnDistanceToEdge = 100.f;
		float switchInterval = 2.5f;
		int rows = 5;
		int perRow = 5;
	};
	struct TwinBladeStageSettings
	{
		VEGA_STRUCT(TwinBladeStageSettings)
		float minSpawnInterval = 0.75f;
		float maxSpawnInterval = 1.5f;
		int spawnCount = 30;
	};
	struct HexagonStageSettings
	{
		VEGA_STRUCT(HexagonStageSettings)
		float spawnInterval = 5.f;
		int spawnGroups = 7;
	};
	struct ChaosStageSettings
	{
		VEGA_STRUCT(ChaosStageSettings)
		float spawnInterval = 3.f;
		float spawnIntervalDecrement = 0.05f;
		float survivalDuration = 60.f;
		float spawnMinDistanceTop = 75.f;
	};
	struct CampaignPlan
	{
		std::vector<CampaignStageEntry> stages{
			{}, {CampaignStageKind::Vanguard,0.f,0}, {}, {CampaignStageKind::TwinBlade,0.f,0},
			{}, {CampaignStageKind::Hexagon,0.f,0}, {}, {CampaignStageKind::Chaos,0.f,0}
		};
		std::vector<VanguardStageSettings> vanguard{ {} };
		std::vector<TwinBladeStageSettings> twinBlade{ {} };
		std::vector<HexagonStageSettings> hexagon{ {} };
		std::vector<ChaosStageSettings> chaos{ {} };
		void Validate() const;
		shared_ptr<GameStage> CreateStage(World& world, const CampaignStageEntry& entry) const;
	};
	// Passive authoring actor: no BeginPlay, timers, stages or player state.
	class CampaignSetupActor : public Actor
	{
		VEGA_CLASS(CampaignSetupActor)
	public:
		explicit CampaignSetupActor(World* world) : Actor{ world } {}
		CampaignPlan plan;
	};
}
