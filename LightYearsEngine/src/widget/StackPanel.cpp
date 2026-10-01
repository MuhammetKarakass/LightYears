#include "widget/StackPanel.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	sf::Vector2f StackPanel::ChildSize(const Widget& child)
	{
		sf::Vector2f size = child.GetIntrinsicSize();
		if (child.HasLayout())
		{
			const sf::Vector2f explicitSize = child.GetLayout().size;
			if (explicitSize.x > 0.f) size.x = explicitSize.x;
			if (explicitSize.y > 0.f) size.y = explicitSize.y;
		}
		return size;
	}
	StackPanel::StackPanel(UIOrientation orientation, float spacing) : mOrientation{ orientation }, mSpacing{ spacing }
	{
	}

	void StackPanel::SetSpacing(float spacing)
	{
		if (mSpacing == spacing) return;
		mSpacing = spacing;
		InvalidateLayout();
	}

	void StackPanel::SetPadding(const sf::Vector2f& padding)
	{
		if (mPadding == padding) return;
		mPadding = padding;
		InvalidateLayout();
	}

	void StackPanel::SetCrossAlign(UIAlign align)
	{
		if (mCrossAlign == align) return;
		mCrossAlign = align;
		InvalidateLayout();
	}

	sf::Vector2f StackPanel::GetIntrinsicSize() const
	{
		float main = 0.f, cross = 0.f;
		std::size_t count = 0;
		for (const auto& child : GetChildren())
		{
			if (!child || child->IsExpired() || !child->GetVisibility()) continue;
			const sf::Vector2f size = ChildSize(*child);
			main += mOrientation == UIOrientation::Horizontal ? size.x : size.y;
			cross = std::max(cross, mOrientation == UIOrientation::Horizontal ? size.y : size.x);
			++count;
		}
		if (count > 1) main += mSpacing * static_cast<float>(count - 1);
		return mOrientation == UIOrientation::Horizontal ? sf::Vector2f{ main + mPadding.x * 2.f, cross + mPadding.y * 2.f } : sf::Vector2f{ cross + mPadding.x * 2.f, main + mPadding.y * 2.f };
	}

	void StackPanel::ArrangeChildren(const UIRect& selfRect, bool force)
	{
		const bool horizontal = mOrientation == UIOrientation::Horizontal;
		float cursor = horizontal ? selfRect.position.x + mPadding.x : selfRect.position.y + mPadding.y;
		const float availableCross = (horizontal ? selfRect.size.y - mPadding.y * 2.f : selfRect.size.x - mPadding.x * 2.f);
		std::size_t visibleCount = 0;
		for (const auto& child : GetChildren()) if (child && !child->IsExpired() && child->GetVisibility()) ++visibleCount;
		std::size_t placed = 0;
		const List<shared_ptr<Widget>>& children = GetChildren();
		for (std::size_t i = 0; i < children.size(); ++i)
		{
			const shared_ptr<Widget> child = children[i];
			if (!child || child->IsExpired() || !child->GetVisibility()) continue;
			const sf::Vector2f size = ChildSize(*child);
			const sf::Vector2f offset = child->HasLayout() ? child->GetLayout().offset : sf::Vector2f{};
			float cross = horizontal ? selfRect.position.y + mPadding.y : selfRect.position.x + mPadding.x;
			if (mCrossAlign == UIAlign::Center) cross += (availableCross - (horizontal ? size.y : size.x)) * .5f;
			else if (mCrossAlign == UIAlign::End) cross += availableCross - (horizontal ? size.y : size.x);
			UIRect childRect;
			childRect.size = size;
			childRect.position = horizontal ? sf::Vector2f{ cursor + offset.x, cross + offset.y } : sf::Vector2f{ cross + offset.x, cursor + offset.y };
			// Match ResolveLayout: whole-pixel positions keep text and sprites crisp.
			childRect.position = { std::round(childRect.position.x), std::round(childRect.position.y) };
			ArrangeChildAt(*child, childRect, force);
			cursor += (horizontal ? size.x : size.y) + (placed + 1 < visibleCount ? mSpacing : 0.f);
			++placed;
		}
	}
}
