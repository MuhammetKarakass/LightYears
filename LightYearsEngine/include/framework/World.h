#pragma once
#include "framework/Core.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class Actor;
	class Application;
	class World
	{
	public:

		World(Application* owningApp);

		void BeginPlayInternal();
		void TickInternal(float deltaTime);
		void Render(sf::RenderWindow& window);
		void CleanCycle();

		sf::Vector2u GetWindowSize();

		virtual ~World();

		template<typename ActorType, typename ...Args>
		weak_ptr<ActorType> SpawnActor(Args... args);

	private:

		void BeginPlay();
		void Tick(float deltaTime);
		Application* mOwningApp;
		bool mBeganPlay;

		List<shared_ptr<Actor>> mActors;
		List<shared_ptr<Actor>> mPendingActors;
	};

	template<typename ActorType, typename ...Args>
	weak_ptr<ActorType> World::SpawnActor(Args... args)
	{
			shared_ptr<ActorType> newActor{ new ActorType(this,args...) };
			mPendingActors.push_back(newActor);
		
		return newActor;
	}
}