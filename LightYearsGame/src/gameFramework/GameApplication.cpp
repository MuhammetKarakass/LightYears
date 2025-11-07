#include "gameFramework/GameApplication.h"
#include "level/LevelOne.h"
#include <framework/AssetManager.h>
#include <Config.h>

ly::Application* GetApplication()
{
	return new ly::GameApplication();
}

namespace ly
{
	GameApplication::GameApplication()
		:Application({ 600, 980 }, 64, std::string("Game Application"), sf::Style::Close|sf::Style::Titlebar)
	{
		AssetManager::GetAssetManager().SetAssetRootDirectory(getResourceDir());
		weak_ptr<LevelOne> NewWorld=LoadWorld<LevelOne>();
		
		counter = 0.f;
	}

	void GameApplication::Tick(float deltaTime)
	{
		/*counter += deltaTime;
		if (counter > 10.f)
		{
			if (!ActorSpaceShip.expired())
			{
				ActorSpaceShip.lock()->Destroy();
			}
		}*/

	}
	/*void GameApplication::SpaceShipMove(shared_ptr<SpaceShip> spaceShip)
	{
		spaceShip->AddActorLocationOffset(spaceShip->GetActorForwardDirection() * 1.f);

	}*/
}