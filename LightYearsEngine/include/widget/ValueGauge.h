#pragma once

#include "widget/Widget.h"
#include "framework/AssetManager.h"

namespace ly
{
	class ValueGauge : public Widget
	{
	public:
		ValueGauge(const sf::Vector2f& size = sf::Vector2f{ 220.f,30.f }, float initialPercent = 1.f,
			const sf::Color& foregroundColor = sf::Color{128,255,128,255},
			const sf::Color& backgroundColor = sf::Color{128,128,128,255});

		virtual sf::FloatRect GetBound() const override;

		void UpdateValue(float value,float maxValue);
		void SetForegroundColor(const sf::Color& color);

	protected:
		virtual void UpdateOrigin(const sf::Vector2f& origin) override;
		void ApplyAlpha(float alpha) override;
		
	private:
		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual void LocationUpdated(const sf::Vector2f& newLocation) override;
		virtual void RotationUpdated(float newRotation) override;

		void CenterText();

		shared_ptr<sf::Font> mBarFont;
		sf::Text mBarText;

		sf::RectangleShape mBarFront;
		sf::RectangleShape mBarBack;

		sf::Color mForegroundColor;
		sf::Color mBackgroundColor;

		float mPercent;
	};
}