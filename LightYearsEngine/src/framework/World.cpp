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

		// Hitch tracing: an actor whose tick, BeginPlay or render takes a millisecond or more is reported under its
		// class name, so a slow frame names the actor type responsible.
		class SlowActorTrace
		{
		public:
			explicit SlowActorTrace(const Actor& actor)
#if LY_ENABLE_PROFILING
				: mActor(actor), mStart(std::chrono::steady_clock::now())
#endif
			{
				(void)actor;
			}

			~SlowActorTrace()
			{
#if LY_ENABLE_PROFILING
				const double elapsed = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - mStart).count();
				if (elapsed >= debug::Profiler::FrameEventThresholdMicroseconds)
				{
					debug::Profiler::RecordFrameEvent(typeid(mActor).name(), elapsed);
				}
#endif
			}

		private:
#if LY_ENABLE_PROFILING
			const Actor& mActor;
			std::chrono::steady_clock::time_point mStart;
#endif
		};
	}

	World::World(Application* owningApp):
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
		LY_PROFILE_SCOPE("World Tick");
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
				LY_PROFILE_SCOPE("World.Promote");
			promotePendingActors();
			}

			{
				LY_PROFILE_SCOPE("World.ActorsTick");
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
				LY_PROFILE_SCOPE("World.StageTick");
				mCurrentStage->get()->TickStage(deltaTime);
			}

			if (mBeganPlay)
			{
				LY_PROFILE_SCOPE("World.LevelTick");
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
			LY_PROFILE_SCOPE("World.HUDTick");
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
		const sf::Vector2f location = actor.GetActorLocation();
		sf::Vector2f minimum{ location.x - radius, location.y - radius };
		sf::Vector2f maximum{ location.x + radius, location.y + radius };
		bool hasShape = radius > 0.f;
		// Body-less static geometry still has collision boxes. Index their full
		// bounds, not just the actor origin, so exact edge/end contacts are found.
		const float rotation = actor.GetActorRotation() * 0.01745329251994329577f;
		const float cosine = std::cos(rotation);
		const float sine = std::sin(rotation);
		for (std::size_t index = 0; index < actor.GetPhysicsCollisionBoxCount(); ++index)
		{
			const PhysicsCollisionBox box = actor.GetPhysicsCollisionBox(index);
			if (box.halfExtents.x <= 0.f || box.halfExtents.y <= 0.f) continue;
			const sf::Vector2f center = location + sf::Vector2f{
				cosine * box.localCenter.x - sine * box.localCenter.y,
				sine * box.localCenter.x + cosine * box.localCenter.y };
			const float angle = rotation + box.localRotationDegrees * 0.01745329251994329577f;
			const float c = std::abs(std::cos(angle));
			const float s = std::abs(std::sin(angle));
			const sf::Vector2f extent{ c * box.halfExtents.x + s * box.halfExtents.y,
				s * box.halfExtents.x + c * box.halfExtents.y };
			minimum.x = std::min(minimum.x, center.x - extent.x);
			minimum.y = std::min(minimum.y, center.y - extent.y);
			maximum.x = std::max(maximum.x, center.x + extent.x);
			maximum.y = std::max(maximum.y, center.y + extent.y);
			hasShape = true;
		}
		if (hasShape) return { minimum, maximum - minimum };

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
		mIsPaused = IsEditMode() || paused;
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

	void World::Render(sf::RenderTarget& window)
	{
		LY_PROFILE_SCOPE("World Render");
		sf::View previousView = window.getView();
		sf::View worldView = GetWorldView();
		WorldRenderPipeline* pipeline = mOwningApp ? mOwningApp->GetWorldRenderPipeline() : nullptr;
		if (pipeline && !pipeline->IsActive())
		{
			pipeline = nullptr;
		}
		if (pipeline)
		{
			LY_PROFILE_SCOPE("World.PipelineBegin");
			pipeline->BeginFrame(*this, worldView);
		}
		else
		{
			window.setView(worldView);
		}
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

		{
			LY_PROFILE_SCOPE("World.RenderCull");
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
		}
		ly::perf::SetRenderStats(renderCandidates, renderCulled, renderSubmitted);

		{
			LY_PROFILE_SCOPE("World.RenderDraw");
		for (std::size_t layerIndex = 0; layerIndex < mRenderBuckets.size(); ++layerIndex)
		{
			const RenderLayer layer = static_cast<RenderLayer>(layerIndex);
			if (pipeline && pipeline->ReplacesLayer(layer))
			{
				continue;
			}
			sf::RenderTarget& target = pipeline ? pipeline->GetTarget(layer) : static_cast<sf::RenderTarget&>(window);
			for (Actor* actor : mRenderBuckets[layerIndex])
			{
				if (pipeline && pipeline->HidesActor(*actor))
				{
					continue;
				}
				const SlowActorTrace trace{ *actor };
				actor->Render(target);
			}
		}
		}

		if (pipeline)
		{
			LY_PROFILE_SCOPE("World.PipelineCompose");
			pipeline->Compose(window);
		}
		window.setView(window.getDefaultView());
		{
			LY_PROFILE_SCOPE("World.RenderHUD");
		RenderHUD(window);
		}

		window.setView(previousView);
	}

	void World::SetRenderViewport(RenderViewport* viewport)
	{
		mRenderViewport = viewport;
	}

	RenderViewport* World::GetRenderViewport()
	{
		return mRenderViewport ? mRenderViewport : (mOwningApp ? &mOwningApp->GetWindowViewport() : nullptr);
	}

	const RenderViewport* World::GetRenderViewport() const
	{
		return mRenderViewport ? mRenderViewport : (mOwningApp ? &mOwningApp->GetWindowViewport() : nullptr);
	}

	sf::Vector2u World::GetWindowSize()
	{
		const RenderViewport* viewport = GetRenderViewport();
		return viewport ? viewport->GetSize() : sf::Vector2u{};
	}

	sf::View World::GetWorldView() const
	{
		if (mViewOverride) return *mViewOverride;
		const RenderViewport* viewport = GetRenderViewport();
		if (!viewport)
		{
			return mCameraManager.GetView(sf::View{});
		}
		return mCameraManager.GetView(viewport->GetDefaultView());
	}

	sf::Vector2f World::GetMouseWorldPosition() const
	{
		const RenderViewport* viewport = GetRenderViewport();
		if (!viewport)
		{
			// No viewport to read the cursor from; aim at the view center.
			return GetWorldView().getCenter();
		}
		return viewport->MapPixelToCoords(viewport->GetMousePixelPosition(), GetWorldView());
	}

	bool World::HasInputFocus() const
	{
		const RenderViewport* viewport = GetRenderViewport();
		return viewport && viewport->HasInputFocus();
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
		LY_PROFILE_SCOPE("Physics.Step");
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
	void World::UpdateCamera(float deltaTime)
	{
		if (const RenderViewport* viewport = GetRenderViewport())
		{
			mCameraManager.Update(deltaTime, viewport->GetDefaultView());
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
			void* visitorContext = nullptr;
			ActorBoundsVisitor visitor = nullptr;
			std::unordered_set<std::uint64_t>* visitedActorIds = nullptr;
			bool stopped = false;
		};
		std::unordered_set<std::uint64_t> visitedActorIds;
		PhysicsVisitContext physicsContext{ context, visitor, &visitedActorIds, false };
		mPhysicsScene->VisitActorsInBounds(
			bounds,
			&physicsContext,
			[](void* rawContext, Actor* actor)
			{
				auto* visit = static_cast<PhysicsVisitContext*>(rawContext);
				if (!visit || !actor || actor->GetIsPendingDestroy() ||
					actor->GetWeakPtr().expired())
				{
					return true;
				}
				if (!visit->visitedActorIds->insert(actor->GetUniqueID()).second)
				{
					return true;
				}
				visit->stopped = !visit->visitor(visit->visitorContext, actor);
				return !visit->stopped;
			}
		);
		if (physicsContext.stopped)
		{
			return;
		}

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

		// Collect first: the visitor may destroy or move actors, which mutates the
		// cell containers being traversed here.
		std::vector<Actor*> candidates;
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
					if (!visitedActorIds.insert(actor->GetUniqueID()).second)
					{
						continue;
					}
					candidates.push_back(actor);
				}
			}
		}

		for (Actor* actor : candidates)
		{
			if (actor->GetWorld() != this || actor->GetIsPendingDestroy())
			{
				continue;
			}
			if (!visitor(context, actor))
			{
				return;
			}
		}
	}
	void World::RenderHUD(sf::RenderTarget& window)
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

