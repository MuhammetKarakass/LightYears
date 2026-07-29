#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"
#include "framework/PerfMonitor.h"
#include <algorithm>
#include <iterator>
#include <utility>

namespace ly{

	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPendingActors{},      
		mActors{},
		mRenderBuckets(static_cast<std::size_t>(RenderLayer::Count)),
		mCurrentStage{mGameStages.end()},
		mGameStages{},
		mIsPaused{ false },
		mCameraManager{}
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
		LY_PROFILE_FUNCTION();
		auto promotePendingActors = [this]()
		{
			if (mPendingActors.empty())
			{
				return;
			}

			const std::size_t firstSpawnedActorIndex = mActors.size();
			mActors.reserve(mActors.size() + mPendingActors.size());
			std::move(
				mPendingActors.begin(),
				mPendingActors.end(),
				std::back_inserter(mActors)
			);
			mPendingActors.clear();

			for (std::size_t index = firstSpawnedActorIndex; index < mActors.size(); ++index)
			{
				const shared_ptr<Actor>& actor = mActors[index];
				actor->BeginPlayInternal();
				OnActorSpawned(actor.get());
			}
		};

		if (!mIsPaused) {
			promotePendingActors();

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
			promotePendingActors();

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

		// Camera updates last so render uses the latest actor/stage/world state without a frame-order snap.
		UpdateCamera(deltaTime);
	}

	void World::CleanCycle()
	{
		LY_PROFILE_FUNCTION();
		const auto firstDestroyedActor = std::remove_if(
			mActors.begin(),
			mActors.end(),
			[](const shared_ptr<Actor>& actor)
			{
				if (!actor->GetIsPendingDestroy())
				{
					return false;
				}

				ly::perf::DecActiveActors();
				return true;
			}
		);
		mActors.erase(firstDestroyedActor, mActors.end());
	}

	void World::SetPaused(bool paused)
	{
		mIsPaused = paused;
	}

	void World::RemoveOverlayHUD()
	{
		mOverlayHUD.reset();
	}

	void World::SetViewTarget(weak_ptr<Actor> target)
	{
		mViewTarget = target;
		mCameraManager.SetFollowTarget(target);
	}

	void World::ClearViewTarget()
	{
		mViewTarget.reset();
		mCameraManager.ClearFollowTarget();
	}

	void World::SetCameraSettings(const CameraSettings& settings)
	{
		mCameraManager.SetSettings(settings);
	}

	void World::SetCameraExternalVelocity(const sf::Vector2f& velocity)
	{
		mCameraManager.SetExternalVelocity(velocity);
	}

	void World::ClearCameraExternalVelocity()
	{
		mCameraManager.ClearExternalVelocity();
	}

	void World::SetCameraPreserveFollowTargetOffset(bool preserve)
	{
		mCameraManager.SetPreserveFollowTargetOffset(preserve);
	}

	void World::SetCameraAdditionalZoomOut(float zoomOut)
	{
		mCameraManager.SetAdditionalZoomOut(zoomOut);
	}

	void World::SetCameraRelativeAdditionalZoomOut(float zoomOutRatio)
	{
		mCameraManager.SetRelativeAdditionalZoomOut(zoomOutRatio);
	}

	void World::SetCameraLookAheadWorldPosition(const std::optional<sf::Vector2f>& worldPosition)
	{
		mCameraManager.SetLookAheadWorldPosition(worldPosition);
	}

	void World::ClearCameraLookAheadWorldPosition()
	{
		mCameraManager.ClearLookAheadWorldPosition();
	}

	void World::SetCameraWorldBounds(const sf::FloatRect& bounds)
	{
		mCameraManager.SetWorldBounds(bounds);
	}

	void World::ClearCameraWorldBounds()
	{
		mCameraManager.ClearWorldBounds();
	}

	void World::PlayCameraShake(float amplitude, float duration, float frequency)
	{
		mCameraManager.PlayShake(amplitude, duration, frequency);
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
		LY_PROFILE_FUNCTION();
		sf::View previousView = window.getView();
		sf::View worldView = GetWorldView();
		window.setView(worldView);

		for (List<Actor*>& bucket : mRenderBuckets)
		{
			bucket.clear();
		}

		for (const shared_ptr<Actor>& actor : mActors)
		{
			if (actor->GetIsPendingDestroy())
			{
				continue;
			}

			const std::size_t layerIndex = static_cast<std::size_t>(actor->GetRenderLayer());
			if (layerIndex < mRenderBuckets.size())
			{
				mRenderBuckets[layerIndex].push_back(actor.get());
			}
		}

		for (const List<Actor*>& bucket : mRenderBuckets)
		{
			for (Actor* actor : bucket)
			{
				actor->Render(window);
			}
		}

		window.setView(window.getDefaultView());
		RenderHUD(window);

		window.setView(previousView);
	}

	sf::Vector2u World::GetWindowSize()
	{
		return mOwningApp->GetWindowSize();
	}

	sf::View World::GetWorldView() const
	{
		return mCameraManager.GetView(mOwningApp->GetRenderWindow().getDefaultView());
	}

	sf::Vector2f World::GetMouseWorldPosition() const
	{
		const sf::RenderWindow& window = mOwningApp->GetRenderWindow();
		return window.mapPixelToCoords(sf::Mouse::getPosition(window), GetWorldView());
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
	void World::UpdateCamera(float deltaTime)
	{
		if (mOwningApp)
		{
			mCameraManager.Update(deltaTime, mOwningApp->GetRenderWindow().getDefaultView());
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

