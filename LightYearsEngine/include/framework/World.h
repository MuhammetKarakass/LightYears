#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include "framework/Object.h"

namespace ly
{
	class Actor;
	class Application;
	class GameStage;
	class World: public Object
	{
	public:

		World(Application* owningApp);

		void BeginPlayInternal();
		void TickInternal(float deltaTime);
		void Render(sf::RenderWindow& window);
		void CleanCycle();

		sf::Vector2u GetWindowSize();

		virtual ~World();

		void AddGameStage(shared_ptr<GameStage> newStage);

		template<typename ActorType, typename ...Args>
		weak_ptr<ActorType> SpawnActor(Args... args);

	protected:

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);

	private:
		Application* mOwningApp;
		bool mBeganPlay;

		List<shared_ptr<Actor>> mActors;
		List<shared_ptr<Actor>> mPendingActors;
		List<shared_ptr<GameStage>> mGameStages;
		List<shared_ptr<GameStage>>::iterator mCurrentStage;

		virtual void InitGameStages();
		virtual void AllGameStagesFinished();
		void NextGameStage();
		void BeginStages();

	};

	template<typename ActorType, typename ...Args>
	weak_ptr<ActorType> World::SpawnActor(Args... args)
	{
			shared_ptr<ActorType> newActor{ new ActorType(this,args...) };
			mPendingActors.push_back(newActor);
		
		return newActor;
	}
}