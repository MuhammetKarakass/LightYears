#include "presentation/hud/evolveChoice/EvolveChoiceView.h"
#include <sstream>

namespace vega
{
	namespace
	{
		std::string WrapDescription(const std::string& description)
		{
			std::istringstream input{ description };
			std::string text, word;
			std::size_t width = 0;
			while (input >> word)
			{
				if (width && width + word.size() + 1 > 76) { text += '\n'; width = 0; }
				if (width) { text += ' '; ++width; }
				text += word; width += word.size();
			}
			return text;
		}
	}

	EvolveChoiceView::EvolveChoiceView(EngineContext& context, std::shared_ptr<const EvolveChoiceViewModel> viewModel) : Panel{ context, { 1.f, 1.f } }, mViewModel{ std::move(viewModel) }
	{
		SetLayout(UILayout::Stretch());
		SetBackgroundColor({ 10, 14, 24, 205 });
		mTitle = AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 24);
		mTitle.lock()->SetLayout(UILayout::Anchored(UIAnchor::Top, { 0.f, 100.f }));
		for (std::size_t index = 0; index < mOptions.size(); ++index)
		{
			mOptions[index] = AddChild<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 18);
			mOptions[index].lock()->SetLayout(UILayout::Anchored(UIAnchor::Top, { 0.f, 210.f + 130.f * static_cast<float>(index) }));
		}
		auto hint = AddChild<TextWidget>("1-3: choose    V / Esc: close    The game keeps running", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 16);
		hint.lock()->SetLayout(UILayout::Anchored(UIAnchor::Bottom, { 0.f, -80.f }));
		SetVisibility(false);
	}

	void EvolveChoiceView::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		if (!mViewModel || !mWatcher.Consume(mViewModel->revision)) return;
		SetVisibility(mViewModel->open);
		if (auto title = mTitle.lock()) title->SetString("EVOLVE - " + mViewModel->abilityName + " - LEVEL " + std::to_string(mViewModel->tierLevel));
		for (std::size_t index = 0; index < mOptions.size(); ++index)
			if (auto text = mOptions[index].lock())
			{
				text->SetVisibility(index < mViewModel->options.size());
				if (index < mViewModel->options.size()) text->SetString(std::to_string(index + 1) + "  " + mViewModel->options[index].name + "\n" + WrapDescription(mViewModel->options[index].description));
			}
	}
}
