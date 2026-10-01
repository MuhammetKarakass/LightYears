#include "presentation/hud/warning/GameplayWarningView.h"
#include <utility>

namespace ly
{
	GameplayWarningView::GameplayWarningView(shared_ptr<const GameplayWarningViewModel> viewModel)
		: Panel{ { 1.f, 1.f } }, mViewModel{ std::move(viewModel) }
	{
		SetBackgroundColor(sf::Color::Transparent);
		SetLayout(UILayout::Stretch());
		mText = AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 24);
		if (const shared_ptr<TextWidget> text = mText.lock())
		{
			UILayout layout = UILayout::Anchored(UIAnchor::Top, { 0.f, 54.f });
			layout.pivot = { 0.5f, 0.5f };
			mBaseLayout = layout;
			text->SetLayout(layout);
			text->SetFillColor(sf::Color{ 255, 45, 45, 245 });
			text->SetVisibility(false);
		}
	}

	void GameplayWarningView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel) return;

		if (mWatcher.Consume(mViewModel->revision))
		{
			if (const shared_ptr<TextWidget> text = mText.lock())
			{
				text->SetString(mViewModel->text);
				text->SetVisibility(mViewModel->visible);
				if (mActivation != mViewModel->activation)
				{
					mActivation = mViewModel->activation;
					mAnimTime = 0.f;
				}
			}
		}

		if (!mViewModel->visible) return;
		mAnimTime += deltaTime;
		const GameplayWarningPulse pulse = ComputeGameplayWarningPulse(mAnimTime);
		if (const shared_ptr<TextWidget> text = mText.lock())
		{
			text->SetFillColor(pulse.color);
			UILayout layout = mBaseLayout;
			layout.offset += pulse.shake;
			text->SetLayout(layout);
		}
	}
}
