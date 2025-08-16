#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
namespace ly{

	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },
		mPendingActors{},
		mActors{}
	{

	}

	void World::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();
		}
	}

	void World::TickInternal(float deltaTime)
	{

        for (std::shared_ptr<Actor> actor : mPendingActors) 
		{
              mActors.push_back(actor);
              actor->BeginPlayInternal();
        }

		if (mBeganPlay)
		{
			Tick(deltaTime);
		}

		mPendingActors.clear();
		for (auto iter= mActors.begin();iter!=mActors.end();)
		{
			if (iter->get()->GetIsPendingDestroy())
			{
				iter = mActors.erase(iter);
			}
			else
			{
				iter->get()->TickInternal(deltaTime);
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
	}

	void World::BeginPlay()
	{
		
	}

	void World::Tick(float deltaTime)
	{

	}
}