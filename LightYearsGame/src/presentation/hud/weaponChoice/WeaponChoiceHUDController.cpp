#include "presentation/hud/weaponChoice/WeaponChoiceHUDController.h"
#include "presentation/hud/weaponChoice/WeaponChoiceView.h"
#include "widget/GameHUD.h"

namespace vega
{
	WeaponChoiceHUDController::WeaponChoiceHUDController(weak_ptr<GameHUD> gameHUD)
		: mGameHUD{ gameHUD }, mPresenter{ std::make_shared<WeaponChoicePresenter>() }
	{
	}

	WeaponChoiceHUDController::~WeaponChoiceHUDController()
	{
		if (auto view = mView.lock()) view->DestroyWidget();
	}

	void WeaponChoiceHUDController::Tick(float deltaTime)
	{
		mPresenter->Tick(deltaTime);
		auto hud = mGameHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<WeaponChoiceView>(UILayer::Modal, mPresenter->GetViewModel(), std::weak_ptr<WeaponChoicePresenter>{ mPresenter });
	}

	bool WeaponChoiceHUDController::HandleEvent(const sf::Event& event)
	{
		const auto* pressed = event.getIf<sf::Event::KeyPressed>();
		if (!pressed || !mPresenter->GetViewModel()->open) return false;
		const int code = static_cast<int>(pressed->code);
		const int first = static_cast<int>(sf::Keyboard::Key::Num1), last = static_cast<int>(sf::Keyboard::Key::Num9);
		if (code < first || code > last) return false;
		return mPresenter->Choose(static_cast<std::size_t>(code - first));
	}
}
