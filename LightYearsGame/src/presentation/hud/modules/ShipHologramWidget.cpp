#include "presentation/hud/modules/ShipHologramWidget.h"
#include "presentation/hud/modules/ModuleScreenView.h"
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		using namespace holo;

		constexpr float Pitch = ModuleScreenView::CellPitch;
		constexpr float Inset = 3.f;
		constexpr float Inner = Pitch - Inset * 2.f - 2.f;
		constexpr float Bridge = Pitch - Inner - Inset;
		constexpr float Chamfer = 6.f;
		constexpr float NoseHeight = 34.f;
		constexpr float TipOut = 14.f;
		const sf::Color HoloCyan{ 70, 205, 255, 255 };
		const sf::Color RimColor{ 180, 235, 255, 255 };
		const sf::Color WhiteCyan{ 210, 248, 255, 255 };

		bool Has(const std::vector<sf::Vector2i>& cells, int x, int y)
		{
			for (const sf::Vector2i& cell : cells)
				if (cell.x == x && cell.y == y) return true;
			return false;
		}

		float Interp(const Pts& pts, float y)
		{
			if (pts.empty()) return 0.f;
			if (y <= pts.front().y) return pts.front().x;
			for (std::size_t i = 0; i + 1 < pts.size(); ++i)
				if (y <= pts[i + 1].y)
				{
					const float span = pts[i + 1].y - pts[i].y;
					const float t = span > 0.0001f ? (y - pts[i].y) / span : 0.f;
					return pts[i].x + (pts[i + 1].x - pts[i].x) * t;
				}
			return pts.back().x;
		}

		// Segment extended by half its thickness at both ends so joints close.
		void Seg(sf::VertexArray& va, const Vec& a, const Vec& b, float thickness, const sf::Color& color)
		{
			const Vec d = b - a;
			const float len = std::sqrt(d.x * d.x + d.y * d.y);
			if (len < 0.001f) return;
			const Vec u = d / len * (thickness * 0.5f);
			Line(va, a - u, b + u, thickness, color);
		}

		// Filled plate for a set of cells: chamfered outer corners, bridges between neighbours, vertical gradient.
		void AddPlate(sf::VertexArray& va, const std::vector<sf::Vector2i>& cells, const sf::Color& top, const sf::Color& bottom)
		{
			if (cells.empty()) return;
			float y0 = 1e9f, y1 = -1e9f;
			for (const sf::Vector2i& c : cells)
			{
				y0 = std::min(y0, c.y * Pitch + Inset);
				y1 = std::max(y1, c.y * Pitch + Inset + Inner);
			}
			const auto colorAt = [&](float y) { return Lerp(top, bottom, (y - y0) / std::max(1.f, y1 - y0)); };
			for (const sf::Vector2i& c : cells)
			{
				const float x = c.x * Pitch + Inset;
				const float y = c.y * Pitch + Inset;
				const bool l = Has(cells, c.x - 1, c.y), r = Has(cells, c.x + 1, c.y), u = Has(cells, c.x, c.y - 1), d = Has(cells, c.x, c.y + 1);
				const Pts poly = ChamferRect({ x, y }, { Inner, Inner }, (!l && !u) ? Chamfer : 0.f, (!r && !u) ? Chamfer : 0.f, (!r && !d) ? Chamfer : 0.f, (!l && !d) ? Chamfer : 0.f);
				const Vec center{ x + Inner * 0.5f, y + Inner * 0.5f };
				for (std::size_t i = 0; i < poly.size(); ++i)
				{
					const Vec& a = poly[i];
					const Vec& b = poly[(i + 1) % poly.size()];
					Tri(va, center, a, b, colorAt(center.y), colorAt(a.y), colorAt(b.y));
				}
				if (r) Rect(va, { x + Inner, y }, { Bridge, Inner }, colorAt(y), colorAt(y + Inner));
				if (d) Rect(va, { x, y + Inner }, { Inner, Bridge }, colorAt(y + Inner), colorAt(y + Inner + Bridge));
				if (r && d && Has(cells, c.x + 1, c.y + 1)) Rect(va, { x + Inner, y + Inner }, { Bridge, Bridge }, colorAt(y + Inner), colorAt(y + Inner + Bridge));
			}
		}

		// Outline of the merged shape of a cell set (outer boundary only, chamfered convex corners).
		void AddRim(sf::VertexArray& va, const std::vector<sf::Vector2i>& cells, float thickness, const sf::Color& color)
		{
			for (const sf::Vector2i& c : cells)
			{
				const float x0 = c.x * Pitch + Inset;
				const float y0 = c.y * Pitch + Inset;
				const float x1 = x0 + Inner;
				const float y1 = y0 + Inner;
				const bool l = Has(cells, c.x - 1, c.y), r = Has(cells, c.x + 1, c.y), u = Has(cells, c.x, c.y - 1), d = Has(cells, c.x, c.y + 1);
				const bool tl = !l && !u, tr = !r && !u, br = !r && !d, bl = !l && !d;
				const float cut = Chamfer;
				if (!u) Seg(va, { x0 + (tl ? cut : 0.f), y0 }, { x1 - (tr ? cut : 0.f) + ((r && !Has(cells, c.x + 1, c.y - 1)) ? Bridge : 0.f), y0 }, thickness, color);
				if (!d) Seg(va, { x0 + (bl ? cut : 0.f), y1 }, { x1 - (br ? cut : 0.f) + ((r && !Has(cells, c.x + 1, c.y + 1)) ? Bridge : 0.f), y1 }, thickness, color);
				if (!l) Seg(va, { x0, y0 + (tl ? cut : 0.f) }, { x0, y1 - (bl ? cut : 0.f) + ((d && !Has(cells, c.x - 1, c.y + 1)) ? Bridge : 0.f) }, thickness, color);
				if (!r) Seg(va, { x1, y0 + (tr ? cut : 0.f) }, { x1, y1 - (br ? cut : 0.f) + ((d && !Has(cells, c.x + 1, c.y + 1)) ? Bridge : 0.f) }, thickness, color);
				if (tl) Seg(va, { x0, y0 + cut }, { x0 + cut, y0 }, thickness, color);
				if (tr) Seg(va, { x1 - cut, y0 }, { x1, y0 + cut }, thickness, color);
				if (br) Seg(va, { x1, y1 - cut }, { x1 - cut, y1 }, thickness, color);
				if (bl) Seg(va, { x0 + cut, y1 }, { x0, y1 - cut }, thickness, color);
			}
		}

		float Noise(float x)
		{
			const float v = std::sin(x * 12.9898f) * 43758.5453f;
			return v - std::floor(v);
		}
	}

	ShipHologramWidget::ShipHologramWidget() : Panel{ { 0.f, 0.f } }
	{
	}

	void ShipHologramWidget::SetData(const HologramData& data)
	{
		mData = data;
		BuildHull();
		BuildStatic();
	}

	void ShipHologramWidget::Tick(float deltaTime)
	{
		Panel::Tick(deltaTime);
		mTime += deltaTime;
	}

	bool ShipHologramWidget::IsActive(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= mData.width || y >= mData.height) return false;
		return mData.active[static_cast<std::size_t>(y * mData.width + x)] != 0;
	}

	bool ShipHologramWidget::SpanAt(float y, float& left, float& right) const
	{
		if (mLeftProfile.empty() || y < mHullTop || y > mHullBottom) return false;
		left = Interp(mLeftProfile, y);
		right = Interp(mRightProfile, y);
		return true;
	}

	void ShipHologramWidget::BuildHull()
	{
		mRows.clear();
		mLeftProfile.clear();
		mRightProfile.clear();
		mOutline.clear();
		mFlameRoots.clear();
		const std::size_t cellCount = static_cast<std::size_t>(std::max(0, mData.width) * std::max(0, mData.height));
		if (mData.width <= 0 || mData.height <= 0 || mData.active.size() != cellCount || mData.zoneColors.size() != cellCount || mData.engine.size() != cellCount) return;

		for (int y = 0; y < mData.height; ++y)
		{
			int minX = mData.width, maxX = -1;
			for (int x = 0; x < mData.width; ++x)
				if (IsActive(x, y))
				{
					minX = std::min(minX, x);
					maxX = std::max(maxX, x);
				}
			if (maxX >= 0) mRows.push_back({ y, minX * Pitch, (maxX + 1) * Pitch, y * Pitch, (y + 1) * Pitch, minX * Pitch, (maxX + 1) * Pitch });
		}
		if (mRows.empty()) return;
		for (std::size_t i = 1; i + 1 < mRows.size(); ++i)
		{
			mRows[i].outlineLeft = std::min(mRows[i].left, std::max(mRows[i - 1].left, mRows[i + 1].left));
			mRows[i].outlineRight = std::max(mRows[i].right, std::min(mRows[i - 1].right, mRows[i + 1].right));
		}


		const auto buildSide = [&](bool left)
		{
			Pts pts;
			const auto push = [&pts](float x, float y)
			{
				if (!pts.empty() && y <= pts.back().y + 0.01f) return;
				pts.push_back({ x, y });
			};
			const auto edge = [left](const RowSpan& r) { return left ? r.outlineLeft : r.outlineRight; };
			const float dir = left ? -1.f : 1.f;
			const RowSpan& first = mRows.front();
			const float cx = (first.outlineLeft + first.outlineRight) * 0.5f;
			const float half = (first.outlineRight - first.outlineLeft) * 0.5f;
			constexpr int steps = 9;
			for (int i = 0; i <= steps; ++i)
			{
				const float t = static_cast<float>(i) / steps;
				push(cx + dir * half * t * (1.f + 0.35f * (1.f - t)), first.top - NoseHeight * (1.f - t));
			}
			// Swept outline: a row that reaches further out than the one above is joined to it by one straight sweep line
			// (corner to corner), so wings read as swept surfaces instead of a staircase.
			const auto outward = [&](std::size_t i) { return i > 0 && (edge(mRows[i]) - edge(mRows[i - 1])) * dir > 0.5f; };
			const auto inwardNext = [&](std::size_t i) { return i + 1 < mRows.size() && (edge(mRows[i + 1]) - edge(mRows[i])) * dir < -0.5f; };
			// A row that reaches further out than the one above and is followed by a narrower one is a wing tip: the leading
			// edge runs straight into a point at mid-row and the trailing edge sweeps back in.
			for (std::size_t i = 0; i < mRows.size(); ++i)
			{
				const RowSpan& r = mRows[i];
				const float x = edge(r);
				if (outward(i) && inwardNext(i)) push(x + dir * TipOut, r.top + (r.bottom - r.top) * 0.5f);
				else if (i == 0 || outward(i)) push(x, r.top);
				if (i + 1 == mRows.size())
				{
					push(x, r.bottom - 8.f);
					push(x - dir * 8.f, r.bottom);
				}
				else if (!outward(i + 1)) push(x, r.bottom);
			}
			return pts;
		};
		mLeftProfile = buildSide(true);
		mRightProfile = buildSide(false);
		mOutline = mLeftProfile;
		for (std::size_t i = mRightProfile.size(); i-- > 1;) mOutline.push_back(mRightProfile[i]);

		mHullTop = mLeftProfile.front().y;
		mHullBottom = mLeftProfile.back().y;
		mHullLeft = 1e9f;
		mHullRight = -1e9f;
		for (const Vec& p : mOutline)
		{
			mHullLeft = std::min(mHullLeft, p.x);
			mHullRight = std::max(mHullRight, p.x);
		}

		// Flames burn under the engine cells of the bottom row.
		const int bottomRow = mRows.back().row;
		std::vector<int> engineColumns;
		for (int x = 0; x < mData.width; ++x)
			if (IsActive(x, bottomRow) && mData.engine[static_cast<std::size_t>(bottomRow * mData.width + x)] != 0) engineColumns.push_back(x);
		// Symmetric selection: every other column from each outer edge inwards (all of them for up to four).
		std::vector<int> chosen;
		const std::size_t columns = engineColumns.size();
		for (std::size_t i = 0; i < columns; ++i)
		{
			const std::size_t fromEdge = std::min(i, columns - 1 - i);
			if (columns <= 4 || fromEdge % 2 == 0) chosen.push_back(engineColumns[i]);
		}
		for (int x : chosen) mFlameRoots.push_back({ (static_cast<float>(x) + 0.5f) * Pitch, mRows.back().bottom - 3.f });
	}

	void ShipHologramWidget::BuildStatic()
	{
		mStaticBody.clear();
		mStaticOver.clear();
		mHoles.clear();
		mPlates.clear();
		if (mOutline.empty()) return;
		const float pulse = 0.85f;
		const auto fl = [](float a) { return a; };
		{
		std::vector<float> ys;
		for (const Vec& p : mLeftProfile) ys.push_back(p.y);
		for (const Vec& p : mRightProfile) ys.push_back(p.y);
		std::sort(ys.begin(), ys.end());
		ys.erase(std::unique(ys.begin(), ys.end(), [](float a, float b) { return b - a < 0.01f; }), ys.end());
		const auto fillColor = [&](float y) { return Alpha(HoloCyan, fl(34.f + (14.f - 34.f) * Clamp01((y - mHullTop) / std::max(1.f, mHullBottom - mHullTop)))); };
		for (std::size_t i = 0; i + 1 < ys.size(); ++i)
		{
			const float ya = ys[i], yb = ys[i + 1];
			Quad(mStaticBody, { Interp(mLeftProfile, ya), ya }, { Interp(mRightProfile, ya), ya }, { Interp(mRightProfile, yb), yb }, { Interp(mLeftProfile, yb), yb }, fillColor(ya), fillColor(ya), fillColor(yb), fillColor(yb));
		}
		}

		// Zone light: soft wash, corner ticks, dividers.
		const auto zoneAt = [&](int x, int y) { return mData.zoneColors[static_cast<std::size_t>(y * mData.width + x)]; };
		for (int y = 0; y < mData.height; ++y)
			for (int x = 0; x < mData.width; ++x)
			{
				if (!IsActive(x, y)) continue;
				const float px = x * Pitch, py = y * Pitch;
				const sf::Color zone = zoneAt(x, y);
				if (mData.locked.size() == mData.active.size() && mData.locked[static_cast<std::size_t>(y * mData.width + x)] != 0)
				{
					// Locked: the zone's color, dimmed, with hatching and a padlock. No module can go here yet.
					const sf::Color dim = Mul(zone, 0.55f);
					Rect(mStaticBody, { px + 2.f, py + 2.f }, { Pitch - 4.f, Pitch - 4.f }, Alpha(dim, fl(46.f)), Alpha(dim, fl(26.f)));
					const sf::Color rim = Alpha(dim, fl(130.f));
					const float lx0 = px + 2.f, lx1 = px + Pitch - 2.f, ly0 = py + 2.f, ly1 = py + Pitch - 2.f;
					Seg(mStaticBody, { lx0, ly0 }, { lx1, ly0 }, 1.2f, rim);
					Seg(mStaticBody, { lx0, ly1 }, { lx1, ly1 }, 1.2f, rim);
					Seg(mStaticBody, { lx0, ly0 }, { lx0, ly1 }, 1.2f, rim);
					Seg(mStaticBody, { lx1, ly0 }, { lx1, ly1 }, 1.2f, rim);
					for (int k = 1; k <= 3; ++k)
					{
						const float t = static_cast<float>(k) * Pitch * 0.25f;
						Seg(mStaticBody, { px + 2.f, py + t }, { px + t, py + 2.f }, 1.f, Alpha(dim, fl(40.f)));
						Seg(mStaticBody, { px + Pitch - t, py + Pitch - 2.f }, { px + Pitch - 2.f, py + Pitch - t }, 1.f, Alpha(dim, fl(40.f)));
					}
					const float cx = px + Pitch * 0.5f, cy = py + Pitch * 0.5f + 3.f;
					const sf::Color lockColor = Alpha(WhiteCyan, fl(185.f));
					Rect(mStaticBody, { cx - 8.f, cy - 3.f }, { 16.f, 13.f }, lockColor, Alpha(lockColor, fl(140.f)));
					Seg(mStaticBody, { cx - 5.f, cy - 3.f }, { cx - 5.f, cy - 10.f }, 2.f, lockColor);
					Seg(mStaticBody, { cx - 5.f, cy - 10.f }, { cx + 5.f, cy - 10.f }, 2.f, lockColor);
					Seg(mStaticBody, { cx + 5.f, cy - 10.f }, { cx + 5.f, cy - 3.f }, 2.f, lockColor);
					continue;
				}
				Rect(mStaticBody, { px + 2.f, py + 2.f }, { Pitch - 4.f, Pitch - 4.f }, Alpha(zone, fl(150.f)), Alpha(zone, fl(96.f)));
				const sf::Color edge = Alpha(Mul(zone, 1.25f), fl(215.f));
				const float ex0 = px + 2.f, ex1 = px + Pitch - 2.f, ey0 = py + 2.f, ey1 = py + Pitch - 2.f;
				Seg(mStaticBody, { ex0, ey0 }, { ex1, ey0 }, 1.5f, edge);
				Seg(mStaticBody, { ex0, ey1 }, { ex1, ey1 }, 1.5f, edge);
				Seg(mStaticBody, { ex0, ey0 }, { ex0, ey1 }, 1.5f, edge);
				Seg(mStaticBody, { ex1, ey0 }, { ex1, ey1 }, 1.5f, edge);
				const sf::Color tick = Alpha(Mul(zone, 1.35f), fl(190.f));
				const float a = px + 5.f, b = px + Pitch - 5.f, c = py + 5.f, d = py + Pitch - 5.f;
				Seg(mStaticBody, { a, c }, { a + 6.f, c }, 0.9f, tick);
				Seg(mStaticBody, { a, c }, { a, c + 6.f }, 0.9f, tick);
				Seg(mStaticBody, { b, c }, { b - 6.f, c }, 0.9f, tick);
				Seg(mStaticBody, { b, c }, { b, c + 6.f }, 0.9f, tick);
				Seg(mStaticBody, { a, d }, { a + 6.f, d }, 0.9f, tick);
				Seg(mStaticBody, { a, d }, { a, d - 6.f }, 0.9f, tick);
				Seg(mStaticBody, { b, d }, { b - 6.f, d }, 0.9f, tick);
				Seg(mStaticBody, { b, d }, { b, d - 6.f }, 0.9f, tick);
				if (IsActive(x + 1, y) && zoneAt(x + 1, y) != zone) Line(mStaticBody, { px + Pitch, py + 3.f }, { px + Pitch, py + Pitch - 3.f }, 2.f, Alpha(Mul(Lerp(zone, zoneAt(x + 1, y), 0.5f), 1.3f), fl(200.f)));
				if (IsActive(x, y + 1) && zoneAt(x, y + 1) != zone) Line(mStaticBody, { px + 3.f, py + Pitch }, { px + Pitch - 3.f, py + Pitch }, 2.f, Alpha(Mul(Lerp(zone, zoneAt(x, y + 1), 0.5f), 1.3f), fl(200.f)));
			}

		// Voids inside a row span stay dark (normal blend, drawn after the additive hull light).
		for (const RowSpan& row : mRows)
			for (int x = static_cast<int>(row.left / Pitch); x < static_cast<int>(row.right / Pitch); ++x)
				if (!IsActive(x, row.row)) Rect(mHoles, { x * Pitch + 3.f, row.row * Pitch + 3.f }, { Pitch - 6.f, Pitch - 6.f }, sf::Color{ 2, 6, 14, 120 }, sf::Color{ 2, 6, 14, 150 });

		// Module plates.
		for (const HologramModule& m : mData.modules)
		{
			if (m.cells.empty()) continue;
			const float af = m.held ? 0.28f : 1.f;
			sf::Color base = m.color;
			if (m.rebooting)
			{
				const std::uint8_t gray = static_cast<std::uint8_t>(base.r * 0.3f + base.g * 0.59f + base.b * 0.11f);
				base = Mul(Lerp(base, sf::Color{ gray, gray, gray, 255 }, 0.7f), 0.65f);
			}
			AddPlate(mPlates, m.cells, Alpha(Mul(base, 0.62f), 225.f * af), Alpha(Mul(base, 0.30f), 205.f * af));
			if (m.rebooting) AddPlate(mPlates, m.cells, sf::Color{ 2, 6, 14, 110 }, sf::Color{ 2, 6, 14, 150 });

			const sf::Color rim = Lerp(base, RimColor, m.hovered ? 0.55f : 0.25f);
			const float rimAlpha = (m.hovered ? 255.f : 225.f) * af;
			if (m.hovered) AddPlate(mStaticOver, m.cells, Alpha(rim, 46.f), Alpha(rim, 22.f));
			AddRim(mStaticOver, m.cells, 7.f, Alpha(base, fl(34.f * pulse * af + (m.hovered ? 20.f : 0.f))));
			AddRim(mStaticOver, m.cells, m.hovered ? 2.4f : 1.8f, Alpha(rim, fl(rimAlpha)));
			// Faint inner highlight along each exposed top edge.
			for (const sf::Vector2i& c : m.cells)
				if (!Has(m.cells, c.x, c.y - 1))
				{
					const float x0 = c.x * Pitch + Inset + Chamfer + 2.f;
					const float x1 = c.x * Pitch + Inset + Inner - Chamfer - 2.f + (Has(m.cells, c.x + 1, c.y) ? Bridge : 0.f);
					const sf::Color line = m.system ? sf::Color{ 255, 214, 110, 255 } : WhiteCyan;
					Line(mStaticOver, { x0, c.y * Pitch + Inset + 5.f }, { x1, c.y * Pitch + Inset + 5.f }, 1.f, Alpha(line, fl(m.system ? 120.f * af : 60.f * af)));
				}
			if (m.rebooting)
			{
				sf::Vector2i anchor = m.cells.front();
				for (const sf::Vector2i& c : m.cells)
					if (c.y < anchor.y || (c.y == anchor.y && c.x < anchor.x)) anchor = c;
				const float bx = anchor.x * Pitch + Inset + 4.f;
				const float by = anchor.y * Pitch + Inset + Inner - 7.f;
				Rect(mPlates, { bx, by }, { Inner - 8.f, 4.f }, sf::Color{ 2, 8, 16, 220 }, sf::Color{ 2, 8, 16, 220 });
				RectH(mStaticOver, { bx, by }, { (Inner - 8.f) * Clamp01(m.rebootFraction), 4.f }, Alpha(WhiteCyan, 230.f), Alpha(HoloCyan, 230.f));
			}
		}

	}

	void ShipHologramWidget::Draw(sf::RenderWindow& windowRef)
	{
		if (mOutline.empty()) return;
		const float t = mTime;
		// Mostly steady flicker with an occasional quick dip.
		float flicker = 1.f - 0.012f * (0.5f + 0.5f * std::sin(t * 31.f));
		const float cycle = std::fmod(t, 6.7f);
		if (cycle > 6.35f && cycle < 6.45f) flicker -= 0.06f;
		else if (cycle > 6.55f && cycle < 6.6f) flicker -= 0.04f;
		flicker = std::clamp(flicker, 0.93f, 1.f);
		const float pulse = 0.85f + 0.15f * std::sin(t * 2.1f);
		const auto fl = [flicker](float a) { return a * flicker; };

		auto& under = mAnimatedUnder;
		auto& body = mAnimatedBody;
		auto& over = mAnimatedOver;
		under.clear();
		body.clear();
		over.clear();

		const float hullWidth = mHullRight - mHullLeft;
		const float centerX = (mHullLeft + mHullRight) * 0.5f;
		const float projectorY = mHullBottom + 44.f;
		const float rx = std::clamp(hullWidth * 0.46f, 90.f, 240.f);
		const float ry = 18.f;

		// Light cone and projector base.
		{
			const float baseHalf = rx * 0.5f;
			const float topHalf = hullWidth * 0.5f;
			Quad(under, { centerX - baseHalf, projectorY }, { centerX + baseHalf, projectorY }, { centerX + topHalf, mHullTop }, { centerX - topHalf, mHullTop }, Alpha(HoloCyan, fl(30.f)), Alpha(HoloCyan, fl(30.f)), Alpha(HoloCyan, 0.f), Alpha(HoloCyan, 0.f));
			constexpr int segments = 56;
			const auto ellipse = [&](float scale, float angle) { return Vec{ centerX + std::cos(angle) * rx * scale, projectorY + std::sin(angle) * ry * scale }; };
			for (int i = 0; i < segments; ++i)
			{
				const float a0 = static_cast<float>(i) / segments * 6.2831853f;
				const float a1 = static_cast<float>(i + 1) / segments * 6.2831853f;
				const Vec c{ centerX, projectorY };
				Tri(under, c, ellipse(0.5f, a0), ellipse(0.5f, a1), Alpha(WhiteCyan, fl(120.f * pulse)), Alpha(HoloCyan, fl(56.f)), Alpha(HoloCyan, fl(56.f)));
				Quad(under, ellipse(0.5f, a0), ellipse(0.5f, a1), ellipse(1.15f, a1), ellipse(1.15f, a0), Alpha(HoloCyan, fl(56.f)), Alpha(HoloCyan, fl(56.f)), Alpha(HoloCyan, 0.f), Alpha(HoloCyan, 0.f));
			}
			Pts ring, ring2;
			for (int i = 0; i < segments; ++i)
			{
				const float a = static_cast<float>(i) / segments * 6.2831853f;
				ring.push_back(ellipse(0.78f, a));
				ring2.push_back(ellipse(1.0f, a));
			}
			ClosedStrip(under, ring, -0.8f, 0.8f, Alpha(RimColor, fl(150.f * pulse)), Alpha(RimColor, fl(150.f * pulse)));
			ClosedStrip(under, ring2, -0.5f, 0.5f, Alpha(HoloCyan, fl(70.f)), Alpha(HoloCyan, fl(70.f)));
			for (int i = 0; i < 16; ++i)
			{
				const float a = (static_cast<float>(i) + 0.5f) / 16.f * 6.2831853f;
				Seg(under, ellipse(0.9f, a), ellipse(1.07f, a), 1.4f, Alpha(RimColor, fl(90.f)));
			}
		}

		// Hull: gradient fill.
		{
			// Scanlines clipped to the hull, drifting slowly.
			const float drift = std::fmod(t * 6.f, 3.f);
			int n = 0;
			for (float y = mHullTop + drift; y < mHullBottom; y += 3.f, ++n)
			{
				float l = 0.f, r = 0.f;
				if (!SpanAt(y, l, r) || r - l < 2.f) continue;
				const float a = fl(n % 4 == 0 ? 14.f : 8.f);
				Rect(body, { l, y }, { r - l, 1.f }, Alpha(WhiteCyan, a), Alpha(WhiteCyan, a));
			}
		}

		// Placement preview: pulsing outline and wash.
		if (mData.previewActive)
		{
			std::vector<sf::Vector2i> cells;
			for (const sf::Vector2i& c : mData.previewCells)
				if (c.x >= 0 && c.y >= 0 && c.x < mData.width && c.y < mData.height) cells.push_back(c);
			const sf::Color col = mData.previewLegal ? sf::Color{ 90, 255, 200, 255 } : sf::Color{ 255, 90, 90, 255 };
			const float beat = 0.65f + 0.35f * std::sin(t * 6.f);
			AddPlate(over, cells, Alpha(col, 70.f * beat), Alpha(col, 36.f * beat));
			AddRim(over, cells, 8.f, Alpha(col, fl(46.f * beat)));
			AddRim(over, cells, 2.f, Alpha(col, fl(235.f * (0.7f + 0.3f * beat))));
		}

		// Hull rim: inner haze, crisp rim, soft outer glow that pulses.
		ClosedStrip(over, mOutline, -7.f, 0.f, Alpha(HoloCyan, 0.f), Alpha(HoloCyan, fl(26.f * pulse)));
		ClosedStrip(over, mOutline, 0.f, 12.f, Alpha(HoloCyan, fl(78.f * pulse)), Alpha(HoloCyan, 0.f));
		ClosedStrip(over, mOutline, -0.9f, 0.9f, Alpha(RimColor, fl(235.f)), Alpha(RimColor, fl(235.f)));

		// Scan sweep with a fading trail, clipped to the hull.
		{
			const float u = std::fmod(t, 4.5f);
			if (u < 3.5f)
			{
				const float p = u / 3.5f;
				const float y = mHullTop + (mHullBottom - mHullTop) * p;
				constexpr float trail = 80.f;
				for (float ty = y - trail; ty < y; ty += 3.f)
				{
					float l = 0.f, r = 0.f;
					if (!SpanAt(ty + 1.5f, l, r)) continue;
					const float k = (ty - (y - trail)) / trail;
					const float a = fl(70.f * k * k);
					Rect(over, { l, ty }, { r - l, 3.f }, Alpha(WhiteCyan, a), Alpha(WhiteCyan, a));
				}
				float l = 0.f, r = 0.f;
				if (SpanAt(y, l, r))
				{
					Rect(over, { l, y - 1.f }, { r - l, 2.f }, Alpha(WhiteCyan, fl(230.f)), Alpha(WhiteCyan, fl(230.f)));
					Rect(over, { l, y - 5.f }, { r - l, 4.f }, Alpha(WhiteCyan, 0.f), Alpha(WhiteCyan, fl(60.f)));
					Rect(over, { l, y + 1.f }, { r - l, 5.f }, Alpha(WhiteCyan, fl(50.f)), Alpha(WhiteCyan, 0.f));
				}
			}
		}

		// Engine flames: mirrored flames share a phase so the pair stays symmetric; length breathes and jitters, the core is hotter than the sheath.
		for (std::size_t i = 0; i < mFlameRoots.size(); ++i)
		{
			const Vec root = mFlameRoots[i];
			const float k = std::floor(std::abs(root.x - centerX) / Pitch * 2.f);
			const float jitter = Noise(std::floor(t * 22.f) + k * 3.f);
			const float length = 52.f + 9.f * std::sin(t * 8.f + k * 1.9f) + 6.f * jitter;
			const float halfWidth = 17.f + 2.f * jitter;
			const float heat = 0.88f + 0.12f * std::sin(t * 31.f + k);
			Tri(over, { root.x - halfWidth * 1.35f, root.y }, { root.x + halfWidth * 1.35f, root.y }, { root.x, root.y + length * 1.05f }, Alpha(HoloCyan, fl(50.f * heat)), Alpha(HoloCyan, fl(50.f * heat)), Alpha(HoloCyan, 0.f));
			Tri(over, { root.x - halfWidth, root.y }, { root.x + halfWidth, root.y }, { root.x, root.y + length }, Alpha(HoloCyan, fl(150.f * heat)), Alpha(HoloCyan, fl(150.f * heat)), Alpha(HoloCyan, 0.f));
			Tri(over, { root.x - halfWidth * 0.55f, root.y }, { root.x + halfWidth * 0.55f, root.y }, { root.x, root.y + length * 0.72f }, Alpha(WhiteCyan, fl(255.f * heat)), Alpha(WhiteCyan, fl(255.f * heat)), Alpha(WhiteCyan, 0.f));
			Tri(over, { root.x - halfWidth * 0.22f, root.y }, { root.x + halfWidth * 0.22f, root.y }, { root.x, root.y + length * 0.45f }, Alpha(sf::Color::White, fl(255.f)), Alpha(sf::Color::White, fl(255.f)), Alpha(sf::Color::White, 0.f));
			Rect(over, { root.x - halfWidth * 1.1f, root.y - 6.f }, { halfWidth * 2.2f, 6.f }, Alpha(WhiteCyan, 0.f), Alpha(WhiteCyan, fl(90.f * heat)));
		}

		// Dust motes drifting up the cone.
		for (int i = 0; i < 16; ++i)
		{
			const float seed = static_cast<float>(i);
			const float phase = std::fmod(t * (0.05f + 0.03f * Noise(seed)) + Noise(seed + 7.f), 1.f);
			const float y = projectorY + (mHullTop + 20.f - projectorY) * phase;
			const float width = (rx * 0.5f + (hullWidth * 0.5f - rx * 0.5f) * phase) * 0.9f;
			const float x = centerX + std::sin(seed * 5.1f + t * 0.35f) * width * (0.3f + 0.7f * Noise(seed + 3.f));
			const float size = 1.5f + 1.5f * Noise(seed + 11.f);
			const float a = fl(std::sin(phase * 3.14159f) * 150.f);
			Rect(over, { x - size * 0.5f, y - size * 0.5f }, { size, size }, Alpha(WhiteCyan, a), Alpha(WhiteCyan, a));
		}

		sf::RenderStates normal;
		normal.transform.translate(GetResolvedRect().position);
		normal.transform.scale({ mData.scale, mData.scale });
		sf::RenderStates additive = normal;
		additive.blendMode = sf::BlendAdd;
		windowRef.draw(under, additive);
		windowRef.draw(body, additive);
		windowRef.draw(mStaticBody, additive);
		windowRef.draw(mHoles, normal);
		windowRef.draw(mPlates, normal);
		windowRef.draw(mStaticOver, additive);
		windowRef.draw(over, additive);
	}
}
