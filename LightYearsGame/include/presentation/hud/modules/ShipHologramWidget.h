#pragma once

#include "presentation/hud/modules/HoloDraw.h"
#include "widget/Panel.h"
#include <vector>

namespace vega
{
	struct HologramModule
	{
		std::vector<sf::Vector2i> cells;
		sf::Color color{ 120, 200, 255 };
		bool held{ false };
		bool rebooting{ false };
		bool hovered{ false };
		bool system{ false };
		float rebootFraction{ 0.f };
	};

	struct HologramData
	{
		int width{ 0 };
		int height{ 0 };
		// Screen scale of the 64-unit cell space (the view shrinks big grids to fit its area).
		float scale{ 1.f };
		// Row-major, width * height. Inactive cells are void (no slot).
		std::vector<sf::Color> zoneColors;
		std::vector<char> active;
		std::vector<char> engine;
		// Optional (same size as active): locked cells are drawn dim with a padlock and take no modules.
		std::vector<char> locked;
		std::vector<HologramModule> modules;
		bool previewActive{ false };
		bool previewLegal{ false };
		std::vector<sf::Vector2i> previewCells;
	};

	// Animated holographic ship projected above a table: hull silhouette, zone light, module plates, preview and projector.
	// Positioned like the schematic grid: cell (0,0) sits at the widget's resolved position, one cell per ModuleScreenView::CellPitch.
	class ShipHologramWidget : public Panel
	{
	public:
		ShipHologramWidget(EngineContext& context);
		void SetData(const HologramData& data);

	protected:
		void Tick(float deltaTime) override;
		void Draw(sf::RenderTarget& windowRef) override;

	private:
		struct RowSpan
		{
			int row{ 0 };
			float left{ 0.f };
			float right{ 0.f };
			float top{ 0.f };
			float bottom{ 0.f };
			// Span used for the outline only: a row narrower than both neighbours is widened so the silhouette has no inward notch.
			float outlineLeft{ 0.f };
			float outlineRight{ 0.f };
		};

		void BuildHull();
		void BuildStatic();
		bool SpanAt(float y, float& left, float& right) const;
		bool IsActive(int x, int y) const;

		HologramData mData;
		std::vector<RowSpan> mRows;
		holo::Pts mLeftProfile;
		holo::Pts mRightProfile;
		holo::Pts mOutline;
		std::vector<holo::Vec> mFlameRoots;
		float mHullTop{ 0.f };
		float mHullBottom{ 0.f };
		float mHullLeft{ 0.f };
		float mHullRight{ 0.f };
		// Geometry that only changes with SetData, built once instead of every frame.
		sf::VertexArray mStaticBody{ sf::PrimitiveType::Triangles };
		sf::VertexArray mStaticOver{ sf::PrimitiveType::Triangles };
		sf::VertexArray mHoles{ sf::PrimitiveType::Triangles };
		sf::VertexArray mPlates{ sf::PrimitiveType::Triangles };
		sf::VertexArray mAnimatedUnder{ sf::PrimitiveType::Triangles };
		sf::VertexArray mAnimatedBody{ sf::PrimitiveType::Triangles };
		sf::VertexArray mAnimatedOver{ sf::PrimitiveType::Triangles };
		float mTime{ 0.f };
	};
}
