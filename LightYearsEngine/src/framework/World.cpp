#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"

namespace ly{

	// World constructor - Oyun sahnesini olu�turur
	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPendingActors{},      
		mActors{}              
	{

	}

	// BeginPlay'i g�venli �ekilde bir kez �a��rmak i�in wrapper
	void World::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();      
		}
	}

	// Her frame world ve t�m actor'lar� g�ncelle
	void World::TickInternal(float deltaTime)
	{

        // Bekleyen actor'lar� aktif listeye ekle ve ba�lat
        for (std::shared_ptr<Actor> actor : mPendingActors) 
		{
              mActors.push_back(actor);           // Aktif listeye ekle
              actor->BeginPlayInternal();         // Actor'� ba�lat
        }

		if (mBeganPlay)
		{
			Tick(deltaTime);  // Virtual - t�retilen s�n�flar override eder
		}

		mPendingActors.clear();  // Bekleyen listeyi temizle

		// T�m aktif actor'lar� g�ncelle
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			iter->get()->TickInternal(deltaTime);  // Her actor'� g�ncelle
			iter++;
		}
	}

	// Silinmek �zere i�aretlenen actor'lar� temizle
	void World::CleanCycle()
	{
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			if (iter->get()->GetIsPendingDestroy())
			{
				iter = mActors.erase(iter);  // Listeden kald�r ve memory'i serbest b�rak
			}
			else
			{
				iter++;
			}
		}
	}

	void World::Render(sf::RenderWindow& window)
	{
		for (const std::shared_ptr<Actor>& actor : mActors)
		{
			actor->Render(window);
		}
	}

	sf::Vector2u World::GetWindowSize()
	{
		return mOwningApp->GetWindowSize();
	}


	World::~World()
	{
		// Shared_ptr'ler otomatik temizlenecek
	}

	// Oyun ba�lad���nda yap�lacak i�lemler (override edilebilir)
	void World::BeginPlay()
	{
		
	}

	// Her frame world'�n yapaca�� i�lemler (override edilebilir)
	void World::Tick(float deltaTime)
	{

	}
}