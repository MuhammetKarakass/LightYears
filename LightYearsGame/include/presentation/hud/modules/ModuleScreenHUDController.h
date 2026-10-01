#pragma once

#include "presentation/hud/HUDController.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"

namespace ly
{
	class GameHUD;
	class ModuleScreenView;

	// Opens/closes the module screen with Tab. The game keeps running while it is open.
	class ModuleScreenHUDController : public HUDController
	{
	public:
		explicit ModuleScreenHUDController(weak_ptr<GameHUD> gameHUD);
		~ModuleScreenHUDController() override;
		void Tick(float deltaTime) override;
		bool HandleEvent(const sf::Event& event) override;
		const shared_ptr<ModuleScreenPresenter>& GetPresenter() const { return mPresenter; }

	private:
		weak_ptr<GameHUD> mGameHUD;
		shared_ptr<ModuleScreenPresenter> mPresenter;
		weak_ptr<ModuleScreenView> mView;
		bool mToggleKeyDown{ false };
	};
}
