#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"

namespace ly{

	// World constructor - Oyun sahnesini oluþturur
	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPendingActors{},      
		mActors{}              
	{

	}

	// BeginPlay'i güvenli þekilde bir kez çaðýrmak için wrapper
	void World::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();      
		}
	}

	// Her frame world ve tüm actor'larý güncelle
	void World::TickInternal(float deltaTime)
	{

        // Bekleyen actor'larý aktif listeye ekle ve baþlat
        for (std::shared_ptr<Actor> actor : mPendingActors) 
		{
              mActors.push_back(actor);           // Aktif listeye ekle
              actor->BeginPlayInternal();         // Actor'ý baþlat
        }

		if (mBeganPlay)
		{
			Tick(deltaTime);  // Virtual - türetilen sýnýflar override eder
		}

		mPendingActors.clear();  // Bekleyen listeyi temizle

		// Tüm aktif actor'larý güncelle
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			iter->get()->TickInternal(deltaTime);  // Her actor'ý güncelle
			iter++;
		}
	}

	// Silinmek üzere iþaretlenen actor'larý temizle
	void World::CleanCycle()
	{
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			if (iter->get()->GetIsPendingDestroy())
			{
				iter = mActors.erase(iter);  // Listeden kaldýr ve memory'i serbest býrak
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

	// Oyun baþladýðýnda yapýlacak iþlemler (override edilebilir)
	void World::BeginPlay()
	{
		
	}

	// Her frame world'ün yapacaðý iþlemler (override edilebilir)
	void World::Tick(float deltaTime)
	{

	}
}