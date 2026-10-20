#include "presentation/hud/modules/HoloCanvas.h"

namespace vega
{
	HoloCanvas::HoloCanvas(EngineContext& context) : Panel{ context, { 0.f, 0.f } }
	{
	}

	void HoloCanvas::Clear()
	{
		mNormal.clear();
		mAdditive.clear();
	}

	void HoloCanvas::Draw(sf::RenderTarget& windowRef)
	{
		sf::RenderStates normal;
		normal.transform.translate(GetResolvedRect().position);
		sf::RenderStates additive = normal;
		additive.blendMode = sf::BlendAdd;
		if (mNormal.getVertexCount() > 0) windowRef.draw(mNormal, normal);
		if (mAdditive.getVertexCount() > 0) windowRef.draw(mAdditive, additive);
	}
}
