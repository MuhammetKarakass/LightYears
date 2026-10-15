#pragma once

#include <SFML/Graphics/View.hpp>

namespace vega
{
	class EditorCamera
	{
	public:
		void SetTextureSize(sf::Vector2u size);
		sf::View GetView() const;
		void InitFrom(const sf::View& view);
		void ZoomAt(float factor, sf::Vector2f worldPoint);
		void Pan(sf::Vector2f worldDelta);
		// Fit the longest dimension to 40%; tiny actors stay at least 1:1.
		void Focus(const sf::FloatRect& bounds);
		float GetZoom() const { return mZoom; }

	private:
		sf::Vector2f mCenter{};
		sf::Vector2f mTextureSize{ 1.f, 1.f };
		float mZoom = 1.f;
	};
}
