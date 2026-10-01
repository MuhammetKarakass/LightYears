#include "presentation/hud/modules/ModuleScreenHUDController.h"
#include "presentation/hud/modules/ModuleScreenView.h"
#include "widget/GameHUD.h"

namespace ly
{
	ModuleScreenHUDController::ModuleScreenHUDController(weak_ptr<GameHUD> gameHUD)
		: mGameHUD{ gameHUD }, mPresenter{ std::make_shared<ModuleScreenPresenter>() }
	{
	}

	ModuleScreenHUDController::~ModuleScreenHUDController()
	{
		if (auto view = mView.lock()) view->DestroyWidget();
	}

	void ModuleScreenHUDController::Tick(float deltaTime)
	{
		mPresenter->Tick(deltaTime);
		auto hud = mGameHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<ModuleScreenView>(UILayer::Menu, mPresenter->GetViewModel(), weak_ptr<ModuleScreenPresenter>{ mPresenter });
	}

	bool ModuleScreenHUDController::HandleEvent(const sf::Event& event)
	{
		if (const auto* released = event.getIf<sf::Event::KeyReleased>())
		{
			if (released->code == sf::Keyboard::Key::Tab) mToggleKeyDown = false;
			return false;
		}
		const auto* pressed = event.getIf<sf::Event::KeyPressed>();
		if (!pressed || pressed->code != sf::Keyboard::Key::Tab) return false;
		// Key repeat sends further presses while Tab is held; only the first toggles.
		if (!mToggleKeyDown) mPresenter->ToggleOpen();
		mToggleKeyDown = true;
		return true;
	}
}
