#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include "framework/TimerManager.h"

namespace ly
{
	class World;
	class Application
	{
	public:

		Application(sf::Vector2u Position,unsigned int bit, std::string& Title,uint32_t Style);
		void Run();

		void QuitApplication();

		sf::Vector2u GetWindowSize() const
		{
			return mWindow.getSize();
		}

		sf::RenderWindow& GetRenderWindow()
		{
			return mWindow;
		}

		const sf::RenderWindow& GetRenderWindow() const
		{
			return mWindow;
		}

		template<typename WorldType>
		weak_ptr<WorldType> LoadWorld();

	private:
		sf::RenderWindow mWindow;
		float mTargetFrameRate;
		sf::Clock mTickClock;

		sf::Clock mCleanCycleClock;
		float mCleanCycleTime;

		shared_ptr<World> mCurrentWorld;
		shared_ptr<World> mPendingWorld;

		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);

		void RenderInternal();
		virtual void Render(); 

		bool DispatchEvent(const std::optional<sf::Event>& event);


	};

	template<typename WorldType>
	weak_ptr<WorldType> Application::LoadWorld()
	{

        auto newWorld = std::make_shared<WorldType>(this);
        mPendingWorld = newWorld;
        return newWorld;
           
	}
}