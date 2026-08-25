#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include "framework/Object.h"
#include "framework/PerfMonitor.h"
#include "framework/camera/CameraManager.h"
#include <optional>

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

		weak_ptr<Actor> GetActorByLayer(CollisionLayer layer) const;
		List<weak_ptr<Actor>> GetActorsInBounds(const sf::FloatRect& bounds) const;

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
		Application* mOwningApp;
		bool mBeganPlay;
		bool mIsPaused;

		List<shared_ptr<Actor>> mActors;
		List<shared_ptr<Actor>> mPendingActors;
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
