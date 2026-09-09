#include "framework/World.h"
#include "framework/Actor.h"
#include "framework/Application.h"
#include "gameplay/GameStage.h"
#include "widget/HUD.h"
#include "framework/PerfMonitor.h"
#include "framework/PhysicsSystem.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace ly{
	namespace
	{
		constexpr float ManualSpatialCellSize = 256.f;

		bool Intersects(const sf::FloatRect& first, const sf::FloatRect& second)
		{
			return first.position.x + first.size.x >= second.position.x &&
				first.position.x <= second.position.x + second.size.x &&
				first.position.y + first.size.y >= second.position.y &&
				first.position.y <= second.position.y + second.size.y;
		}
	}

	World::World(Application* owningApp):
		mOwningApp{ owningApp },
		mBeganPlay{ false },   
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
				RegisterActorSpatialQuery(*actor);
				OnActorSpawned(actor.get());
			}
		};

		if (!mIsPaused) {
			promotePendingActors();

			for (auto iter = mActors.begin(); iter != mActors.end();)
			{
				Actor& actor = *iter->get();
				actor.TickInternal(
					deltaTime * GetSimulationTimeScale(
						actor.GetSimulationTimeDomain()
					)
				);
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
			[this](const shared_ptr<Actor>& actor)
			{
				if (!actor->GetIsPendingDestroy())
				{
					return false;
				}

				RemoveActorSpatialQuery(*actor);
				ly::perf::DecActiveActors();
				return true;
			}
		);
		mActors.erase(firstDestroyedActor, mActors.end());
	}

	bool World::ShouldUseManualSpatialQuery(const Actor& actor)
	{
		// Actors with a Box2D shape are already returned by the physics
		// broadphase. Keep every body-less actor here to preserve the legacy
		// query contract: tests and gameplay coordinators may intentionally have
		// no collision layer yet still be discoverable by a spatial query.
		return !actor.HasPhysicsBody() &&
			!actor.GetIsPendingDestroy();
	}

	sf::FloatRect World::GetManualSpatialBounds(const Actor& actor)
	{
		const float radius = std::max(0.f, actor.GetPhysicsCollisionRadius());
		if (radius > 0.f)
		{
			const sf::Vector2f location = actor.GetActorLocation();
			return {
				{ location.x - radius, location.y - radius },
				{ radius * 2.f, radius * 2.f }
			};
		}

		const sf::FloatRect bounds = actor.GetActorGlobalBounds();
		if (bounds.size.x > 0.f || bounds.size.y > 0.f)
		{
			return bounds;
		}

		return { actor.GetActorLocation(), { 0.f, 0.f } };
	}

	World::ManualSpatialCellRange World::GetManualSpatialCellRange(
		const sf::FloatRect& bounds
	)
	{
		const float right = bounds.position.x + std::max(0.f, bounds.size.x);
		const float bottom = bounds.position.y + std::max(0.f, bounds.size.y);
		return {
			static_cast<int>(std::floor(bounds.position.x / ManualSpatialCellSize)),
			static_cast<int>(std::floor(right / ManualSpatialCellSize)),
			static_cast<int>(std::floor(bounds.position.y / ManualSpatialCellSize)),
			static_cast<int>(std::floor(bottom / ManualSpatialCellSize))
		};
	}

	void World::RegisterActorSpatialQuery(Actor& actor)
	{
		mSpatiallyRegisteredActors.insert(&actor);
		RefreshActorSpatialQuery(actor);
	}

	void World::RemoveActorSpatialQuery(Actor& actor)
	{
		const auto rangeIt = mManualSpatialActorCells.find(&actor);
		if (rangeIt != mManualSpatialActorCells.end())
		{
			const ManualSpatialCellRange range = rangeIt->second;
			for (int y = range.minY; y <= range.maxY; ++y)
			{
				for (int x = range.minX; x <= range.maxX; ++x)
				{
					const ManualSpatialCell cell{ x, y };
					auto cellIt = mManualSpatialCells.find(cell);
					if (cellIt == mManualSpatialCells.end())
					{
						continue;
					}

					List<Actor*>& actors = cellIt->second;
					actors.erase(
						std::remove(actors.begin(), actors.end(), &actor),
						actors.end()
					);
					if (actors.empty())
					{
						mManualSpatialCells.erase(cellIt);
					}
				}
			}
			mManualSpatialActorCells.erase(rangeIt);
		}

		mSpatiallyRegisteredActors.erase(&actor);
	}

	void World::RefreshActorSpatialQuery(Actor& actor)
	{
		if (mSpatiallyRegisteredActors.find(&actor) ==
			mSpatiallyRegisteredActors.end())
		{
			return;
		}

		const auto existingRange = mManualSpatialActorCells.find(&actor);
		if (existingRange != mManualSpatialActorCells.end())
		{
			const ManualSpatialCellRange range = existingRange->second;
			for (int y = range.minY; y <= range.maxY; ++y)
			{
				for (int x = range.minX; x <= range.maxX; ++x)
				{
					const ManualSpatialCell cell{ x, y };
					auto cellIt = mManualSpatialCells.find(cell);
					if (cellIt == mManualSpatialCells.end())
					{
						continue;
					}
					List<Actor*>& actors = cellIt->second;
					actors.erase(
						std::remove(actors.begin(), actors.end(), &actor),
						actors.end()
					);
					if (actors.empty())
					{
						mManualSpatialCells.erase(cellIt);
					}
				}
			}
			mManualSpatialActorCells.erase(existingRange);
		}

		if (!ShouldUseManualSpatialQuery(actor))
		{
			return;
		}

		const ManualSpatialCellRange range =
			GetManualSpatialCellRange(GetManualSpatialBounds(actor));
		for (int y = range.minY; y <= range.maxY; ++y)
		{
			for (int x = range.minX; x <= range.maxX; ++x)
			{
				mManualSpatialCells[ManualSpatialCell{ x, y }].push_back(&actor);
			}
		}
		mManualSpatialActorCells.emplace(&actor, range);
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
		const sf::Vector2f viewSize = worldView.getSize();
		const sf::Vector2f viewCenter = worldView.getCenter();
		const sf::FloatRect viewBounds{
			{ viewCenter.x - viewSize.x * 0.5f, viewCenter.y - viewSize.y * 0.5f },
			viewSize
		};
		int renderCandidates = 0;
		int renderCulled = 0;
		int renderSubmitted = 0;

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
			++renderCandidates;
			const bool mayCull = actor->GetRenderLayer() != RenderLayer::Background &&
				actor->GetRenderLayer() != RenderLayer::Foreground;
			const std::optional<sf::FloatRect> renderBounds =
				mayCull ? actor->GetRenderBounds() : std::nullopt;
			if (renderBounds && !Intersects(*renderBounds, viewBounds))
			{
				++renderCulled;
				continue;
			}

			const std::size_t layerIndex = static_cast<std::size_t>(actor->GetRenderLayer());
			if (layerIndex < mRenderBuckets.size())
			{
				mRenderBuckets[layerIndex].push_back(actor.get());
				++renderSubmitted;
			}
		}
		ly::perf::SetRenderStats(renderCandidates, renderCulled, renderSubmitted);

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

	List<weak_ptr<Actor>> World::GetActorsInBounds(
		const sf::FloatRect& bounds
	) const
	{
		List<weak_ptr<Actor>> result;
		ForEachActorInBounds(bounds, [&result](Actor& actor)
		{
			const shared_ptr<Object> object = actor.GetWeakPtr().lock();
			if (object)
			{
				result.push_back(std::static_pointer_cast<Actor>(object));
			}
		});
		return result;
	}

	void World::VisitActorsInBounds(
		const sf::FloatRect& bounds,
		void* context,
		ActorBoundsVisitor visitor
	) const
	{
		if (!visitor)
		{
			return;
		}

		struct PhysicsVisitContext
		{
			const World* world = nullptr;
			void* visitorContext = nullptr;
			ActorBoundsVisitor visitor = nullptr;
		};
		PhysicsVisitContext physicsContext{ this, context, visitor };
		PhysicsSystem::Get().VisitActorsInBounds(
			bounds,
			&physicsContext,
			[](void* rawContext, Actor* actor)
			{
				auto* visit = static_cast<PhysicsVisitContext*>(rawContext);
				if (!visit || !actor || actor->GetWorld() != visit->world ||
					actor->GetIsPendingDestroy() || actor->GetWeakPtr().expired())
				{
					return true;
				}
				return visit->visitor(visit->visitorContext, actor);
			}
		);

		// Swept projectiles deliberately avoid Box2D bodies, but they still need
		// spatial queries. Traverse only the matching cells of their dedicated
		// index instead of scanning every actor in the world.
		const ManualSpatialCellRange range = GetManualSpatialCellRange(bounds);
		std::uint64_t queryStamp = ++mManualSpatialQueryStamp;
		if (queryStamp == 0)
		{
			// Wraparound is practically unreachable, but zero is reserved as the
			// initial per-actor stamp so retain a valid marker if it happens.
			queryStamp = ++mManualSpatialQueryStamp;
		}

		for (int y = range.minY; y <= range.maxY; ++y)
		{
			for (int x = range.minX; x <= range.maxX; ++x)
			{
				const auto cellIt = mManualSpatialCells.find(ManualSpatialCell{ x, y });
				if (cellIt == mManualSpatialCells.end())
				{
					continue;
				}

				for (Actor* actor : cellIt->second)
				{
					if (!actor || actor->GetWorld() != this ||
						actor->GetIsPendingDestroy() ||
						actor->mLastManualSpatialQueryStamp == queryStamp)
					{
						continue;
					}
					actor->mLastManualSpatialQueryStamp = queryStamp;
					if (!Intersects(GetManualSpatialBounds(*actor), bounds))
					{
						continue;
					}
					if (!visitor(context, actor))
					{
						return;
					}
				}
			}
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

