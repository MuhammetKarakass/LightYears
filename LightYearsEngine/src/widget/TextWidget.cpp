#include "widget/TextWidget.h"

namespace ly
{
	TextWidget::TextWidget(const std::string& textStr, const std::string& fontPath, unsigned int characterSize):
		mFont{AssetManager::GetAssetManager().LoadFont(fontPath)},
		mText{*mFont.get(),textStr,characterSize}
	{
	}
	void TextWidget::SetString(const std::string& newStr)
	{
		mText.setString(newStr);
	}

	void TextWidget::SetTextSize(unsigned int newSize)
	{
		mText.setCharacterSize(newSize);
	}
	void TextWidget::SetFillColor(const sf::Color& color)
	{
		mText.setFillColor(color);
	}
	void TextWidget::Draw(sf::RenderWindow& windowRef)
	{
		windowRef.draw(mText);
	}
	void TextWidget::LocationUpdated(const sf::Vector2f& newLocation)
	{
		mText.setPosition(newLocation);
	}
	void TextWidget::RotationUpdated(float newRotation)
	{
		mText.setRotation(sf::degrees(newRotation));
	}
	
	sf::FloatRect TextWidget::GetBound() const
	{
		return mText.getGlobalBounds();
	}

	void TextWidget::UpdateOrigin(const sf::Vector2f& origin)
	{
		mText.setOrigin(origin);
	}
	void TextWidget::ApplyAlpha(float alpha)
	{
		sf::Color currentColor = mText.getFillColor();
		currentColor.a = static_cast<std::uint8_t>(alpha * 255.f);
		mText.setFillColor(currentColor);
	}
}