#include "framework/RenderViewport.h"

namespace vega
{
	WindowRenderViewport::WindowRenderViewport(sf::RenderWindow& window)
		: mWindow{ window }
	{
	}

	sf::RenderTarget& WindowRenderViewport::GetRenderTarget()
	{
		return mWindow;
	}

	const sf::RenderTarget& WindowRenderViewport::GetRenderTarget() const
	{
		return mWindow;
	}

	sf::Vector2u WindowRenderViewport::GetSize() const
	{
		return mWindow.getSize();
	}

	sf::Vector2i WindowRenderViewport::GetMousePixelPosition() const
	{
		return sf::Mouse::getPosition(mWindow);
	}

	sf::Vector2f WindowRenderViewport::MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const
	{
		return mWindow.mapPixelToCoords(pixel, view);
	}

	sf::View WindowRenderViewport::GetDefaultView() const
	{
		return mWindow.getDefaultView();
	}

	bool WindowRenderViewport::HasInputFocus() const
	{
		return mWindow.hasFocus();
	}
}
