#include "framework/Application.h"
#include "framework/Core.h"
#include "framework/World.h"
#include "framework/AssetManager.h"
#include "framework/PhysicsSystem.h"

namespace ly
{
	Application::Application(sf::Vector2u Position, unsigned int bit, std::string& Title, uint32_t Style)
		:mWindow{ sf::VideoMode(Position, bit), Title, Style },
		mTargetFrameRate{ 60.f },
		mTickClock{},
		currentWorld{ nullptr },
		mCleanCycleClock{},
		mCleanCycleTime{2.f}
	{

	}
	void Application::Run()
	{
		mTickClock.restart();
		float accumulatedTime = 0.f;
		float targetDeltaTime = 1.f / mTargetFrameRate;
		while (mWindow.isOpen())
		{
			while (const std::optional event = mWindow.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					mWindow.close();
				}

			}
			accumulatedTime += mTickClock.restart().asSeconds();
			while (accumulatedTime >= targetDeltaTime)
			{
				// Update game logic here
				accumulatedTime -= targetDeltaTime;
				TickInternal(targetDeltaTime);
				RenderInternal();
			}
		}
	}
	void Application::TickInternal(float deltaTime)
	{
		Tick(deltaTime);
		
		if(currentWorld)
		{
			currentWorld->TickInternal(deltaTime);
		}

		PhysicsSystem::Get().Step(deltaTime);

		if (mCleanCycleClock.getElapsedTime().asSeconds() > mCleanCycleTime)
		{
			mCleanCycleClock.restart();
			AssetManager::GetAssetManager().CleanCycle();
			currentWorld->CleanCycle();
		}
	}
	void Application::Tick(float deltaTime)
	{

	}
	void Application::RenderInternal()
	{
		mWindow.clear();

		Render();

		mWindow.display();
	}
	void Application::Render()
	{
		if(currentWorld)
		{
			currentWorld->Render(mWindow);
		}
		else
		{
			mWindow.clear(sf::Color::Black);
		}
	}
}
