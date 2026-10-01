#include "presentation/hud/GameplayWarningHUDController.h"
#include "presentation/hud/warning/GameplayWarningView.h"
#include "widget/GameHUD.h"

namespace ly
{
	GameplayWarningHUDController::GameplayWarningHUDController(weak_ptr<GameHUD> gameHUD)
		: mGameHUD{ gameHUD }, mViewModel{ std::make_shared<GameplayWarningViewModel>() }
	{
	}

	GameplayWarningHUDController::~GameplayWarningHUDController()
	{
		if (const shared_ptr<GameplayWarningView> view = mView.lock()) view->DestroyWidget();
	}

	void GameplayWarningHUDController::Tick(float deltaTime)
	{
		(void)deltaTime;
		const shared_ptr<GameHUD> hud = mGameHUD.lock();
		if (!hud || !hud->HasInit() || !mView.expired()) return;
		mView = hud->AddToLayer<GameplayWarningView>(UILayer::Hud, mViewModel);
	}

	void GameplayWarningHUDController::ShowGameplayWarning(const GameplayWarning& warning)
	{
		const bool isNewWarning = !mViewModel->visible || mViewModel->type != warning.type;
		SetIfChanged(mViewModel->type, warning.type, mViewModel->revision);
		SetIfChanged(mViewModel->text, FormatGameplayWarningText(warning), mViewModel->revision);
		SetIfChanged(mViewModel->visible, true, mViewModel->revision);
		if (isNewWarning) SetIfChanged(mViewModel->activation, mViewModel->activation + 1, mViewModel->revision);
	}

	void GameplayWarningHUDController::HideGameplayWarning(GameplayWarningType warningType)
	{
		if (mViewModel->visible && mViewModel->type == warningType)
			SetIfChanged(mViewModel->visible, false, mViewModel->revision);
	}
}


