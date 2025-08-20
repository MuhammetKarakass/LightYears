#pragma once
#include <framework/Application.h>
#include "framework/Core.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class Vanguard;
	class Actor;
	class PlayerSpaceShip;
	class SpaceShip;
	class GameApplication : public Application
	{
	public:

		GameApplication();
		float counter{ 0.f };
		

		virtual void Tick(float deltaTime) override;

		void SpaceShipMove(shared_ptr<SpaceShip> spaceShip);
		
	};
}