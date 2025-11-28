#include "framework/Application.h"
#include "framework/Core.h"
#include "framework/World.h"
#include "framework/AssetManager.h"
#include "framework/PhysicsSystem.h"
#include "framework/TimerManager.h"

namespace ly
{
	Application::Application(sf::Vector2u Position, unsigned int bit, std::string& Title, uint32_t Style)
		:mWindow{ sf::VideoMode(Position, bit), Title, Style },  // SFML penceresi oluþtur
		mTargetFrameRate{ 75.f },     
		mTickClock{},                 
		mCurrentWorld{ nullptr },      
		mCleanCycleClock{},           
		mCleanCycleTime{2.f}          
	{

	}
	
	// Ana oyun döngüsü - program çalýþtýðý sürece devam eder
	void Application::Run()
	{
		mTickClock.restart();              
		float accumulatedTime = 0.f;       
		float targetDeltaTime = 1.f / mTargetFrameRate;  // Frame baþýna hedef süre (1/60 = 0.0167s)
		
		while (mWindow.isOpen())  
		{
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
			
			// Geçen zamaný biriktir
			accumulatedTime += mTickClock.restart().asSeconds();
			
			// Sabit frame rate için fixed timestep döngüsü
			while (accumulatedTime >= targetDeltaTime)
			{
				accumulatedTime -= targetDeltaTime;  // Kullanýlan zamaný çýkar
				TickInternal(targetDeltaTime);       // Oyun mantýðýný güncelle
				RenderInternal();                    // Ekraný yeniden çiz
			}
		}
	}

	void Application::QuitApplication()
	{
		mWindow.close();
	}
	
	void Application::TickInternal(float deltaTime)
	{
		Tick(deltaTime);  // Virtual - türetilen sýnýflar override eder
		
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

		// Belirli aralýklarla temizlik döngüsü çalýþtýr
		if (mCleanCycleClock.getElapsedTime().asSeconds() > mCleanCycleTime)
		{
			mCleanCycleClock.restart();
			AssetManager::GetAssetManager().CleanCycle();
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
