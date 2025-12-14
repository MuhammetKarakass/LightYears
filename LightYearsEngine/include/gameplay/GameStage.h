#pragma once

#include "framework/Object.h"
#include "framework/Core.h"
#include "framework/Delegate.h"
#include <string>
#include <SFML/Graphics.hpp>

namespace ly
{
	class World;
	class GameStage : public Object
	{
		
	public:
		GameStage(World* world);

		Delegate<> onStageFinished;
		Delegate<> onStageStarted;
		//Text, fadeIn, hold, fadeOut, location, size, color
		Delegate<const std::string&, float, float, float, const sf::Vector2f&, float, sf::Color> onNotification;

		const World* GetWorld() const { return mWorld; }
		World* GetWorld() { return mWorld; }

		virtual void BeginStage();
		virtual void TickStage(float deltaTime);

		bool IsStageFinished() const { return mStageFinished; }
		void FinishStage();


	private:
		World* mWorld;
		bool mStageFinished;

		virtual void StageFinished();
	};
}
