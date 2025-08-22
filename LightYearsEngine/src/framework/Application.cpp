#include "framework/Application.h"
#include "framework/Core.h"
#include "framework/World.h"
#include "framework/AssetManager.h"
#include "framework/PhysicsSystem.h"

namespace ly
{
	Application::Application(sf::Vector2u Position, unsigned int bit, std::string& Title, uint32_t Style)
		:mWindow{ sf::VideoMode(Position, bit), Title, Style },  // SFML penceresi olu�tur
		mTargetFrameRate{ 60.f },     
		mTickClock{},                 
		currentWorld{ nullptr },      
		mCleanCycleClock{},           
		mCleanCycleTime{2.f}          
	{

	}
	
	// Ana oyun d�ng�s� - program �al��t��� s�rece devam eder
	void Application::Run()
	{
		mTickClock.restart();              
		float accumulatedTime = 0.f;       
		float targetDeltaTime = 1.f / mTargetFrameRate;  // Frame ba��na hedef s�re (1/60 = 0.0167s)
		
		while (mWindow.isOpen())  
		{
			while (const std::optional event = mWindow.pollEvent())
			{
				if (event->is<sf::Event::Closed>())
				{
					mWindow.close();
				}
			}
			
			// Ge�en zaman� biriktir
			accumulatedTime += mTickClock.restart().asSeconds();
			
			// Sabit frame rate i�in fixed timestep d�ng�s�
			while (accumulatedTime >= targetDeltaTime)
			{
				accumulatedTime -= targetDeltaTime;  // Kullan�lan zaman� ��kar
				TickInternal(targetDeltaTime);       // Oyun mant���n� g�ncelle
				RenderInternal();                    // Ekran� yeniden �iz
			}
		}
	}
	
	void Application::TickInternal(float deltaTime)
	{
		Tick(deltaTime);  // Virtual - t�retilen s�n�flar override eder
		
		if(currentWorld)
		{
			currentWorld->TickInternal(deltaTime);
		}

		// Fizik sim�lasyonunu ilerlet
		PhysicsSystem::Get().Step(deltaTime);

		// Belirli aral�klarla temizlik d�ng�s� �al��t�r
		if (mCleanCycleClock.getElapsedTime().asSeconds() > mCleanCycleTime)
		{
			mCleanCycleClock.restart();                    // Sayac� s�f�rla
			AssetManager::GetAssetManager().CleanCycle();  // Unused asset'leri temizle
			currentWorld->CleanCycle();                    // �l� actor'lar� temizle
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
