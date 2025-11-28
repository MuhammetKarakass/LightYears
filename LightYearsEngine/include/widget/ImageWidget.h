#pragma once

#include "widget/Widget.h"

namespace ly
{
	class ImageWidget : public Widget
	{
	public:
		ImageWidget(const std::string& imagePath);

		void SetImage(const shared_ptr<sf::Texture>& texture);
		virtual sf::FloatRect GetBound() const override;
		
	protected:
		virtual void UpdateOrigin(const sf::Vector2f& origin) override;
		
	private:
		virtual void LocationUpdated(const sf::Vector2f& newLocation) override;
		virtual void RotationUpdated(float newRotation) override;
		virtual void Draw(sf::RenderWindow& windowRef) override;

		shared_ptr<sf::Texture> mTexture;
		sf::Sprite mSprite;
	};
}