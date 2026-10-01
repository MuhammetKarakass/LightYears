#pragma once

#include <SFML/System/Vector2.hpp>

namespace ly
{
	struct UIRect
	{
		sf::Vector2f position{ 0.f, 0.f };
		sf::Vector2f size{ 0.f, 0.f };
	};

	enum class UIAnchor
	{
		TopLeft,
		Top,
		TopRight,
		Left,
		Center,
		Right,
		BottomLeft,
		Bottom,
		BottomRight
	};

	struct UILayout
	{
		sf::Vector2f anchorMin{ 0.f, 0.f };
		sf::Vector2f anchorMax{ 0.f, 0.f };
		sf::Vector2f pivot{ 0.f, 0.f };
		sf::Vector2f offset{ 0.f, 0.f };
		sf::Vector2f size{ 0.f, 0.f };
		sf::Vector2f insetMin{ 0.f, 0.f };
		sf::Vector2f insetMax{ 0.f, 0.f };

		static UILayout Anchored(UIAnchor anchor, const sf::Vector2f& offset = { 0.f, 0.f }, const sf::Vector2f& size = { 0.f, 0.f });
		static UILayout Stretch(const sf::Vector2f& insetMin = { 0.f, 0.f }, const sf::Vector2f& insetMax = { 0.f, 0.f });
	};

	UIRect ResolveLayout(const UILayout& layout, const UIRect& parent, const sf::Vector2f& intrinsicSize);
}
