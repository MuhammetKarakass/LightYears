#pragma once

#include "widget/Widget.h"
#include "framework/AssetManager.h"

namespace ly
{
	class Button : public Widget
	{
	public:
		Button(const std::string& textString = "Button", 
			const std::string& texturePath = "SpaceShooterRedux/PNG/UI/buttonBlue.png",
			const std::string& fontPath="SpaceShooterRedux/Bonus/kenvector_future.ttf");

		virtual sf::FloatRect GetBound() const override;
		virtual bool HandleEvent(const sf::Event& event) override;
		void SetButtonText(const std::string& text);
		void SetTextSize(unsigned int size);

		Delegate<> onButtonClicked;
		
	protected:
		virtual void UpdateOrigin(const sf::Vector2f& origin) override;
		
	private:
		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual void LocationUpdated(const sf::Vector2f& newLocation) override;
		virtual void RotationUpdated(float newRotation) override;


		void ButtonDefault();
		void ButtonClicked();
		void ButtonHovered();

		void CenterText();

		shared_ptr<sf::Texture> mButtonTexture;
		sf::Sprite mButtonSprite;

		shared_ptr<sf::Font> mButtonFont;
		sf::Text mButtonText;

		sf::Color mButtonDefaultColor;
		sf::Color mButtonClickedColor;
		sf::Color mButtonHoveredColor;

		bool mButtonClicked;
	};
}