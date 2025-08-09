#include "gameFramework/GameApplication.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <Config.h>

ly::Application* GetApplication()
{
	return new ly::GameApplication();
}

namespace ly
{
	GameApplication::GameApplication()
		:Application({ 600, 980 }, 32, std::string("Game Application"), sf::Style::Close|sf::Style::Titlebar)
	{
		weak_ptr<World> NewWorld=LoadWorld<World>();
		NewWorld.lock()->SpawnActor<Actor>();
		actorToDestroy = NewWorld.lock()->SpawnActor<Actor>();
		actorToDestroy.lock()->SetTexture(getResourceDir()+ "SpaceShooterRedux/PNG/playerShip1_blue.png");
		actorToDestroy.lock()->SetActorLocation({ 300.f, 490.f });
		actorToDestroy.lock()->SetActorRotation(45.f);
	}
	void GameApplication::Tick(float deltaTime)
	{
		counter += deltaTime;
		if (counter > 2.f)
		{
			if (!actorToDestroy.expired())
			{
				actorToDestroy.lock()->Destroy();
			}
		}
	}
}