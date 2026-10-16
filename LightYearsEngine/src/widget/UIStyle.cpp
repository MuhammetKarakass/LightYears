#include "widget/UIStyle.h"
#include "framework/EngineAssets.h"

namespace ly
{
	namespace
	{
		UIStyle& ActiveStyle()
		{
			static UIStyle style = UIStyle::MakeDefault();
			return style;
		}
	}

	const std::string& UIStyle::Font(UIFontRole role) const
	{
		return fonts[static_cast<std::size_t>(role)];
	}

	sf::Color UIStyle::Color(UIColorRole role) const
	{
		return colors[static_cast<std::size_t>(role)];
	}

	unsigned int UIStyle::TextSize(UITextSize size) const
	{
		return textSizes[static_cast<std::size_t>(size)];
	}

	UIStyle UIStyle::MakeDefault()
	{
		UIStyle style;
		style.fonts = { EngineAssets::Get().font, EngineAssets::Get().font };
		style.colors = {
			sf::Color::White, sf::Color{ 160, 160, 160 }, sf::Color{ 80, 160, 255 },
			sf::Color{ 0, 255, 0 }, sf::Color{ 255, 255, 0 }, sf::Color{ 255, 0, 0 },
			sf::Color{ 35, 35, 35, 220 }, sf::Color{ 128, 128, 128 }
		};
		style.textSizes = { 10, 16, 20, 32 };
		return style;
	}

	const UIStyle& UIStyle::Get()
	{
		return ActiveStyle();
	}

	void UIStyle::Set(const UIStyle& style)
	{
		ActiveStyle() = style;
	}
}
