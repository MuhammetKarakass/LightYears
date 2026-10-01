#include "widget/Panel.h"

namespace ly
{
	Panel::Panel(const sf::Vector2f& size) : mSize{ size }, mBackground{ size }
	{
		mBackground.setFillColor(mBackgroundColor);
	}

	void Panel::SetPanelSize(const sf::Vector2f& size)
	{
		if (mSize == size) return;
		mSize = size;
		mBackground.setSize(size);
		InvalidateLayout();
	}

	void Panel::SetBackgroundColor(const sf::Color& color)
	{
		mBackgroundColor = color;
		mBackground.setFillColor(sf::Color{ color.r, color.g, color.b, static_cast<std::uint8_t>(color.a * GetEffectiveAlpha()) });
	}

	sf::FloatRect Panel::GetBound() const
	{
		return mBackground.getGlobalBounds();
	}

	void Panel::UpdateOrigin(const sf::Vector2f& origin)
	{
		mBackground.setOrigin(origin);
	}

	void Panel::LocationUpdated(const sf::Vector2f& newLocation)
	{
		mBackground.setPosition(newLocation);
	}

	void Panel::RotationUpdated(float newRotation)
	{
		mBackground.setRotation(sf::degrees(newRotation));
	}

	void Panel::PlaceAt(const UIRect& rect)
	{
		mBackground.setSize(rect.size);
		Widget::PlaceAt(rect);
	}

	void Panel::ArrangeChildren(const UIRect& selfRect, bool force)
	{
		Widget::ArrangeChildren(selfRect, force);
	}

	void Panel::ApplyAlpha(float alpha)
	{
		mBackground.setFillColor(sf::Color{ mBackgroundColor.r, mBackgroundColor.g, mBackgroundColor.b, static_cast<std::uint8_t>(mBackgroundColor.a * alpha) });
	}

	sf::Vector2f Panel::GetIntrinsicSize() const
	{
		return mSize;
	}

	void Panel::Draw(sf::RenderWindow& windowRef)
	{
		if (mBackgroundColor.a > 0) windowRef.draw(mBackground);
	}
}
