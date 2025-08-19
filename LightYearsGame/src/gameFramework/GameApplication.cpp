#include "gameFramework/GameApplication.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/AssetManager.h>
#include <Config.h>
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"

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
		testActor.lock()->SetActorRotation(0.f);

		ActorSpaceShip = NewWorld.lock()->SpawnActor<SpaceShip>();
		ActorSpaceShip.lock()->SetTexture("SpaceShooterRedux/PNG/playerShip1_blue.png");
		ActorSpaceShip.lock()->SetActorLocation({ 200.f, 100.f });
		ActorSpaceShip.lock()->SetActorRotation(0.f);		
		counter = 0.f;
	}

	void GameApplication::Tick(float deltaTime)
	{
		counter += deltaTime;
		if (counter > 10.f)
		{
			if (!ActorSpaceShip.expired())
			{
				ActorSpaceShip.lock()->Destroy();
			}
		}

	}
	/*void GameApplication::SpaceShipMove(shared_ptr<SpaceShip> spaceShip)
	{
		spaceShip->AddActorLocationOffset(spaceShip->GetActorForwardDirection() * 1.f);

	}*/
}