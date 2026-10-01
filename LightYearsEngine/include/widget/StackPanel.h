#pragma once

#include "widget/Panel.h"

namespace ly
{
	enum class UIOrientation { Horizontal, Vertical };
	enum class UIAlign { Start, Center, End };

	class StackPanel : public Panel
	{
	public:
		explicit StackPanel(UIOrientation orientation, float spacing = 0.f);
		void SetSpacing(float spacing);
		void SetPadding(const sf::Vector2f& padding);
		void SetCrossAlign(UIAlign align);
		sf::Vector2f GetIntrinsicSize() const override;

	protected:
		void ArrangeChildren(const UIRect& selfRect, bool force) override;

	private:
		static sf::Vector2f ChildSize(const Widget& child);
		UIOrientation mOrientation;
		float mSpacing;
		sf::Vector2f mPadding{};
		UIAlign mCrossAlign{ UIAlign::Start };
	};
}
