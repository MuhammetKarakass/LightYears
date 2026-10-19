#pragma once
#include <SFML/Graphics.hpp>
#include <framework/TimerManager.h>
#include <gameplay/GameStage.h>

namespace vega
{
	struct TwinBladeStageSettings;
	class TwinBladeStage : public GameStage
	{
	public:
		TwinBladeStage(World* world);
		TwinBladeStage(World* world, const TwinBladeStageSettings& settings);
		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

	private:

		List<float> mSpawnInterval;
		List<sf::Vector2f> mSpawnLocations;
		TimerHandle mSpawnTimerHandle;
		int mSpawnCount;
		int mCurrentSpawnCount;

		sf::Vector2f mLastSpawnLoc;

		void SpawnTwinBlade();
		virtual void StageFinished() override;
		void AddSpawnLocations();
	};
}

