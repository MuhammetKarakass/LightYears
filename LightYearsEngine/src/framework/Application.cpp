#include "framework/Application.h"
#include "framework/Core.h"
#include "framework/World.h"
#include "framework/AssetManager.h"
#include "framework/AudioManager.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"
#include "framework/ShaderManager.h"

namespace ly
{
	Application::Application(sf::Vector2u Position, unsigned int bit, std::string& Title, uint32_t Style)
		:mWindow{ sf::VideoMode(Position, bit), Title, Style },
		mTargetFrameRate{ 60.f },     
		mTickClock{},   
		mCurrentWorld{ nullptr },
		mCleanCycleClock{},    
		mCleanCycleTime{2.f}          
	{		
		mWindow.setFramerateLimit(60);
		mWindow.setVerticalSyncEnabled(false);
	}
	
	void Application::Run()
	{
		mTickClock.restart();
		
		while (mWindow.isOpen())  
		{
			sf::Time deltaTime = mTickClock.restart();
			float dt = deltaTime.asSeconds();
			
			while (const std::optional event = mWindow.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					QuitApplication();
				}
				else 
				{
					DispatchEvent(event);
				}
			}
			
			TickInternal(dt);
			RenderInternal();
		}
	}

	void Application::QuitApplication()
	{
		AudioManager::GetAudioManager().StopMusic();
		AudioManager::GetAudioManager().CleanCycle();
		mWindow.close();
	}
	
	void Application::TickInternal(float deltaTime)
	{
		Tick(deltaTime);
		
		if(mCurrentWorld)
		{
			mCurrentWorld->TickInternal(deltaTime);
		}

		TimerManager::GetGlobalTimerManager().UpdateTimer(deltaTime);

		bool isPaused = mCurrentWorld && mCurrentWorld->IsPaused();

		if (!isPaused)
		{
			TimerManager::GetGameTimerManager().UpdateTimer(deltaTime);
			PhysicsSystem::Get().Step(deltaTime);
		}

		AudioManager::GetAudioManager().Update(deltaTime);
		
		if (mCleanCycleClock.getElapsedTime().asSeconds() > mCleanCycleTime)
		{
			mCleanCycleClock.restart();
			AssetManager::GetAssetManager().CleanCycle();
			AudioManager::GetAudioManager().CleanCycle();
			if (mCurrentWorld)
				mCurrentWorld->CleanCycle();
		}

		if(mPendingWorld && mPendingWorld!=mCurrentWorld)
		{
			mCurrentWorld = nullptr;

			TimerManager::GetGameTimerManager().ClearAllTimers();
			TimerManager::GetGlobalTimerManager().ClearAllTimers();

			PhysicsSystem::Get().Cleanup();
			PhysicsSystem::Get().InitializeWorld({ 0.f,0.f });

			mCurrentWorld = mPendingWorld;
			mCurrentWorld->BeginPlayInternal();
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
		if(mCurrentWorld)
		{
			mCurrentWorld->Render(mWindow);
		}
		else
		{
			mWindow.clear(sf::Color::Black);
		}
	}
	
	bool Application::DispatchEvent(const std::optional<sf::Event>& event)
	{
		if (mCurrentWorld && event.has_value())
		{
			return mCurrentWorld->DispatchEvent(event.value());
		}
		return false;
	}
}
