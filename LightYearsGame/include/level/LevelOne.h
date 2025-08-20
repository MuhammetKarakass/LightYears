#pragma once

#include "framework/World.h"

namespace ly
{
	class Vanguard;
	class PlayerSpaceShip;
	class LevelOne : public World
	{

	public:
		LevelOne(Application* owningApp);

		weak_ptr<Vanguard> ActorSpaceShip;
		weak_ptr<PlayerSpaceShip> testActor;
	};
}