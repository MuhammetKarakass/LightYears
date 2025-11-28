#include "widget/Button.h"

namespace ly
{
	Button::Button(const std::string& textString, const std::string& texturePath, const std::string& fontPath):
		mButtonTexture{AssetManager::GetAssetManager().LoadTexture(texturePath)},
		mButtonSprite{*mButtonTexture},
		mButtonFont{AssetManager::GetAssetManager().LoadFont(fontPath)},
		mButtonText{*mButtonFont, textString},
		mButtonDefaultColor{128,128,128,255},
		mButtonClickedColor{64,64,64,255},
		mButtonHoveredColor{192,192,192,255},
		mButtonClicked{false}
	{
		mButtonSprite.setColor(mButtonDefaultColor);
	}
	
	sf::FloatRect Button::GetBound() const
	{
		return mButtonSprite.getGlobalBounds();
	}

	bool Button::HandleEvent(const sf::Event& event)
	{
		bool handled = false;

		if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
		{
			if (mouseReleased->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f mousePos(static_cast<float>(mouseReleased->position.x),
					static_cast<float>(mouseReleased->position.y));

				if (GetBound().contains(mousePos) && mButtonClicked)
				{
					onButtonClicked.Broadcast();
					handled = true;
				}
				ButtonDefault();
			}
		}

		else if (const auto* mouseClicked = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if(mouseClicked->button == sf::Mouse::Button::Left)
{
				sf::Vector2f mousePos(static_cast<float>(mouseClicked->position.x),
					static_cast<float>(mouseClicked->position.y));
				if (GetBound().contains(mousePos))
				{
					ButtonClicked();
					handled = true;
				}
			}
		}

		else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
		{
			if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
			{
				sf::Vector2f mousePos(static_cast<float>(mouseMoved->position.x),
					static_cast<float>(mouseMoved->position.y));
				if (GetBound().contains(mousePos))
				{
					ButtonHovered();
					
				}

				else 
				{
					ButtonDefault();
				}

				handled = true;
			}
		}

		return handled || Widget::HandleEvent(event);
	}
	
	void Button::Draw(sf::RenderWindow& windowRef)
	{
		windowRef.draw(mButtonSprite);
		windowRef.draw(mButtonText);
	}
	
	void Button::LocationUpdated(const sf::Vector2f& newLocation)
	{
		mButtonSprite.setPosition(newLocation);
		CenterText();
	}
	
	void Button::RotationUpdated(float newRotation)
	{
		mButtonSprite.setRotation(sf::degrees(newRotation));
		mButtonText.setRotation(sf::degrees(newRotation));
	}

	void Button::ButtonDefault()
	{
		mButtonClicked = false;
		mButtonSprite.setColor(mButtonDefaultColor);
	}

	void Button::ButtonClicked()
	{
		mButtonClicked = true;
		mButtonSprite.setColor(mButtonClickedColor);
	}

	void Button::ButtonHovered()
	{
		mButtonSprite.setColor(mButtonHoveredColor);
	}

	void Button::CenterText()
	{
		sf::Vector2f widgetCenter = GetCenterPosition();
		sf::FloatRect textBounds = mButtonText.getLocalBounds();

		// Text'in origin'ini merkeze ayarla
		mButtonText.setOrigin(sf::Vector2f{
			textBounds.position.x + textBounds.size.x / 2.0f,
			textBounds.position.y + textBounds.size.y / 2.0f
			});

		// Text'i widget merkezine yerleþtir
		mButtonText.setPosition(widgetCenter);
	}

	void Button::SetButtonText(const std::string& text)
	{
		mButtonText.setString(text);
		CenterText();
	}

	void Button::SetTextSize(unsigned int size)
	{
		mButtonText.setCharacterSize(size);
		CenterText();
	}

	void Button::UpdateOrigin(const sf::Vector2f& origin)
	{
		mButtonSprite.setOrigin(origin);
	}
}