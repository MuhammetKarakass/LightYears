#include "framework/AssetManager.h"
#include "widget/ImageWidget.h"

namespace ly
{
	ImageWidget::ImageWidget(const std::string& imagePath):
		mTexture{AssetManager::GetAssetManager().LoadTexture(imagePath)},
		mSprite{*mTexture}
	{
	}
	void ImageWidget::SetImage(const shared_ptr<sf::Texture>& texture)
	{
		if (!texture)
			return;
		mTexture = texture;
		mSprite.setTexture(*mTexture);
		InvalidateLayout();
	}

	sf::FloatRect ImageWidget::GetBound() const
	{
		return mSprite.getGlobalBounds();
	}

	void ImageWidget::LocationUpdated(const sf::Vector2f& newLocation)
	{
		mSprite.setPosition(newLocation);
	}
	void ImageWidget::RotationUpdated(float newRotation)
	{
		mSprite.setRotation(sf::degrees(newRotation));
	}
	void ImageWidget::Draw(sf::RenderWindow& windowRef)
	{
		windowRef.draw(mSprite);
	}

	void ImageWidget::UpdateOrigin(const sf::Vector2f& origin)
	{
		mSprite.setOrigin(origin);
	}
	void ImageWidget::ApplyAlpha(float alpha)
	{
		sf::Color color = mSprite.getColor();
		color.a = static_cast<std::uint8_t>(alpha * 255.f);
		mSprite.setColor(color);
	}
}
