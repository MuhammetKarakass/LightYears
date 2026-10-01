#include "widget/MainMenuHUD.h"
#include "widget/MenuLayout.h"
#include "widget/StackPanel.h"
#include "widget/TextWidget.h"

namespace ly
{
	namespace
	{
		void SetLegacyCenteredTextLayout(TextWidget& text, float centerY)
		{
			const sf::FloatRect glyphBounds = text.GetBound();
			UILayout layout = UILayout::Anchored(UIAnchor::Top, { glyphBounds.position.x, centerY + glyphBounds.position.y });
			layout.pivot = { .5f, .5f };
			text.SetLayout(layout);
		}
	}

	MainMenuHUD::MainMenuHUD()
	{
		auto menuLayer = GetLayer(UILayer::Menu).lock();
		auto title = menuLayer->AddChild<TextWidget>("Light Years");
		title.lock()->SetTextSize(40);
		SetLegacyCenteredTextLayout(*title.lock(), 100.f);

		UILayout columnLayout = UILayout::Anchored(UIAnchor::Center, {});
		columnLayout.pivot = { .5f, 0.f };
		auto column = BuildMenuColumn(*menuLayer, columnLayout, 0.f).lock();
		mStartButton = AddMenuButton(*column, "Start", 25);
		mQuitButton = AddMenuButton(*column, "Quit", 20);
		column->SetSpacing(100.f - mStartButton.lock()->GetIntrinsicSize().y);
		columnLayout.offset.y = -mStartButton.lock()->GetIntrinsicSize().y * .5f;
		column->SetLayout(columnLayout);
	}

	void MainMenuHUD::Init(sf::RenderWindow& windowRef)
	{
		static_cast<void>(windowRef);
		if (auto button = mStartButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &MainMenuHUD::StartButtonClicked);
		if (auto button = mQuitButton.lock()) button->onButtonClicked.BindAction(GetWeakPtr(), &MainMenuHUD::QuitButtonClicked);
	}

	void MainMenuHUD::StartButtonClicked()
	{
		onStartButtonClicked.Broadcast();
	}

	void MainMenuHUD::QuitButtonClicked()
	{
		onQuitButtonClicked.Broadcast();
	}
}
