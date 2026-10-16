#pragma once

#include "widget/Widget.h"
#include "framework/AssetManager.h"
#include "framework/EngineAssets.h"

namespace ly
{
	class TextWidget : public Widget
	{
		friend struct GameHUDDamageE2ETestAccess;

	public:
		TextWidget(const std::string& textStr, const std::string& fontPath = EngineAssets::Get().font,
			unsigned int characterSize=10);

		void SetString(const std::string& newStr);
		void SetTextSize(unsigned int newSize);
		void SetFillColor(const sf::Color& color);

		virtual sf::FloatRect GetBound() const override;
		
	protected:
		virtual void UpdateOrigin(const sf::Vector2f& origin) override;
		virtual void ApplyAlpha(float alpha) override;

	private:

		virtual void Draw(sf::RenderTarget& windowRef) override;
		virtual void LocationUpdated(const sf::Vector2f& newLocation) override;
		virtual void RotationUpdated(float newRotation) override;

		shared_ptr<sf::Font> mFont;
		sf::Text mText;
		std::uint8_t mAuthoredTextAlpha{ 255 };
	};
}
