#pragma once

#include <SFML/Graphics.hpp>

namespace vega
{
	// The surface a World is presented on: the application window in the game, an editor panel's render
	// texture in the editor. Non-owning users; the owner must outlive every World that uses it.
	class RenderViewport
	{
	public:
		virtual ~RenderViewport() = default;
		virtual sf::RenderTarget& GetRenderTarget() = 0;
		virtual const sf::RenderTarget& GetRenderTarget() const = 0;
		// Size in target pixels.
		virtual sf::Vector2u GetSize() const = 0;
		// Cursor position in target pixels (may lie outside the target).
		virtual sf::Vector2i GetMousePixelPosition() const = 0;
		// Maps a target pixel into world coordinates through the given view.
		virtual sf::Vector2f MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const = 0;
		// Default (unscrolled, unzoomed) view of the surface; the camera derives the world view from it.
		virtual sf::View GetDefaultView() const = 0;
		// False while device input must not be read (unfocused window, editor panel not hovered/focused).
		virtual bool HasInputFocus() const = 0;
	};

	// Presents on an sf::RenderWindow with exactly the calls World used before this interface existed.
	class WindowRenderViewport final : public RenderViewport
	{
	public:
		explicit WindowRenderViewport(sf::RenderWindow& window);

		sf::RenderTarget& GetRenderTarget() override;
		const sf::RenderTarget& GetRenderTarget() const override;
		sf::Vector2u GetSize() const override;
		sf::Vector2i GetMousePixelPosition() const override;
		sf::Vector2f MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const override;
		sf::View GetDefaultView() const override;
		bool HasInputFocus() const override;

	private:
		sf::RenderWindow& mWindow;
	};
}
