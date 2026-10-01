#pragma once

#include <SFML/Graphics/Color.hpp>
#include <array>
#include <string>

namespace ly
{
	enum class UIFontRole { Body, Title, Count };
	enum class UIColorRole { Text, TextMuted, Accent, Positive, Warning, Danger, PanelBackground, GaugeBackground, Count };
	enum class UITextSize { Small, Body, Large, Title, Count };

	struct UIStyle
	{
		std::array<std::string, static_cast<std::size_t>(UIFontRole::Count)> fonts;
		std::array<sf::Color, static_cast<std::size_t>(UIColorRole::Count)> colors;
		std::array<unsigned int, static_cast<std::size_t>(UITextSize::Count)> textSizes;

		const std::string& Font(UIFontRole role) const;
		sf::Color Color(UIColorRole role) const;
		unsigned int TextSize(UITextSize size) const;
		static UIStyle MakeDefault();
		static const UIStyle& Get();
		static void Set(const UIStyle& style);
	};
}
