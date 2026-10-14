#pragma once

#include <framework/RenderViewport.h>

namespace vega
{
	// Shared with headless tests because constructing a RenderTexture acquires an OpenGL context.
	sf::Vector2i MapWindowToTexturePixel(sf::Vector2i windowPixel, const sf::FloatRect& imageRect, sf::Vector2u textureSize);

	class EditorViewport final : public ly::RenderViewport
	{
	public:
		explicit EditorViewport(sf::RenderWindow& window);

		sf::RenderTarget& GetRenderTarget() override;
		const sf::RenderTarget& GetRenderTarget() const override;
		sf::Vector2u GetSize() const override;
		sf::Vector2i GetMousePixelPosition() const override;
		sf::Vector2f MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const override;
		sf::View GetDefaultView() const override;
		bool HasInputFocus() const override;

		sf::RenderTexture& GetTexture();
		void ResizeToWindow();
		void SetImagePresentation(const sf::FloatRect& imageRect, bool hovered, bool focused);
		bool HasPanelInputFocus() const;

	private:
		sf::RenderWindow& mWindow;
		sf::RenderTexture mTexture;
		sf::FloatRect mImageRect{};
		bool mHovered = false;
		bool mFocused = false;
	};
}
