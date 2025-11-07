#pragma once

#include <SFML/Graphics.hpp>
#include <framework/TimerManager.h>
#include <gameplay/GameStage.h>
#include <utility> // for std::pair

namespace ly
{
	class UFOStage : public GameStage
	{
	public:
		UFOStage(World* world);

		virtual void BeginStage() override;
		virtual void TickStage(float deltaTime) override;

	private:
		virtual void StageFinished() override;
		
		sf::Vector2f RandomSpawnLoc();
		void SpawnUFO();
		void SpawnUFOPair(); // Ýki UFO spawn eder (biri hemen, diðeri 1 saniye sonra)
		
		// Spawn location ve velocity'yi pair olarak döndürür
		std::pair<sf::Vector2f, sf::Vector2f> GetSpawnProperties();

		List<float> mSpawnInterval;
		int mSpawnAmt;
		int mCurrentSpawnCount;
		float mUFOSpeed;

		TimerHandle mSpawnTimerHandlePair;   // Çift spawn için timer
		TimerHandle mSpawnTimerHandleSecond; // Ýkinci UFO için timer
	};
}