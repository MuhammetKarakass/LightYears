#pragma once

#include "widget/Panel.h"
#include <SFML/Graphics.hpp>

namespace vega
{
	// Retained vertex canvas for the module screen chrome: gradients, hairlines and glows in local (parent-relative) coordinates.
	class HoloCanvas : public Panel
	{
	public:
		HoloCanvas();
		void Clear();
		sf::VertexArray& Normal() { return mNormal; }
		sf::VertexArray& Additive() { return mAdditive; }

	protected:
		void Draw(sf::RenderTarget& windowRef) override;

	private:
		sf::VertexArray mNormal{ sf::PrimitiveType::Triangles };
		sf::VertexArray mAdditive{ sf::PrimitiveType::Triangles };
	};
}
