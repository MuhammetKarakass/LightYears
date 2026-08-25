#pragma once

#include "framework/Core.h"

#include <SFML/System/Vector2.hpp>

#include <cstddef>

namespace ly
{
	// A cast-local radial boundary. The same resolver is used by gameplay and
	// rendering so the visible storm edge is always the real damage edge.
	class IonStormBoundary final
	{
	public:
		static IonStormBoundary Generate(
			int controlPointCount,
			float innerCoreRadius,
			float outerMinRadius,
			float outerMaxRadius
		);

		float GetBoundaryRadius(float angleRadians) const;
		bool Contains(const sf::Vector2f& offset) const;
		List<sf::Vector2f> BuildBoundaryPoints(std::size_t pointCount) const;

		float GetInnerCoreRadius() const { return mInnerCoreRadius; }
		float GetOuterMinRadius() const { return mOuterMinRadius; }
		float GetOuterMaxRadius() const { return mOuterMaxRadius; }
		std::size_t GetControlPointCount() const { return mControlRadii.size(); }

	private:
		float mInnerCoreRadius = 250.f;
		float mOuterMinRadius = 250.f;
		float mOuterMaxRadius = 335.f;
		List<float> mControlRadii;
	};
}
