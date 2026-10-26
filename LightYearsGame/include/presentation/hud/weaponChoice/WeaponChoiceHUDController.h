#pragma once

#include "framework/Core.h"
#include "presentation/hud/HUDController.h"
#include "presentation/hud/weaponChoice/WeaponChoicePresenter.h"

#include <memory>

namespace vega
{
	class GameHUD;
	class WeaponChoiceView;

	// Shows the Second Hardpoint weapon choice while one is pending; number keys 1-9 pick a row. The game keeps running.
	class WeaponChoiceHUDController : public HUDController
	{
	public:
		explicit WeaponChoiceHUDController(weak_ptr<GameHUD> gameHUD);
		~WeaponChoiceHUDController() override;
		void Tick(float deltaTime) override;
		bool HandleEvent(const sf::Event& event) override;
		const std::shared_ptr<WeaponChoicePresenter>& GetPresenter() const { return mPresenter; }

	private:
		weak_ptr<GameHUD> mGameHUD;
		std::shared_ptr<WeaponChoicePresenter> mPresenter;
		weak_ptr<WeaponChoiceView> mView;
	};
}
