#include "editor/EditorCamera.h"
#include <algorithm>
#include <cmath>

namespace vega
{
	void EditorCamera::SetTextureSize(sf::Vector2u size)
	{
		if (size.x && size.y) mTextureSize = sf::Vector2f{ size };
	}

	sf::View EditorCamera::GetView() const
	{
		return sf::View{ mCenter, mTextureSize * mZoom };
	}

	void EditorCamera::InitFrom(const sf::View& view)
	{
		mCenter = view.getCenter();
		mZoom = std::clamp(view.getSize().x / mTextureSize.x, 0.05f, 20.f);
	}

	void EditorCamera::ZoomAt(float factor, sf::Vector2f worldPoint)
	{
		if (!std::isfinite(factor) || factor <= 0.f) return;
		const float zoom = std::clamp(mZoom * factor, 0.05f, 20.f);
		// Use the clamped ratio so the anchor stays fixed even at the zoom limits.
		mCenter = worldPoint + (mCenter - worldPoint) * (zoom / mZoom);
		mZoom = zoom;
	}

	void EditorCamera::Pan(sf::Vector2f worldDelta)
	{
		mCenter += worldDelta;
	}

	void EditorCamera::Focus(const sf::FloatRect& bounds)
	{
		mCenter = bounds.position + bounds.size * 0.5f;
		mZoom = std::clamp(std::max(bounds.size.x / (mTextureSize.x * 0.4f),
			bounds.size.y / (mTextureSize.y * 0.4f)), 1.f, 20.f);
	}
}
