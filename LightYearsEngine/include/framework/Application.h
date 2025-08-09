#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class World;
	class Application
	{
	public:

		Application(sf::Vector2u Position,unsigned int bit, std::string& Title,uint32_t Style);
		void Run();

		template<typename WorldType>
		weak_ptr<WorldType> LoadWorld();

	private:
		sf::RenderWindow mWindow;
		float mTargetFrameRate;
		sf::Clock mTickClock;

		sf::Clock mCleanCycleClock;
		float mCleanCycleTime;

		shared_ptr<World> currentWorld;

		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);

		void RenderInternal();
		virtual void Render(); 

	};

	template<typename WorldType>
	weak_ptr<WorldType> Application::LoadWorld()
	{

        auto newWorld = std::make_shared<WorldType>(this);
        currentWorld = newWorld;
        currentWorld->BeginPlayInternal();
        return newWorld;
           
	}
}