#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include "framework/Object.h"
#include "framework/SimulationTime.h"
#include "framework/PerfMonitor.h"
#include "framework/camera/CameraManager.h"
#include <optional>
#include <type_traits>

namespace ly
{
	class Actor;
	class Application;
	class GameStage;
	class HUD;
	class World: public Object
	{
	public:

		World(Application* owningApp);

		void BeginPlayInternal();
		void TickInternal(float deltaTime);

		// Selective time modifiers are evaluated per actor domain. This keeps the
		// engine clock, stage clock, camera, HUD, and global timers real-time.
		void SetSimulationTimeModifier(
			SimulationTimeDomain domain,
			SimulationTimeModifierSourceId sourceId,
			float multiplier
		);
		void RemoveSimulationTimeModifier(
			SimulationTimeDomain domain,
			SimulationTimeModifierSourceId sourceId
		);
		float GetSimulationTimeScale(SimulationTimeDomain domain) const;
		void Render(sf::RenderWindow& window);
		void CleanCycle();

		void SetPaused(bool paused);
		bool IsPaused() const { return mIsPaused; }
		void RemoveOverlayHUD();

		void SetViewTarget(weak_ptr<Actor> target);
		void ClearViewTarget();
		void SetCameraSettings(const CameraSettings& settings);
		void SetCameraExternalVelocity(const sf::Vector2f& velocity);
		void ClearCameraExternalVelocity();
		void SetCameraPreserveFollowTargetOffset(bool preserve);
		void SetCameraAdditionalZoomOut(float zoomOut);
		void SetCameraRelativeAdditionalZoomOut(float zoomOutRatio);
		void SetCameraLookAheadWorldPosition(const std::optional<sf::Vector2f>& worldPosition);
		void ClearCameraLookAheadWorldPosition();
		void SetCameraWorldBounds(const sf::FloatRect& bounds);
		void ClearCameraWorldBounds();
		void PlayCameraShake(float amplitude, float duration, float frequency);

		// Keeps the non-Box2D broadphase in sync for actors that are gameplay
		// queryable but intentionally have no physics body (for example swept
		// projectiles). These are engine-internal hooks called by Actor state
		// changes.
		void RefreshActorSpatialQuery(Actor& actor);
		void RemoveActorSpatialQuery(Actor& actor);

		weak_ptr<Actor> GetActorByLayer(CollisionLayer layer) const;
		List<weak_ptr<Actor>> GetActorsInBounds(const sf::FloatRect& bounds) const;

		// Allocation-free spatial traversal for repeated gameplay queries such as
		// projectile sweeps. The visitor is invoked only for world-owned actors.
		template<typename Visitor>
		void ForEachActorInBounds(
			const sf::FloatRect& bounds,
			Visitor&& visitor
		) const
		{
			using VisitorType = std::remove_reference_t<Visitor>;
			const auto invoke = [](void* context, Actor* actor)
			{
				(*static_cast<VisitorType*>(context))(*actor);
				return true;
			};
			VisitActorsInBounds(bounds, &visitor, invoke);
		}

		sf::Vector2u GetWindowSize();
		sf::View GetWorldView() const;
		sf::Vector2f GetMouseWorldPosition() const;

		virtual ~World();

		void AddGameStage(shared_ptr<GameStage> newStage);
		virtual bool DispatchEvent(const sf::Event& event);

		Application* GetApplication() const { return mOwningApp; }
		const Application* GetApplicationRef() const { return mOwningApp; }

		template<typename ActorType, typename ...Args>
		weak_ptr<ActorType> SpawnActor(Args... args);

		template<typename HUDType, typename ...Args>
		weak_ptr<HUDType> SpawnHUD(Args... args);

		template<typename HUDType, typename ...Args>
		weak_ptr<HUDType> SpawnOverlayHUD(Args... args);

		template<typename HUDType>
		weak_ptr<HUDType> GetHUD() const
		{
			return dynamic_pointer_cast<HUDType>(mHUD);
		}

		template<typename ActorType>
		weak_ptr<ActorType> GetActorByType() const
		{
			for (const auto& actor: mActors)
			{
				if (auto castedActor = dynamic_cast<ActorType*>(actor.get()))
				{
					return weak_ptr<ActorType>(std::static_pointer_cast<ActorType>(actor));
				}
			}
			return weak_ptr<ActorType>();
		}

		template<typename ActorType>
		List<weak_ptr<ActorType>> GetActorsByType() const
		{
			List<weak_ptr<ActorType>> result;
			for (const auto& actor : mActors)
			{
				if (auto castedActor = dynamic_cast<ActorType*>(actor.get()))
				{
					result.push_back(weak_ptr<ActorType>(std::static_pointer_cast<ActorType>(actor)));
				}
			}
			return result;
		}

	protected:

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void OnActorSpawned(Actor* actor);

	private:
		struct ManualSpatialCell
		{
			int x = 0;
			int y = 0;

			bool operator==(const ManualSpatialCell& other) const
			{
				return x == other.x && y == other.y;
			}
		};

		struct ManualSpatialCellHash
		{
			std::size_t operator()(const ManualSpatialCell& cell) const
			{
				const std::size_t x = std::hash<int>{}(cell.x);
				const std::size_t y = std::hash<int>{}(cell.y);
				return x ^ (y + 0x9e3779b9u + (x << 6u) + (x >> 2u));
			}
		};

		struct ManualSpatialCellRange
		{
			int minX = 0;
			int maxX = -1;
			int minY = 0;
			int maxY = -1;
		};

		Application* mOwningApp;
		bool mBeganPlay;
		bool mIsPaused;

		List<shared_ptr<Actor>> mActors;
		List<shared_ptr<Actor>> mPendingActors;
		// Separate broadphase for actors that deliberately avoid a Box2D body.
		// This prevents every gameplay spatial query from scanning all of mActors.
		Dictionary<ManualSpatialCell, List<Actor*>, ManualSpatialCellHash> mManualSpatialCells;
		Dictionary<Actor*, ManualSpatialCellRange> mManualSpatialActorCells;
		Set<Actor*> mSpatiallyRegisteredActors;
		Map<
			SimulationTimeDomain,
			Map<SimulationTimeModifierSourceId, float>
		> mSimulationTimeModifiers;
		mutable std::uint64_t mManualSpatialQueryStamp = 0;
		List<List<Actor*>> mRenderBuckets;
		List<shared_ptr<GameStage>> mGameStages;
		List<shared_ptr<GameStage>>::iterator mCurrentStage;
		shared_ptr<HUD> mHUD;
		shared_ptr<HUD> mOverlayHUD;

		virtual void InitGameStages();
		virtual void AllGameStagesFinished();
		void NextGameStage();
		void BeginStages();
		void UpdateCamera(float deltaTime);
		void RenderHUD(sf::RenderWindow& window);
		void RegisterActorSpatialQuery(Actor& actor);
		static bool ShouldUseManualSpatialQuery(const Actor& actor);
		static sf::FloatRect GetManualSpatialBounds(const Actor& actor);
		static ManualSpatialCellRange GetManualSpatialCellRange(const sf::FloatRect& bounds);
		using ActorBoundsVisitor = bool(*)(void* context, Actor* actor);
		void VisitActorsInBounds(
			const sf::FloatRect& bounds,
			void* context,
			ActorBoundsVisitor visitor
		) const;

		weak_ptr<Actor> mViewTarget;
		CameraManager mCameraManager;
	};

	template<typename ActorType, typename ...Args>
	weak_ptr<ActorType> World::SpawnActor(Args... args)
	{
		shared_ptr<ActorType> newActor = std::make_shared<ActorType>(this, args...);
		mPendingActors.push_back(newActor);
		ly::perf::IncActiveActors();
		return newActor;
	}

	template<typename HUDType, typename ...Args>
	weak_ptr<HUDType> World::SpawnHUD(Args... args)
	{
		shared_ptr<HUDType> newHUD = std::make_shared<HUDType>(args...);
		mHUD = newHUD;
		return newHUD;
	}

	template<typename HUDType, typename ...Args>
	weak_ptr<HUDType> World::SpawnOverlayHUD(Args... args)
	{
		shared_ptr<HUDType> newHUD = std::make_shared<HUDType>(args...);
		mOverlayHUD = newHUD;
		return newHUD;
	}
}
