#pragma once

#include <framework/Application.h>

namespace ly
{
	class GameApplication : public Application
	{
	public:
		GameApplication();

	private:
		virtual void Tick(float deltaTime) override;
	};
}


