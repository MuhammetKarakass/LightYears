#pragma once
#include <framework/Application.h>
#include "framework/Core.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class Actor;
	class PlayerSpaceShip;
	class GameApplication : public Application
	{
	public:

		GameApplication();
		float counter{ 0.f };
		weak_ptr<PlayerSpaceShip> testActor;

		virtual void Tick(float deltaTime) override;
	};
}