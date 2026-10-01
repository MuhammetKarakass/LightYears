#include "widget/GameOverHUD.h"
#include "widget/MenuLayout.h"
#include "widget/StackPanel.h"
#include "widget/TextWidget.h"

namespace ly
{
	namespace
	{
		void UpdateCenteredTextLayout(TextWidget& text, float centerY)
		{
			const sf::FloatRect glyphBounds = text.GetBound();
			const sf::Vector2f widgetLocation = text.GetWidgetLocation();
			UILayout layout = text.GetLayout();
			layout.offset = { glyphBounds.position.x - widgetLocation.x, centerY + glyphBounds.position.y - widgetLocation.y };
			text.SetLayout(layout);
		}
	}

	GameOverHUD::GameOverHUD()
	{
		auto modalLayer = GetLayer(UILayer::Modal).lock();
		auto overlay = modalLayer->AddChild<Panel>();
		overlay.lock()->SetLayout(UILayout::Stretch({ 50.f, 50.f }, { 50.f, 50.f }));
		overlay.lock()->SetBackgroundColor({ 60, 60, 60, 150 });

		mTitleText = modalLayer->AddChild<TextWidget>("Game Over");
		mTitleText.lock()->SetTextSize(40);
		UILayout titleLayout = UILayout::Anchored(UIAnchor::Top);
		titleLayout.pivot = { .5f, .5f };
		mTitleText.lock()->SetLayout(titleLayout);
		UpdateCenteredTextLayout(*mTitleText.lock(), 150.f);

		mScoreText = modalLayer->AddChild<TextWidget>("Score: 0");
		mScoreText.lock()->SetTextSize(30);
		UILayout scoreLayout = UILayout::Anchored(UIAnchor::Top);
		scoreLayout.pivot = { .5f, .5f };
		mScoreText.lock()->SetLayout(scoreLayout);
		UpdateCenteredTextLayout(*mScoreText.lock(), 200.f);

		UILayout columnLayout = UILayout::Anchored(UIAnchor::Top, { 0.f, 300.f });
		columnLayout.pivot = { .5f, 0.f };
		auto column = BuildMenuColumn(*modalLayer, columnLayout, 0.f).lock();
		mRestartButton = AddMenuButton(*column, "Restart", 25);
		mMainMenuButton = AddMenuButton(*column, "Main Menu", 20);
		mQuitButton = AddMenuButton(*column, "Quit", 20);
		const float buttonHeight = mRestartButton.lock()->GetIntrinsicSize().y;
		column->SetSpacing(75.f - buttonHeight);
		columnLayout.offset.y -= buttonHeight * .5f;
		column->SetLayout(columnLayout);
	}

	void GameOverHUD::Init(sf::RenderWindow& windowRef)
	{
		static_cast<void>(windowRef);
		if (auto button = mRestartButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &GameOverHUD::RestartButtonClicked);
		if (auto button = mQuitButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &GameOverHUD::QuitButtonClicked);
		if (auto button = mMainMenuButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &GameOverHUD::MainMenuButtonClicked);
	}

	void GameOverHUD::SetTitleText(const std::string& titleText)
	{
		if (auto title = mTitleText.lock())
		{
			title->SetString(titleText);
			UpdateCenteredTextLayout(*title, 150.f);
		}
	}

	void GameOverHUD::SetScoreText(unsigned int score)
	{
		if (auto text = mScoreText.lock())
		{
			text->SetString("Score: " + std::to_string(score));
			UpdateCenteredTextLayout(*text, 200.f);
		}
	}

	void GameOverHUD::RestartButtonClicked()
	{
		onRestartButtonClicked.Broadcast();
	}

	void GameOverHUD::QuitButtonClicked()
	{
		onQuitButtonClicked.Broadcast();
	}

	void GameOverHUD::MainMenuButtonClicked()
	{
		onMainMenuButtonClicked.Broadcast();
	}
}
