#pragma once

#include "presentation/hud/HUDController.h"
#include "presentation/hud/evolveChoice/EvolveChoicePresenter.h"

namespace vega
{
	class GameHUD;
	class EvolveChoiceView;
	class ModuleScreenPresenter;
	class EvolveChoiceHUDController : public HUDController
	{
	public:
		explicit EvolveChoiceHUDController(weak_ptr<GameHUD> hud, std::weak_ptr<ModuleScreenPresenter> modules = {});
		~EvolveChoiceHUDController() override;
		void Tick(float deltaTime) override;
		bool HandleEvent(const sf::Event& event) override;
		const std::shared_ptr<EvolveChoicePresenter>& GetPresenter() const { return mPresenter; }
	private:
		weak_ptr<GameHUD> mHUD;
		std::weak_ptr<ModuleScreenPresenter> mModules;
		std::shared_ptr<EvolveChoicePresenter> mPresenter;
		weak_ptr<EvolveChoiceView> mView;
	};
}
