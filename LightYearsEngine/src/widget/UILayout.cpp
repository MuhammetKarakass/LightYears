#include "widget/UILayout.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sf::Vector2f AnchorValue(UIAnchor anchor)
		{
			switch (anchor)
			{
			case UIAnchor::TopLeft: return { 0.f, 0.f };
			case UIAnchor::Top: return { 0.5f, 0.f };
			case UIAnchor::TopRight: return { 1.f, 0.f };
			case UIAnchor::Left: return { 0.f, 0.5f };
			case UIAnchor::Center: return { 0.5f, 0.5f };
			case UIAnchor::Right: return { 1.f, 0.5f };
			case UIAnchor::BottomLeft: return { 0.f, 1.f };
			case UIAnchor::Bottom: return { 0.5f, 1.f };
			case UIAnchor::BottomRight: return { 1.f, 1.f };
			}
			return { 0.f, 0.f };
		}

		float ResolveAxis(float parentPosition, float parentSize, float anchorMin, float anchorMax, float pivot, float offset, float size, float intrinsicSize, float insetMin, float insetMax, float& extent)
		{
			if (anchorMin == anchorMax)
			{
				extent = size > 0.f ? size : intrinsicSize;
				return parentPosition + parentSize * anchorMin + offset - pivot * extent;
			}

			extent = std::max(0.f, parentSize * (anchorMax - anchorMin) - insetMin - insetMax);
			return parentPosition + parentSize * anchorMin + insetMin;
		}
	}

	UILayout UILayout::Anchored(UIAnchor anchor, const sf::Vector2f& offset, const sf::Vector2f& size)
	{
		const sf::Vector2f value = AnchorValue(anchor);
		return { value, value, value, offset, size, { 0.f, 0.f }, { 0.f, 0.f } };
	}

	UILayout UILayout::Stretch(const sf::Vector2f& insetMin, const sf::Vector2f& insetMax)
	{
		return { { 0.f, 0.f }, { 1.f, 1.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f }, insetMin, insetMax };
	}

	UIRect ResolveLayout(const UILayout& layout, const UIRect& parent, const sf::Vector2f& intrinsicSize)
	{
		UIRect result;
		result.position.x = ResolveAxis(parent.position.x, parent.size.x, layout.anchorMin.x, layout.anchorMax.x, layout.pivot.x, layout.offset.x, layout.size.x, intrinsicSize.x, layout.insetMin.x, layout.insetMax.x, result.size.x);
		result.position.y = ResolveAxis(parent.position.y, parent.size.y, layout.anchorMin.y, layout.anchorMax.y, layout.pivot.y, layout.offset.y, layout.size.y, intrinsicSize.y, layout.insetMin.y, layout.insetMax.y, result.size.y);
		result.position.x = std::round(result.position.x);
		result.position.y = std::round(result.position.y);
		return result;
	}
}
