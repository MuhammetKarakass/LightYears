#include "level/LevelOne.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include "enemy/Vanguard.h"
#include <gameplay/GameStage.h>
#include "enemy/VanguardStage.h"
#include "enemy/TwinBladeStage.h"
#include "enemy/HexagonStage.h"
#include "enemy/UFOStage.h"
#include <gameplay/WaitStage.h>


namespace ly
{
	LevelOne::LevelOne(Application* owningApp)
		:World(owningApp)
	{
		testActor = SpawnActor<PlayerSpaceShip>();
		testActor.lock()->SetActorLocation({ 300.f, 490.f });
	}

	void LevelOne::BeginPlay()
	{
		
	}

	void LevelOne::Tick(float deltaTime)
	{
	}


	void LevelOne::InitGameStages()
	{
		AddGameStage(shared_ptr<UFOStage>{new UFOStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
		AddGameStage(shared_ptr<VanguardStage>{new VanguardStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this,5.f)});
		AddGameStage(shared_ptr<TwinBladeStage>{new TwinBladeStage(this)});
		AddGameStage(shared_ptr<HexagonStage>{new HexagonStage(this)});
		AddGameStage(shared_ptr<WaitStage>{new WaitStage(this, 5.f)});
	}
}