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
		sf::Vector2u GetWindowSize();

		virtual ~World();

		template<typename ActorType>
		weak_ptr<ActorType> SpawnActor();

	private:

		void BeginPlay();
		void Tick(float deltaTime);
		Application* mOwningApp;
		bool mBeganPlay;

		List<shared_ptr<Actor>> mActors;
		List<shared_ptr<Actor>> mPendingActors;
	};

	template<typename ActorType>
	weak_ptr<ActorType> World::SpawnActor()
	{
			shared_ptr<ActorType> newActor{ new ActorType(this) };
			mPendingActors.push_back(newActor);
		
		return newActor;
	}
}