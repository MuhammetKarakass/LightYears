#include "widget/ValueGauge.h"

namespace ly
{
	ValueGauge::ValueGauge(const sf::Vector2f& size, float initialPercent, const sf::Color& foregroundColor, const sf::Color& backgroundColor) :
		mBarFont{AssetManager::GetAssetManager().LoadFont("SpaceShooterRedux/Bonus/kenvector_future.ttf"),},
		mBarText{*mBarFont},
		mBarFront{ size },
		mBarBack{ size },
		mPercent{ initialPercent },
		mForegroundColor(foregroundColor),
		mBackgroundColor(backgroundColor)
	{
		mBarFront.setFillColor(mForegroundColor);
		mBarBack.setFillColor(mBackgroundColor);
		mBarText.setCharacterSize(20);
		mBarText.setFillColor(sf::Color::White);
		CenterText();  // Text'i ortala
	}

	void ValueGauge::UpdateValue(float value, float maxValue)
	{
		if (maxValue == 0.f)
			return;

		mPercent = std::clamp(value / maxValue, 0.0f, 1.0f);

		mBarText.setString(std::to_string(static_cast<int>(value)) + "/" +
			std::to_string(static_cast<int>(maxValue)));

		sf::Vector2f barSize = mBarBack.getSize();
		mBarFront.setSize({ barSize.x * mPercent, barSize.y });
		
		CenterText();  // Text güncellendikten sonra yeniden ortala
	}
	
	void ValueGauge::SetForegroundColor(const sf::Color& color)
	{
		mForegroundColor = color;
		mBarFront.setFillColor(mForegroundColor);
	}
	
	void ValueGauge::Draw(sf::RenderWindow& windowRef)
	{
		windowRef.draw(mBarBack);
		windowRef.draw(mBarFront);
		windowRef.draw(mBarText);
	}
	
	void ValueGauge::LocationUpdated(const sf::Vector2f& newLocation)
	{
		mBarFront.setPosition(newLocation);
		mBarBack.setPosition(newLocation);

		CenterText();
	}
	
	void ValueGauge::RotationUpdated(float newRotation)
	{
		mBarText.setRotation(sf::degrees(newRotation));
		mBarFront.setRotation(sf::degrees(newRotation));
		mBarBack.setRotation(sf::degrees(newRotation));
	}
	
	sf::FloatRect ValueGauge::GetBound() const
	{
		return mBarBack.getGlobalBounds();
	}

	void ValueGauge::CenterText()
	{
		sf::Vector2f widgetCenter = GetCenterPosition();
		sf::FloatRect textBounds = mBarText.getLocalBounds();
		
		// Text'in origin'ini merkeze ayarla
		mBarText.setOrigin(sf::Vector2f{
			textBounds.position.x + textBounds.size.x / 2.0f,
			textBounds.position.y + textBounds.size.y / 2.0f
		});
		
		// Text'i widget merkezine yerleþtir
		mBarText.setPosition(widgetCenter);
	}

	void ValueGauge::UpdateOrigin(const sf::Vector2f& origin)
	{
		mBarFront.setOrigin(origin);
		mBarBack.setOrigin(origin);
	}
	void ValueGauge::ApplyAlpha(float alpha)
	{
		sf::Color currentFrontColor = mBarFront.getFillColor();
		sf::Color currentBackColor = mBarBack.getFillColor();
		sf::Color currentTextColor = mBarText.getFillColor();

		currentFrontColor.a = static_cast<std::uint8_t>(alpha * 255.f);
		currentBackColor.a = static_cast<std::uint8_t>(alpha * 255.f);
		currentTextColor.a = static_cast<std::uint8_t>(alpha * 255.f);

		mBarFront.setFillColor(currentFrontColor);
		mBarBack.setFillColor(currentBackColor);
		mBarText.setFillColor(currentTextColor);

	}
}