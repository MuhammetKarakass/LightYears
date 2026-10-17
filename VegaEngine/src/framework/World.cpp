#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "framework/WorldRenderPipeline.h"
#include "framework/RenderViewport.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"
#include "framework/PerfMonitor.h"
#include "framework/PhysicsScene.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <typeinfo>
#include <cstdint>
#include <iterator>
#include <unordered_set>
#include <utility>
#include <vector>

#include "WorldInternal.h"

namespace vega{
	using world_internal::SlowActorTrace;

	World::World(Application* owningApp, EngineContext* context):
		mContext{ owningApp ? &owningApp->GetEngineContext() : context ? context : &EngineContext::Current() },
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
		mPhysicsScene{ std::make_unique<PhysicsScene>() },
		mPendingActors{},      
		mActors{},
		mManualSpatialCells{},
		mManualSpatialActorCells{},
		mSpatiallyRegisteredActors{},
		mRenderBuckets(static_cast<std::size_t>(RenderLayer::Count)),
		mCurrentStage{mGameStages.end()},
		mGameStages{},
		mIsPaused{ false },
		mCameraManager{}
	{

	}

	void World::SetSimulationTimeModifier(
		SimulationTimeDomain domain,
		SimulationTimeModifierSourceId sourceId,
		float multiplier
	)
	{
		if (sourceId == 0 || !std::isfinite(multiplier) || multiplier < 0.f)
		{
			return;
		}

		mSimulationTimeModifiers[domain][sourceId] = multiplier;
	}

	void World::RemoveSimulationTimeModifier(
		SimulationTimeDomain domain,
		SimulationTimeModifierSourceId sourceId
	)
	{
		const auto domainIt = mSimulationTimeModifiers.find(domain);
		if (domainIt == mSimulationTimeModifiers.end())
		{
			return;
		}

		domainIt->second.erase(sourceId);
		if (domainIt->second.empty())
		{
			mSimulationTimeModifiers.erase(domainIt);
		}
	}

	float World::GetSimulationTimeScale(SimulationTimeDomain domain) const
	{
		const auto domainIt = mSimulationTimeModifiers.find(domain);
		if (domainIt == mSimulationTimeModifiers.end())
		{
			return 1.f;
		}

		float scale = 1.f;
		for (const auto& [sourceId, multiplier] : domainIt->second)
		{
			(void)sourceId;
			if (std::isfinite(multiplier))
			{
				scale = std::min(scale, std::max(0.f, multiplier));
			}
		}
		return scale;
	}

	void World::BeginPlayInternal()
	{
		if (!mBeganPlay)
		{
			mBeganPlay = true;
			BeginPlay();  
			if (!IsEditMode())
			{
				InitGameStages();
				BeginStages();
			}
		}
	}
	void World::TickInternal(float deltaTime)
	{
		VEGA_PROFILE_SCOPE("World Tick");
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
				const SlowActorTrace trace{ *actor };
				if (!IsEditMode()) actor->BeginPlayInternal();
				RegisterActorSpatialQuery(*actor);
				if (!IsEditMode()) OnActorSpawned(actor.get());
			}
		};

		if (IsEditMode())
		{
			mIsPaused = true;
			promotePendingActors();
			CleanCycle();
			return;
		}

		if (!mIsPaused) {
			{
				VEGA_PROFILE_SCOPE("World.Promote");
			promotePendingActors();
			}

			{
				VEGA_PROFILE_SCOPE("World.ActorsTick");
			for (auto iter = mActors.begin(); iter != mActors.end();)
			{
				Actor& actor = *iter->get();
				const SlowActorTrace trace{ actor };
				actor.TickInternal(
					deltaTime * GetSimulationTimeScale(
						actor.GetSimulationTimeDomain()
					)
				);
				iter++;
			}
			}

			if (mCurrentStage != mGameStages.end())
			{
				VEGA_PROFILE_SCOPE("World.StageTick");
				mCurrentStage->get()->TickStage(deltaTime);
			}

			if (mBeganPlay)
			{
				VEGA_PROFILE_SCOPE("World.LevelTick");
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
					actor->TickInternal(
						deltaTime * GetSimulationTimeScale(
							actor->GetSimulationTimeDomain()
						)
					);
				}
			}

			if (mCurrentStage != mGameStages.end())
			{
				mCurrentStage->get()->TickStage(deltaTime);
			}
		}

		if(mHUD)
		{
			VEGA_PROFILE_SCOPE("World.HUDTick");
			if (RenderViewport* viewport = GetRenderViewport(); viewport && !mHUD->HasInit())
			{
				mHUD->NativeInit(viewport->GetRenderTarget());
			}
			mHUD->Tick(deltaTime);
		}
		if(mOverlayHUD)
		{
			if (RenderViewport* viewport = GetRenderViewport(); viewport && !mOverlayHUD->HasInit())
			{
				mOverlayHUD->NativeInit(viewport->GetRenderTarget());
			}
			mOverlayHUD->Tick(deltaTime);
		}

		// Ensure destroyed actors are removed promptly to avoid large accumulation
		CleanCycle();

		// Perf monitoring
		vega::perf::TickAndReport(deltaTime);

		// Camera updates last so render uses the latest actor/stage/world state without a frame-order snap.
		UpdateCamera(deltaTime);
	}

	void World::CleanCycle()
	{
		VEGA_PROFILE_FUNCTION();
		const auto firstDestroyedActor = std::remove_if(
			mActors.begin(),
			mActors.end(),
			[this](const shared_ptr<Actor>& actor)
			{
				if (!actor->GetIsPendingDestroy())
				{
					return false;
				}

				RemoveActorSpatialQuery(*actor);
				vega::perf::DecActiveActors();
				return true;
			}
		);
		mActors.erase(firstDestroyedActor, mActors.end());
	}

	void World::SetPaused(bool paused)
	{
		mIsPaused = IsEditMode() || paused;
	}

	void World::RemoveOverlayHUD()
	{
		mOverlayHUD.reset();
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
		for (const auto& actor : mActors) if (actor) actor->Detach();
		for (const auto& actor : mPendingActors) if (actor) actor->Detach();
		// Release actors while the physics scene is still alive. Actors that
		// outlive the world through external shared_ptrs must not touch the dead
		// scene later, so detach their bodies first.
		for (const shared_ptr<Actor>& actor : mActors)
		{
			if (actor) actor->UnInitializePhysics();
		}
		for (const shared_ptr<Actor>& actor : mPendingActors)
		{
			if (actor) actor->UnInitializePhysics();
		}
		mActors.clear();
		mPendingActors.clear();
		mPhysicsScene.reset();
	}

	void World::StepPhysics(float deltaTime)
	{
		VEGA_PROFILE_SCOPE("Physics.Step");
		mPhysicsScene->Step(deltaTime);
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
		// Keep the finishing stage alive: this runs inside its onStageFinished broadcast.
		const shared_ptr<GameStage> finishedStage = *mCurrentStage;
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
}
