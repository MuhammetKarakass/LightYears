#include "widget/PauseMenuHUD.h"
#include "widget/MenuLayout.h"
#include "widget/StackPanel.h"
#include "widget/TextWidget.h"

namespace ly
{
	PauseMenuHUD::PauseMenuHUD()
	{
		auto modalLayer = GetLayer(UILayer::Modal).lock();
		auto overlay = modalLayer->AddChild<Panel>();
		auto overlayLayout = UILayout::Stretch({ 50.f, 50.f }, { 50.f, 50.f });
		overlay.lock()->SetLayout(overlayLayout);
		overlay.lock()->SetBackgroundColor({ 60, 60, 60, 150 });

		auto title = modalLayer->AddChild<TextWidget>("Paused");
		title.lock()->SetTextSize(40);
		const sf::FloatRect titleGlyphBounds = title.lock()->GetBound();
		UILayout titleLayout = UILayout::Anchored(UIAnchor::Top, { titleGlyphBounds.position.x, 100.f + titleGlyphBounds.position.y });
		titleLayout.pivot = { .5f, .5f };
		title.lock()->SetLayout(titleLayout);

		UILayout columnLayout = UILayout::Anchored(UIAnchor::Top, { 0.f, 200.f });
		columnLayout.pivot = { .5f, 0.f };
		auto column = BuildMenuColumn(*modalLayer, columnLayout, 0.f).lock();
		mResumeButton = AddMenuButton(*column, "Resume", 25);
		mRestartButton = AddMenuButton(*column, "Restart", 25);
		mQuitButton = AddMenuButton(*column, "Quit", 20);
		mMainMenuButton = AddMenuButton(*column, "Main Menu", 20);
		const float buttonHeight = mResumeButton.lock()->GetIntrinsicSize().y;
		column->SetSpacing(75.f - buttonHeight);
		columnLayout.offset.y -= buttonHeight * .5f;
		column->SetLayout(columnLayout);
	}

	void PauseMenuHUD::Init(sf::RenderWindow& windowRef)
	{
		static_cast<void>(windowRef);
		if (auto button = mResumeButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::ResumeButtonClicked);
		if (auto button = mRestartButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::RestartButtonClicked);
		if (auto button = mQuitButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::QuitButtonClicked);
		if (auto button = mMainMenuButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &PauseMenuHUD::MainMenuButtonClicked);
	}

	bool PauseMenuHUD::HandleEvent(const sf::Event& event)
	{
		const bool handled = HUD::HandleEvent(event);
		if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>(); keyPressed && keyPressed->code == sf::Keyboard::Key::Escape)
		{
			onResumeButtonClicked.Broadcast();
			return true;
		}
		return handled;
	}

	void PauseMenuHUD::ResumeButtonClicked()
	{
		onResumeButtonClicked.Broadcast();
	}

	void PauseMenuHUD::RestartButtonClicked()
	{
		onRestartButtonClicked.Broadcast();
	}

	void PauseMenuHUD::QuitButtonClicked()
	{
		onQuitButtonClicked.Broadcast();
	}

	void PauseMenuHUD::MainMenuButtonClicked()
	{
		onMainMenuButtonClicked.Broadcast();
	}
}
