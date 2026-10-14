#include "editor/EditorViewport.h"

#include <stdexcept>

namespace vega
{
	sf::Vector2i MapWindowToTexturePixel(sf::Vector2i windowPixel, const sf::FloatRect& imageRect, sf::Vector2u textureSize)
	{
		// A hidden/collapsed panel has no drawable rectangle and must not divide by zero.
		if (imageRect.size.x <= 0.f || imageRect.size.y <= 0.f)
		{
			return {};
		}
		// Keep outside pixels outside so aiming does not stick to the image edges.
		return {
			static_cast<int>((static_cast<float>(windowPixel.x) - imageRect.position.x) * static_cast<float>(textureSize.x) / imageRect.size.x),
			static_cast<int>((static_cast<float>(windowPixel.y) - imageRect.position.y) * static_cast<float>(textureSize.y) / imageRect.size.y)
		};
	}

	EditorViewport::EditorViewport(sf::RenderWindow& window)
		: mWindow{ window }, mTexture{ window.getSize() }
	{
		mTexture.setSmooth(true);
	}

	sf::RenderTarget& EditorViewport::GetRenderTarget()
	{
		return mTexture;
	}

	const sf::RenderTarget& EditorViewport::GetRenderTarget() const
	{
		return mTexture;
	}

	sf::Vector2u EditorViewport::GetSize() const
	{
		return mTexture.getSize();
	}

	sf::Vector2i EditorViewport::GetMousePixelPosition() const
	{
		return MapWindowToTexturePixel(sf::Mouse::getPosition(mWindow), mImageRect, mTexture.getSize());
	}

	sf::Vector2f EditorViewport::MapPixelToCoords(sf::Vector2i pixel, const sf::View& view) const
	{
		return mTexture.mapPixelToCoords(pixel, view);
	}

	sf::View EditorViewport::GetDefaultView() const
	{
		return mTexture.getDefaultView();
	}

	bool EditorViewport::HasInputFocus() const
	{
		return mWindow.hasFocus() && HasPanelInputFocus();
	}

	sf::RenderTexture& EditorViewport::GetTexture()
	{
		return mTexture;
	}

	void EditorViewport::ResizeToWindow()
	{
		const sf::Vector2u size = mWindow.getSize();
		// Panel docking only scales the image; the game pipeline composes at window resolution.
		if (size.x > 0u && size.y > 0u && size != mTexture.getSize())
		{
			if (!mTexture.resize(size))
			{
				throw std::runtime_error("Cannot resize the editor viewport render texture");
			}
			mTexture.setSmooth(true);
		}
	}

	void EditorViewport::SetImagePresentation(const sf::FloatRect& imageRect, bool hovered, bool focused)
	{
		mImageRect = imageRect;
		mHovered = hovered;
		mFocused = focused;
	}

	bool EditorViewport::HasPanelInputFocus() const
	{
		return mHovered || mFocused;
	}
}
