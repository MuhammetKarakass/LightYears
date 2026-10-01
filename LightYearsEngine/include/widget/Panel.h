#pragma once

#include "widget/Widget.h"

namespace ly
{
	class Panel : public Widget
	{
	public:
		explicit Panel(const sf::Vector2f& size = { 0.f, 0.f });
		void SetPanelSize(const sf::Vector2f& size);
		void SetBackgroundColor(const sf::Color& color);
		sf::FloatRect GetBound() const override;
		sf::Vector2f GetIntrinsicSize() const override;

	protected:
		void UpdateOrigin(const sf::Vector2f& origin) override;
		void ApplyAlpha(float alpha) override;
		void Draw(sf::RenderWindow& windowRef) override;
		void LocationUpdated(const sf::Vector2f& newLocation) override;
		void RotationUpdated(float newRotation) override;
		void PlaceAt(const UIRect& rect) override;
		void ArrangeChildren(const UIRect& selfRect, bool force) override;

	private:
		sf::Vector2f mSize;
		sf::RectangleShape mBackground;
		sf::Color mBackgroundColor{ sf::Color::Transparent };
	};
}
