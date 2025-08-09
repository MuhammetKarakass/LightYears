#pragma once
#include <framework/Application.h>
#include "framework/Core.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class Actor;
	class GameApplication : public Application
	{
	public:

		GameApplication();
		float counter{ 0.f };
		weak_ptr<Actor> actorToDestroy;

		virtual void Tick(float deltaTime) override;
	};
}