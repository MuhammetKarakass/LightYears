#pragma once

#include "framework/World.h"

namespace ly
{
	class MainMenuHUD;
	class MainMenuLevel : public World
	{
	public:
		MainMenuLevel(Application* owningApplication);

	protected:
		virtual void BeginPlay() override;

	private:
		weak_ptr<MainMenuHUD> mMainMenuHUD;

		void StartGame();
		void QuitGame();
	};
}