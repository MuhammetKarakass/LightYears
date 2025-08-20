#include "level/LevelOne.h"

#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include "enemy/Vanguard.h"

namespace ly
{
	LevelOne::LevelOne(Application* owningApp)
		:World(owningApp)
	{
		testActor =SpawnActor<PlayerSpaceShip>();
		testActor.lock()->SetActorLocation({ 300.f, 490.f });

		ActorSpaceShip =SpawnActor<Vanguard>();
		ActorSpaceShip.lock()->SetActorLocation({ 200.f, 100.f });
	}
}