#include "gameFramework/GameApplication.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/AssetManager.h>
#include <Config.h>
#include "player/PlayerSpaceShip.h"

ly::Application* GetApplication()
{
	return new ly::GameApplication();
}

namespace ly
{
	GameApplication::GameApplication()
		:Application({ 600, 980 }, 32, std::string("Game Application"), sf::Style::Close|sf::Style::Titlebar)
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory(getResourceDir());
		weak_ptr<World> NewWorld=LoadWorld<World>();
		testActor = NewWorld.lock()->SpawnActor<PlayerSpaceShip>();
		testActor.lock()->SetActorLocation({ 300.f, 490.f });
		testActor.lock()->SetActorRotation(0.f);	}
	void GameApplication::Tick(float deltaTime)
	{
		/*counter += deltaTime;
		if (counter > 5.f)
		{
			if (!testActor.expired())
			{
				testActor.lock()->Destroy();
			}
		}*/
	}
}