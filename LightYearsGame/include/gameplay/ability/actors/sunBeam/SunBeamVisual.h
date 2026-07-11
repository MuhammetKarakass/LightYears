#pragma once

#include "gameConfigs/AbilityVisualStructs.h"

#include <SFML/Graphics.hpp>
#include <optional>

namespace ly
{
	struct SunBeamColumnVisualState
	{
		float topWidthScale = 0.18f;
		float intensity = 1.f;
	};

	struct SunBeamGroundGlowVisualState
	{
		float scale = 1.f;
		float intensity = 1.f;
	};

	struct SunBeamRadialPulseVisualState
	{
		float progress = 0.f;
		float intensity = 1.f;
	};

	enum class SunBeamOverheadVisualStage
	{
		Arrival,
		Impact
	};

	struct SunBeamOverheadVisualState
	{
		SunBeamOverheadVisualStage stage = SunBeamOverheadVisualStage::Arrival;
		float progress = 0.f;
		float intensity = 1.f;
	};

	struct SunBeamVisualFrame
	{
		std::optional<SunBeamColumnVisualState> column;
		std::optional<SunBeamOverheadVisualState> overhead;
		std::optional<SunBeamGroundGlowVisualState> groundGlow;
		std::optional<SunBeamRadialPulseVisualState> radialPulse;
	};

	class SunBeamVisual
	{
	public:
		void Configure(
			const SunBeamVisualDefinition& definition,
			float beamWidth,
			float beamLength,
			float impactRadius
		);

		void Draw(
			sf::RenderTarget& target,
			const sf::Vector2f& impactLocation,
			const SunBeamVisualFrame& frame,
			float age
		);

	private:
		void ConfigureColumnGeometry(float topWidthScale);
		void ConfigurePerspectiveShaftGeometry(
			const sf::Vector2f& sourceLocation,
			const sf::Vector2f& headLocation,
			float columnWidthScale
		);

		SunBeamVisualDefinition mDefinition;
		sf::ConvexShape mOuterColumn;
		sf::ConvexShape mCoreColumn;
		sf::CircleShape mGroundGlow;
		sf::CircleShape mImpactRing;
		sf::CircleShape mOverheadOuter;
		sf::CircleShape mOverheadCore;
		sf::CircleShape mConvergenceHalo;
		sf::RectangleShape mHorizontalGlare;
		sf::RectangleShape mVerticalGlare;
		sf::VertexArray mPerspectiveOuterShaft{ sf::PrimitiveType::TriangleStrip, 4 };
		sf::VertexArray mPerspectiveCoreShaft{ sf::PrimitiveType::TriangleStrip, 4 };
		float mBeamWidth = 1.f;
		float mVisibleBeamLength = 1.f;
		float mImpactRadius = 1.f;
	};
}
