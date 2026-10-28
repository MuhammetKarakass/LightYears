#include "presentation/hud/evolveChoice/EvolveChoiceHUDController.h"
#include "presentation/hud/evolveChoice/EvolveChoiceView.h"
#include "presentation/hud/modules/ModuleScreenPresenter.h"
#include "widget/GameHUD.h"

namespace vega
{
	EvolveChoiceHUDController::EvolveChoiceHUDController(weak_ptr<GameHUD> hud, std::weak_ptr<ModuleScreenPresenter> modules) : mHUD{ hud }, mModules{ modules }, mPresenter{ std::make_shared<EvolveChoicePresenter>() } {}
	EvolveChoiceHUDController::~EvolveChoiceHUDController() { if (auto view = mView.lock()) view->DestroyWidget(); }

	void EvolveChoiceHUDController::Tick(float deltaTime)
	{
		mPresenter->Tick(deltaTime);
		if (const auto modules = mModules.lock(); modules && modules->GetViewModel()->isOpen) mPresenter->Close();
		const auto hud = mHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<EvolveChoiceView>(UILayer::Modal, mPresenter->GetViewModel());
	}

	bool EvolveChoiceHUDController::HandleEvent(const sf::Event& event)
	{
		if (const auto modules = mModules.lock(); modules && modules->GetViewModel()->isOpen) { mPresenter->Close(); return false; }
		return mPresenter->HandleEvent(event);
	}
}
