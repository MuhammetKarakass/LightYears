#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"
#include "framework/PerfMonitor.h"

namespace ly{

	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPendingActors{},      
		mActors{},
		mCurrentStage{mGameStages.end()},
		mGameStages{},
		mIsPaused{ false }
	{

	}
	void World::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();  
			InitGameStages();
			BeginStages();
		}
	}
	void World::TickInternal(float deltaTime)
	{
		if (!mIsPaused) {

			for (std::shared_ptr<Actor> actor : mPendingActors)
			{
				mActors.push_back(actor);
				actor->BeginPlayInternal();

				OnActorSpawned(actor.get());
			}

			mPendingActors.clear();

			for (auto iter = mActors.begin(); iter != mActors.end();)
			{
				iter->get()->TickInternal(deltaTime);
				iter++;
			}

			if (mCurrentStage != mGameStages.end())
			{
				mCurrentStage->get()->TickStage(deltaTime);
			}

			if (mBeganPlay)
			{
				Tick(deltaTime);
			}
		}

		else
		{
			for (std::shared_ptr<Actor> actor : mPendingActors)
			{
				mActors.push_back(actor);
				actor->BeginPlayInternal();

				OnActorSpawned(actor.get());
			}

			mPendingActors.clear();

			for(auto& actor : mActors)
			{
				if (actor->GetTickWhenPaused())
				{
					actor->TickInternal(deltaTime);
				}
			}

			if (mCurrentStage != mGameStages.end())
			{
				mCurrentStage->get()->TickStage(deltaTime);
			}
		}

		if(mHUD)
		{
			if(!mHUD->HasInit())
				mHUD->NativeInit(mOwningApp->GetRenderWindow());
			mHUD->Tick(deltaTime);
		}
		if(mOverlayHUD)
		{
			if(!mOverlayHUD->HasInit())
				mOverlayHUD->NativeInit(mOwningApp->GetRenderWindow());
			mOverlayHUD->Tick(deltaTime);
		}

		// Ensure destroyed actors are removed promptly to avoid large accumulation
		CleanCycle();

		// Perf monitoring
		ly::perf::TickAndReport(deltaTime);
	}

	void World::CleanCycle()
	{
		for (auto iter = mActors.begin(); iter != mActors.end();)
		{
			if (iter->get()->GetIsPendingDestroy())
			{
				// Decrement global active actor count for monitoring
				ly::perf::DecActiveActors();
				iter = mActors.erase(iter);
			}
			else
			{
				iter++;
			}
		}
	}

	void World::SetPaused(bool paused)
	{
		mIsPaused = paused;
	}

	void World::RemoveOverlayHUD()
	{
		mOverlayHUD.reset();
	}

	weak_ptr<Actor> World::GetActorByLayer(CollisionLayer layer) const
	{
		for (auto& actor : mActors)
		{
			if (!actor->GetIsPendingDestroy() && actor->GetCollisionLayer() == layer)
			{
				return actor;
			}
		}
		return weak_ptr<Actor>();
	}

	void World::Render(sf::RenderWindow& window)
	{
		// Batch render all actors
		for (const std::shared_ptr<Actor>& actor : mActors)
		{
			// Skip destroyed actors
			if (!actor->GetIsPendingDestroy())
			{
				actor->Render(window);
			}
		}
		RenderHUD(window);
	}

	sf::Vector2u World::GetWindowSize()
	{
		return mOwningApp->GetWindowSize();
	}


	bool World::DispatchEvent(const sf::Event& event)
	{
		if (mOverlayHUD)
		{
			return mOverlayHUD->HandleEvent(event);
		}

		if (mHUD)
		{
			return mHUD->HandleEvent(event);
		}
		return false;
	}

	World::~World()
	{
		
	}


	void World::AddGameStage(shared_ptr<GameStage> newStage)
	{
		mGameStages.push_back(newStage);
	}


	void World::BeginPlay()
	{
		
	}

	void World::Tick(float deltaTime)
	{

	}

	void World::OnActorSpawned(Actor* actor)
	{
	}

	void World::InitGameStages()
	{
	}

	void World::AllGameStagesFinished()
	{
	}


	void World::NextGameStage()
	{
		mCurrentStage = mGameStages.erase(mCurrentStage);
		if(mCurrentStage!=mGameStages.end())
		{
			mCurrentStage->get()->BeginStage();
			mCurrentStage->get()->onStageFinished.BindAction(GetWeakPtr(), &World::NextGameStage);
		}
		else
		{
			AllGameStagesFinished();
		}
	}
	void World::BeginStages()
	{
		mCurrentStage = mGameStages.begin();
		if(mCurrentStage != mGameStages.end())
		{
			mCurrentStage->get()->BeginStage();
			mCurrentStage->get()->onStageFinished.BindAction(GetWeakPtr(), &World::NextGameStage);
		}
	}
	void World::RenderHUD(sf::RenderWindow& window)
	{
		if(mHUD)
		{
			if (mHUD->HasInit())
			{
				mHUD->Draw(window);
			}
		}
		if(mOverlayHUD)
		{
			if (mOverlayHUD->HasInit())
			{
				mOverlayHUD->Draw(window);
			}
		}
	}
}

